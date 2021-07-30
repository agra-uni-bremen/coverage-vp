#include "inline.h"
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

#define HAS_PREFIX(STR, PREFIX) \
	(std::string(STR).find(PREFIX) == 0)

class DwarfException : public std::exception {
public:
	std::string msg;

	DwarfException(std::string _msg) : msg(_msg) {}
	const char *what(void) const throw() {
		return msg.c_str();
	}
};

static void throw_dwfl_error(std::string prefix) {
	const char *msg = dwfl_errmsg(dwfl_errno());
	if (msg) {
		throw DwarfException(prefix + ": " + std::string(msg));
	} else {
		throw DwarfException(prefix);
	}
}

Coverage::Coverage(std::string path) {
	const char *fn = path.c_str();

	if ((fd = open(fn, O_RDONLY)) == -1)
		throw std::system_error(errno, std::generic_category());
	if (!(dwfl = dwfl_begin(&offline_callbacks)))
		throw_dwfl_error("dwfl_begin failed");

	if (!(mod = dwfl_report_offline(dwfl, "main", "main", fd)))
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

void
Coverage::init(void) {
	Dwarf_Addr bias;
	Dwarf_Die *cu;

	bias = 0;
	cu = nullptr;

	while ((cu = dwfl_module_nextcu(mod, cu, &bias))) {
		size_t lines;
		Dwarf_Addr block_prev = 0; /* Previous basic block start address */

		if (dwfl_getsrclines (cu, &lines) != 0)
			continue; // No line information

		for (size_t i = 0; i < lines; i++) {
			Dwarf_Addr addr;
			Dwfl_Line *line;

			line = dwfl_onesrcline (cu, i);
			if (!dwfl_lineinfo(line, &addr, NULL, NULL, NULL, NULL))
				throw_dwfl_error("dwfl_lineinfo failed");

			if (!dwfl_module_addrname(mod, addr)) {
				std::cerr << "Warning: Could not determine name for 0x" << std::hex << addr << std::endl;
				continue;
			}

			if (block_prev == 0)
				block_prev = addr;
			bool block_start;
			dwarf_lineblock(dwarf_getsrc_die(cu, addr), &block_start);

			auto sources = get_sources(mod, addr);
			for (auto s : sources) {
				if (s.symbol_name.empty() || s.source_path.empty())
					continue; // FIXME: Bug in get_sources

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

				if (block_start && block_prev != 0) {
					BasicBlock *bb = f.blocks.add(block_prev, block_start);
					std::cout << "Block between 0x" << std::hex << block_prev << " 0x" << std::hex << block_start << std::endl;
					sl.blocks.push_back(bb);
					block_prev = 0;
				}
			}
		}
	}
}

void Coverage::cover(uint64_t addr, bool tainted) {
	auto sources = get_sources(mod, addr);
	for (auto source : sources) {
		SourceFile &f = files.at(source.source_path);

		Function &func = f.funcs.at(source.symbol_name);
		if (addr == func.first_instr)
			func.exec_count++;
		func.blocks.visit(addr); // XXX

		SourceLine &sl = f.lines.at((unsigned int)source.line);
		if (addr == sl.first_instr)
			sl.exec_count++;
		
		// XXX: Addr handling for inlined stuff?
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
