#include "addr2line.h"
#include "coverage.h"
#include "instr.h"

#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>

#include <fstream>
#include <system_error>
#include <iostream>
#include <filesystem>

#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/filter/gzip.hpp>

#include "dwarf_exception.h"

using namespace rv32;

/* Taken from elfutils example code. */
static char *debuginfo_path;
static const Dwfl_Callbacks offline_callbacks = (Dwfl_Callbacks){
	.find_elf = dwfl_build_id_find_elf,
	.find_debuginfo = dwfl_standard_find_debuginfo,

	.section_address = dwfl_offline_section_address,
	.debuginfo_path = &debuginfo_path,
};

typedef enum {
	INIT_BASIC_BLOCKS,
	INIT_FUNCTIONS,
} handler_type;

struct Context {
	Coverage *cov;
	handler_type handler;
};

#define ARCH RV32
#define FORMAT_VERSION "1"
#define GCC_VERSION "10.3.1 20210424"
#define FILE_EXT ".gcov.json.gz"

#define HAS_PREFIX(STR, PREFIX) \
	(std::string(STR).find(PREFIX) == 0)

Coverage::Coverage(std::string path) {
	const char *fn = path.c_str();

	if ((fd = open(fn, O_RDONLY)) == -1)
		throw std::system_error(errno, std::generic_category());
	if (!(dwfl = dwfl_begin(&offline_callbacks)))
		throw_dwfl_error("dwfl_begin failed");

	if (!(mod = dwfl_report_offline(dwfl, "", fn, fd)))
		throw_dwfl_error("dwfl_report_offline failed");
}

Coverage::~Coverage(void) {
	if (fd >= 0) {
		close(fd);
		fd = -1;
	}

	if (mod) {
		dwfl_report_end(dwfl, NULL, NULL);
		mod = nullptr;
	}

	if (dwfl) {
		dwfl_end(dwfl);
		dwfl = nullptr;
	}
}

/* https://en.wikipedia.org/wiki/Basic_block#Creation_algorithm */
void
Coverage::init_basic_blocks(uint64_t func_start, uint64_t func_end) {
	uint64_t addr;
	bool prev_wasjump = false;

	/* The first instruction is always a leader */
	block_leaders[func_start] = true;

	addr = func_start;
	while (addr < func_end) {
		// Instruction that immediately follows a (un)conditional jump/branch is a leader
		if (prev_wasjump)
			block_leaders[addr] = true;
		prev_wasjump = false;

		uint32_t mem_word = instr_mem->load_instr(addr);
		Instruction instr = Instruction(mem_word);

		// TODO: decode_and_expand_compressed for compressed
		int32_t o = instr.opcode();
		if (o == Opcode::OP_BEQ) {
			uint64_t dest = addr + instr.B_imm();
			if (dest >= func_start && dest < func_end)
				block_leaders[dest] = true;
		} else if (o == Opcode::OP_JAL) {
			uint64_t dest = addr + instr.J_imm();
			if (dest >= func_start && dest < func_end)
				block_leaders[dest] = true;
		} else if (o == Opcode::OP_JALR) {
			// XXX: not implemented → assuming target is a different function
		} else {
			goto next;
		}
		prev_wasjump = true;

next:
		if (instr.is_compressed()) {
			addr += sizeof(uint16_t);
		} else {
			addr += sizeof(uint32_t);
		}
	}
}

void
Coverage::add_func(uint64_t func_start, uint64_t func_end) {
	std::vector<std::pair<Function*, SourceLine*>> prev;
	uint64_t addr = func_start;
	uint64_t block_prev = addr;

	while (addr < func_end) {
		auto sources = get_sources(mod, addr);
		for (auto s : sources) {
			if (s.symbol_name.empty() || s.source_path.empty())
				assert(0);

			bool newFile = files.count(s.source_path) == 0;
			SourceFile &sf = files[s.source_path];
			if (newFile)
				sf.name = std::move(s.source_path);

			bool newFunc = sf.funcs.count(s.symbol_name) == 0;
			Function &f = sf.funcs[s.symbol_name];
			if (newFunc) {
				f.name = std::move(s.symbol_name);
				f.definition.first.line = s.line;
				f.definition.first.column = s.column;
				f.first_instr = addr;
			}

			bool newLine = sf.lines.count(s.line) == 0;
			SourceLine &sl = sf.lines[s.line];
			if (newLine) {
				sl.func_name = f.name;
				sl.definition.line = s.line;
				sl.definition.column = s.column;
				sl.first_instr = addr;

				if (sl.definition.line > f.definition.second.line)
					f.definition.second = sl.definition;
			}
		}

		uint32_t mem_word = instr_mem->load_instr(addr);
		Instruction instr = Instruction(mem_word);
		if (instr.is_compressed()) {
			addr += sizeof(uint16_t);
		} else {
			addr += sizeof(uint32_t);
		}

		bool block_start = block_leaders.count(addr) != 0;
		if (block_start || addr == func_end) {
			assert(block_prev < addr);
			for (auto s : sources) {
				SourceFile &sf = files.at(s.source_path);
				Function &f = sf.funcs.at(s.symbol_name);
				SourceLine sl = sf.lines.at(s.line);

				BasicBlock *bb = blocks.add(block_prev, addr);
				sl.blocks.push_back(bb);
				f.blocks.push_back(bb);
			}
			
			block_prev = addr;
		}
	}
}

