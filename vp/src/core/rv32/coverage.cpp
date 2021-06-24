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

using namespace rv32;

/* Taken from elfutils example code. */
static char *debuginfo_path;
static const Dwfl_Callbacks offline_callbacks = (Dwfl_Callbacks){
	.find_elf = dwfl_build_id_find_elf,
	.find_debuginfo = dwfl_standard_find_debuginfo,

	.section_address = dwfl_offline_section_address,
	.debuginfo_path = &debuginfo_path,
};

#define ARCH RV32
#define FORMAT_VERSION "1"
#define GCC_VERSION "10.3.1 20210424"
#define FILE_EXT ".gcov.json.gz"

/* True if the instruction ends a basic block. */
static inline bool basic_block_end(Instruction &i) {
	int32_t o = i.opcode();
	return o == Opcode::OP_JAL || o == Opcode::OP_JALR || o == Opcode::OP_BEQ;
}

Coverage::Coverage(std::string path, instr_memory_if *_instr_mem)
  : instr_mem(_instr_mem) {
	const char *fn = path.c_str();

	if ((fd = open(fn, O_RDONLY)) == -1)
		throw std::system_error(errno, std::generic_category());
	if (!(dwfl = dwfl_begin(&offline_callbacks)))
		throw std::runtime_error("dwfl_begin failed");

	if (!(mod = dwfl_report_offline(dwfl, "main", "main", fd)))
		throw std::runtime_error("dwfl_report_offline failed");

	init();
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

void
Coverage::init(void) {
	int n;

	if ((n = dwfl_module_getsymtab(mod)) == -1)
		throw std::runtime_error("dwfl_module_getsymtab failed");

	for (int i = 0; i < n; i++) {
		GElf_Sym sym;
		GElf_Addr addr;
		const char *name;
		uint64_t end;

		name = dwfl_module_getsym_info(mod, i, &sym, &addr, NULL, NULL, NULL);
		if (!name || GELF_ST_TYPE(sym.st_info) != STT_FUNC)
			continue; /* not a function symbol */
		end = (uint64_t)(addr + sym.st_size);

		std::pair<Function::Location, Function::Location> def;
		std::string fp = get_loc(mod, def.first, addr);
		get_loc(mod, def.second, end - sizeof(uint32_t)); // XXX

		bool newfile = files.count(fp) == 0;
		SourceFile &sf = files[fp];
		if (newfile)
			sf.name = std::move(fp);

		Function &f = sf.funcs[name];
		f.name = std::string(name);
		f.first_instr = addr;
		f.definition = def;
		f.exec_count = 0;
		add_lines(sf, f, addr, end);
	}
}

std::string Coverage::get_loc(Dwfl_Module *mod, Function::Location &loc, GElf_Addr addr) {
	Dwfl_Line *line;
	const char *srcfp;
	int lnum, cnum;

	line = dwfl_module_getsrc(mod, addr);
	if (!line)
		throw std::runtime_error("dwfl_module_getsrc failed");

	if (!(srcfp = dwfl_lineinfo(line, NULL, &lnum, &cnum, NULL, NULL)))
		throw std::runtime_error("dwfl_lineinfo failed");

	assert(lnum > 0 && cnum > 0);

	loc.line = (unsigned int)lnum;
	loc.column = (unsigned int)cnum;

	return std::string(srcfp);
}

void Coverage::add_lines(SourceFile &sf, Function &f, uint64_t addr, uint64_t end) {
	uint32_t mem_word;
	Instruction instr;
	uint64_t bb_start;

	bb_start = addr;
	while (addr < end) {
		Dwfl_Line *line;
		int lnum, cnum;
		BasicBlock *bb;

		line = dwfl_module_getsrc(mod, addr);
		if (!line)
			throw std::runtime_error("dwfl_module_getsrc failed");
		if (!dwfl_lineinfo(line, NULL, &lnum, &cnum, NULL, NULL))
			throw std::runtime_error("dwfl_lineinfo failed");

		bool newLine = sf.lines.count(lnum) == 0;
		SourceLine &sl = sf.lines[lnum];
		if (newLine) {
			sl.func = &f;
			sl.definition.line = (unsigned int)lnum;
			sl.definition.column = (unsigned int)cnum;
			sl.first_instr = addr;
		}

		mem_word = instr_mem->load_instr(addr);
		instr = Instruction(mem_word);
		if (instr.is_compressed()) {
			addr += sizeof(uint16_t);
		} else {
			addr += sizeof(uint32_t);
		}

		if (basic_block_end(instr)) {
			bb = f.blocks.add(bb_start, addr);
			sl.blocks.push_back(bb);
			bb_start = addr;
		}
	}
}

void Coverage::cover(uint64_t addr) {
	Dwfl_Line *line;
	int lnum, cnum;
	const char *srcfp, *symbol;

	line = dwfl_module_getsrc(mod, addr);
	if (!line)
		throw std::runtime_error("dwfl_module_getsrc failed");
	if (!(srcfp = dwfl_lineinfo(line, NULL, &lnum, &cnum, NULL, NULL)))
		throw std::runtime_error("dwfl_lineinfo failed");

	std::string name = std::string(srcfp);
	if (files.count(name) == 0)
		return; /* assembly file, etc */
	SourceFile &f = files.at(name);

	if (!(symbol = dwfl_module_addrname(mod, addr)))
		throw std::runtime_error("dwfl_module_addrname failed");

	Function &func = f.funcs.at(symbol);
	if (addr == func.first_instr)
		func.exec_count++;
	func.blocks.visit(addr);

	SourceLine &sl = f.lines.at(lnum);
	if (addr == sl.first_instr)
		sl.exec_count++;
}

void Coverage::marshal(void) {
	nlohmann::json j;

	j["format_version"] = FORMAT_VERSION;
	j["gcc_version"] = GCC_VERSION;

	j["files"] = nlohmann::json::array();
	nlohmann::json &jfiles = j["files"];

	for (auto &f : files) {
		jfiles.clear();
		auto path = std::filesystem::path(f.first);

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
