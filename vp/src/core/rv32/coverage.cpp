#include "coverage.h"
#include "instr.h"

#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>

#include <system_error>
#include <iostream>

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

Coverage::Coverage(std::string path, instr_memory_if *_instr_mem)
  : instr_mem(_instr_mem) {
	const char *fn = path.c_str();

	if ((fd = open(fn, O_RDONLY)) == -1)
		throw std::system_error(errno, std::generic_category());
	if (!(dwfl = dwfl_begin(&offline_callbacks)))
		throw std::runtime_error("dwfl_begin failed");

	init();
}

Coverage::~Coverage(void) {
	if (fd >= 0) {
		close(fd);
		fd = -1;
	}

	if (dwfl) {
		dwfl_end(dwfl);
		dwfl = nullptr;
	}
}

void
Coverage::init(void) {
	int n;
	Dwfl_Module *mod;

	if (!(mod = dwfl_report_offline(dwfl, "main", "main", fd)))
		throw std::runtime_error("dwfl_report_offline failed");
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
		f.definition = def;
		f.total_blocks = count_blocks(addr, end);
		f.exec_blocks = 0;
		f.exec_count = 0;
	}

	dwfl_report_end(dwfl, NULL, NULL);
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

	cnum++; /* For some reason, columns start at zero */
	assert(lnum > 0 && cnum > 0);

	loc.line = (unsigned int)lnum;
	loc.column = (unsigned int)cnum;

	return std::string(srcfp);
}

size_t Coverage::count_blocks(uint64_t addr, uint64_t end) {
	size_t num_blocks;
	uint32_t mem_word;
	int32_t opcode;
	Instruction instr;

	/* Each function has *at least* one basic block. */
	num_blocks = 1;

	while (addr < end) {
		mem_word = instr_mem->load_instr(addr);
		instr = Instruction(mem_word);

		if (instr.is_compressed()) {
			addr += sizeof(uint16_t);
		} else {
			addr += sizeof(uint32_t);
		}

		opcode = instr.opcode();
		if (opcode == Opcode::OP_JAL || opcode == Opcode::OP_BEQ) {
			num_blocks++;
		}
	}

	return num_blocks;
}
