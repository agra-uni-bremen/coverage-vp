#ifndef RISCV_VP_COVERAGE_H
#define RISCV_VP_COVERAGE_H

#include <stdint.h>

#include <map>
#include <string>
#include <memory>
#include <vector>

#include <elfutils/libdwfl.h>

#include "mem_if.h"

namespace rv32 {

class ELFFile {
	int fd = -1;
	Dwfl *dwfl = nullptr;
	Dwfl_Module *mod = nullptr;
	instr_memory_if *instr_mem = nullptr;

	std::map<std::string, uint64_t> symbols;
	void init(void);
	size_t count_blocks(uint64_t, uint64_t);

public:
	ELFFile(std::string path, instr_memory_if *_instr_mem);
	~ELFFile(void);
};

}

#endif
