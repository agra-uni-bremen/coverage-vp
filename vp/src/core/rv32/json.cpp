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

#include <assert.h>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <iostream>

#include "coverage.h"

using namespace rv32;
using namespace nlohmann;

void SourceLine::to_json(json &out) {
	json j;

	auto has_unexecuted_block = [this](void) {
		if (exec_count == 0)
			return true;

		for (size_t i = 0; i < blocks.size(); i++) {
			if (!blocks[i]->visited)
				return true;
		}

		return false;
	};

	j["branches"] = json::array();
	j["count"] = exec_count;
	j["line_number"] = definition.line;
	j["unexecuted_block"] = has_unexecuted_block();
	j["function_name"] = func_name;
	j["symex/tainted_once"] = tainted_once;
	j["symex/symbolic_once"] = symbolic_once;
	j["symex/initial_concretization"] = initial_conc;

	out.push_back(j);
}

void Function::to_json(json &out) {
	size_t visited = 0;
	for (auto block : blocks)
		if (block->visited) visited++;

	out.push_back({
		{"blocks", blocks.size()},
		{"blocks_executed", visited},
		{"demangled_name", name},
		{"end_column", definition.second.column},
		{"end_line", definition.second.line},
		{"execution_count", exec_count},
		{"name", name},
		{"start_column", definition.first.column},
		{"start_line", definition.first.line},
	});
}

void SourceFile::to_json(json &out) {
	json j;
	json jlines;
	json jfuncs;

	for (auto &l : lines) {
		l.second.to_json(jlines);
	}

	for (auto &p : funcs) {
		Function &f = p.second;
		f.to_json(jfuncs);
	}

	j["file"] = std::filesystem::path(name).filename();
	j["lines"] = jlines;
	j["functions"] = jfuncs;

	out.push_back(j);
}
