#ifndef RISCV_VP_COVERAGE_H
#define RISCV_VP_COVERAGE_H

#include <stdint.h>
#include <stddef.h>

#include <map>
#include <string>
#include <memory>
#include <vector>
#include <utility>
#include <fstream>

#include <elfutils/libdwfl.h>

#include "mem_if.h"

namespace rv32 {

struct Function {
public:
	struct Location {
		unsigned int line;
		unsigned int column;
	};

	std::string name;
	std::pair<Location, Location> definition;

	size_t total_blocks;
	size_t exec_blocks;
	size_t exec_count;
};

class SourceFile {
	friend class Coverage;

private:
	std::string name;
	std::map<std::string, Function> funcs;

public:
	void marshal(std::ostream &stream);
};

class Coverage {
	int fd = -1;
	Dwfl *dwfl = nullptr;
	instr_memory_if *instr_mem = nullptr;

	std::map<std::string, SourceFile> files;

	void init(void);
	std::string get_loc(Dwfl_Module *, Function::Location &, GElf_Addr);
	size_t count_blocks(uint64_t, uint64_t);

public:
	Coverage(std::string path, instr_memory_if *_instr_mem);
	~Coverage(void);
};

}

#endif
