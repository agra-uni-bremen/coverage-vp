#ifndef RISCV_VP_COVERAGE_H
#define RISCV_VP_COVERAGE_H

#include <stdint.h>
#include <stddef.h>

#include <map>
#include <string>
#include <memory>
#include <vector>
#include <utility>

#include <elfutils/libdwfl.h>

#include "mem_if.h"

namespace rv32 {

struct Function {
	struct Location {
		unsigned int line;
		unsigned int column;
	};

	size_t total_blocks;
	size_t exec_blocks;
	size_t exec_count;

	std::string name;
	std::pair<Location, Location> definition;
};

class File {
	std::string name;
	std::map<std::string, Function> funcs;
};

class Coverage {
	int fd = -1;
	Dwfl *dwfl = nullptr;
	instr_memory_if *instr_mem = nullptr;

	std::map<std::string, File> files;

	void init(void);
	std::string location_info(Dwfl_Module *, Function::Location &, GElf_Addr);
	size_t count_blocks(uint64_t, uint64_t);

public:
	Coverage(std::string path, instr_memory_if *_instr_mem);
	~Coverage(void);
};

}

#endif
