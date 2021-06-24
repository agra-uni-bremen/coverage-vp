#ifndef RISCV_VP_COVERAGE_H
#define RISCV_VP_COVERAGE_H

#include <stdint.h>
#include <stddef.h>

#include <map>
#include <string>
#include <memory>
#include <vector>
#include <utility>
#include <memory>

#include <nlohmann/json.hpp>
#include <elfutils/libdwfl.h>

#include "mem_if.h"

namespace rv32 {

class BasicBlock {
public:
	uint64_t start, end;
	bool visited = false;

	BasicBlock(uint64_t _start, uint64_t _end)
		: start(_start), end(_end), visited(false) {}
};

class BasicBlockList {
private:
	std::vector<BasicBlock> blocks;

public:
	std::unique_ptr<BasicBlock> add(uint64_t start, uint64_t end);
	void visit(uint64_t addr);

	size_t size(void);
	size_t visited(void);
};

struct Function {
public:
	struct Location {
		unsigned int line;
		unsigned int column;
	};

	std::string name;
	std::pair<Location, Location> definition;
	BasicBlockList blocks;

	uint64_t first_instr;
	size_t exec_count;

	void to_json(nlohmann::json &);
};

class SourceLine {
public:
	Function* func;
	Function::Location definition;

	// References to BasicBlockList elements of func.
	std::vector<std::unique_ptr<BasicBlock>> blocks;

	uint64_t first_instr;
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
	std::map<uint64_t, bool> get_block_leaders(uint64_t, uint64_t);
	void add_lines(SourceFile &, Function &f, uint64_t, uint64_t);

public:
	Coverage(std::string path, instr_memory_if *_instr_mem);
	~Coverage(void);

	void cover(uint64_t addr);
	void marshal(void);
};

}

#endif
