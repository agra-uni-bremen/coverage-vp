#ifndef RISCV_VP_COVERAGE_H
#define RISCV_VP_COVERAGE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <map>
#include <string>
#include <memory>
#include <vector>
#include <utility>

#include <nlohmann/json.hpp>
#include <elfutils/libdwfl.h>

#include "mem_if.h"
#include "symbolic_context.h"

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
	std::vector<BasicBlock*> blocks;

public:
	~BasicBlockList(void);

	BasicBlock *add(uint64_t start, uint64_t end);
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
	uint64_t last_instr;
	size_t exec_count = 0;

	void to_json(nlohmann::json &);
};

class SourceLine {
public:
	std::string func_name;
	Function::Location definition;

	// References to BasicBlockList elements of func.
	std::vector<BasicBlock*> blocks;

	uint64_t first_instr;
	size_t exec_count = 0;
	std::map<uint64_t, bool> tainted_instrs;

	void to_json(nlohmann::json &);
};

class SourceFile {
	friend class Coverage;

private:
	std::string name;

	std::map<int, SourceLine> lines;
	// The assumption here is that functions/symbol names are unique
	// on a per source file basis. However, this assumption may not
	// neccesairly hold. For example, RIOT uses static inline
	// function defined in header files. As such, multiple symbols
	// will appear at different addresses which are all defined with
	// the same name in the same source file (the corresponding
	// header file).
	//
	// This causes issues and should somehow be addressed.
	std::map<std::string, Function> funcs;

	void to_json(nlohmann::json &);
};

class Coverage {
	int fd = -1;
	Dwfl *dwfl = nullptr;
	Dwfl_Module *mod = nullptr;

	std::map<std::string, SourceFile> files;

	std::string get_loc(Dwfl_Module *, Function::Location &, GElf_Addr);
	void add_func(Function*, uint64_t, uint64_t);
	std::map<uint64_t, bool> get_block_leaders(uint64_t, uint64_t);

public:
	instr_memory_if *instr_mem = nullptr;

	Coverage(std::string path);
	~Coverage(void);

	void init(void);

	void cover(uint64_t addr, bool tainted);
	void marshal(void);
};

}

#endif