static int
handle_func(Dwarf_Die *die, void *arg)
{
	Context *ctx = (Context *)arg;
	Dwarf_Addr lopc, hipc;

	if (dwarf_func_inline(die))
		return DWARF_CB_OK;

	if (dwarf_lowpc(die, &lopc) == 0 && dwarf_highpc(die, &hipc) == 0) {
		lopc += ctx->cov->bias;
		hipc += ctx->cov->bias;

		switch (ctx->handler) {
		case INIT_BASIC_BLOCKS:
			ctx->cov->init_basic_blocks(lopc, hipc);
			break;
		case INIT_FUNCTIONS:
			ctx->cov->add_func(lopc, hipc);
			break;
		}
	}

	return DWARF_CB_OK;
}

// We need to iterate over every insturction since we need to
// map addresses to source lines in Coverage:cover. Iterating
// over each line (e.g. with dwfl_getsrclines) does not work.
//
// In order to iterate over all instructions, we iterate over
// all DWARF Compilation Units (CUs) and extract all functions
// defined in these CUs (using dwarf_getfuncs). Afterwards, we
// iterate over all instructions in these functions. To
// iteratively fill our internal datastructures.
void
Coverage::init(void) {
	Dwarf_Die *cu;
	Context ctx;
	ctx.cov = this;

	bias = 0;
	cu = nullptr;

	// The GCOV JSON data structure includes information about basic
	// blocks. Theoratically it is possible to encode information
	// about basic block leaders in the DWARF Line Table. However,
	// either GCC does not support this or I was unable to configure
	// it accordingly. For this reason, we extract basic block
	// leaders manually (through init_basic_blocks) first and then
	// iterate over all instructoins (through add_func).

	ctx.handler = INIT_BASIC_BLOCKS;
	while ((cu = dwfl_module_nextcu(mod, cu, &bias)))
		dwarf_getfuncs(cu, handle_func, (void *)&ctx, 0);

	bias = 0;
	cu = nullptr;

	ctx.handler = INIT_FUNCTIONS;
	while ((cu = dwfl_module_nextcu(mod, cu, &bias)))
		dwarf_getfuncs(cu, handle_func, (void *)&ctx, 0);
}

void Coverage::cover(uint64_t addr, bool tainted) {
	auto sources = get_sources(mod, addr);
	for (auto source : sources) {
		if (files.count(source.source_path) == 0)
			continue; /* assembler file */

		SourceFile &f = files.at(source.source_path);

		Function &func = f.funcs.at(source.symbol_name);
		if (addr == func.first_instr)
			func.exec_count++;
		blocks.visit(addr);

		SourceLine &sl = f.lines.at((unsigned int)source.line);
		if (addr == sl.first_instr)
			sl.exec_count++;
		
		if (tainted) {
			sl.tainted_instrs[addr] = true;
		} else if (!tainted) {
			auto elem = sl.tainted_instrs.find(addr);
			if (elem != sl.tainted_instrs.end())
				sl.tainted_instrs.erase(elem);
		}
	}
}

void Coverage::marshal(void) {
	nlohmann::json j;
	char *path_filter;

	j["format_version"] = FORMAT_VERSION;
	j["gcc_version"] = GCC_VERSION;

	j["files"] = nlohmann::json::array();
	nlohmann::json &jfiles = j["files"];

	path_filter = getenv("SYMEX_COVERAGE_PATH");
	for (auto &f : files) {
		jfiles.clear();
		auto path = std::filesystem::path(f.first);

		if (path_filter && !HAS_PREFIX(path, path_filter))
			continue;

		SourceFile &file = f.second;
		file.to_json(jfiles);

		j["data_file"] = path.filename();
		j["current_working_directory"] = path.parent_path();

		auto fp = f.first + FILE_EXT;
		std::ofstream fout(fp);
		if (!fout.is_open())
			throw std::runtime_error("failed to open " + std::string(fp));

		boost::iostreams::filtering_streambuf<boost::iostreams::output> gzstr;
		gzstr.push(boost::iostreams::gzip_compressor());
		gzstr.push(fout);

		std::ostream out(&gzstr);
		out << std::setw(4) << j << std::endl;
	}
}
