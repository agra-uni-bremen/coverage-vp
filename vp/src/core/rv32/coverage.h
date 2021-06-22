#ifndef RISCV_VP_COVERAGE_H
#define RISCV_VP_COVERAGE_H

#include <stdint.h>
#include <stddef.h>

#include <map>
#include <string>
#include <memory>
#include <vector>
#include <utility>

#include <nlohmann/json.hpp>
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

	void to_json(nlohmann::json &);
};

class SourceLine {
public:
	std::string func;
	Function::Location definition;
	size_t exec_count = 0;

	void to_json(nlohmann::json &);
};

class SourceFile {
	friend class Coverage;

private:
	std::string name;

	std::map<std::string, Function> funcs;
	std::map<int, SourceLine> lines;

	void to_json(nlohmann::json &);
};

class Coverage {
	int fd = -1;
	Dwfl *dwfl = nullptr;
	Dwfl_Module *mod = nullptr;
	instr_memory_if *instr_mem = nullptr;

	std::map<std::string, SourceFile> files;

	void init(void);
	std::string get_loc(Dwfl_Module *, Function::Location &, GElf_Addr);
	size_t count_blocks(uint64_t, uint64_t);

public:
	Coverage(std::string path, instr_memory_if *_instr_mem);
	~Coverage(void);

	void cover(uint64_t addr);
	void marshal(void);
};

}

#endif
