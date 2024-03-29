/*
 * Copyright (c) 2021 Group of Computer Architecture, University of Bremen
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

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
};

struct Function {
public:
	struct Location {
		int line;
		int column;
	};

	std::string name;
	std::pair<Location, Location> definition;

	// References to BasicBlockList elements of Coverage.
	std::vector<BasicBlock*> blocks;

	uint64_t first_instr;
	size_t exec_count = 0;

	void to_json(nlohmann::json &);
};

class SourceLine {
public:
	std::string func_name;
	Function::Location definition;

	// References to BasicBlockList elements of Coverage.
	std::vector<BasicBlock*> blocks;

	uint64_t first_instr;
	size_t exec_count = 0;

	bool symbolic_once = false;
	bool tainted_once = false;
	bool initial_conc = false;

	void to_json(nlohmann::json &);
};

class SourceFile {
	friend class Coverage;

private:
	std::string name;

	std::map<int, SourceLine> lines;
	std::map<std::string, Function> funcs;

	void to_json(nlohmann::json &);
};

class Coverage {
	int fd = -1;
	Dwfl *dwfl = nullptr;
	Dwfl_Module *mod = nullptr;
	BasicBlockList blocks;

	std::map<uint64_t, bool> block_leaders;
	std::map<std::string, SourceFile> files;

public:
	Dwarf_Addr bias = 0;
	instr_memory_if *instr_mem = nullptr;

	Coverage(std::string path);
	~Coverage(void);

	void init(void);
	void init_basic_blocks(uint64_t, uint64_t);
	void add_func(uint64_t, uint64_t);

	void cover(uint64_t addr, bool tainted, bool symbolic, bool init);
	void marshal(void);
};

}

#endif
