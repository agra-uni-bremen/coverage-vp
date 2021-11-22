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

#ifndef RISCV_VP_ADDR2LINE_H
#define RISCV_VP_ADDR2LINE_H

#include <elfutils/libdwfl.h>

#include <string>
#include <vector>

class SourceInfo {
public:
	std::string symbol_name;
	std::string source_path;

	int line;
	int column;

	SourceInfo(std::string _symbol_name, std::string _source_path, int _line, int _column)
		: symbol_name(_symbol_name), source_path(_source_path), line(_line), column(_column) {}
};

// For a given instruction address this function returns a SourceInfo
// for all effected source code lines. For example, when a function is
// inlined both the inlined code and the invocation of the inlined code
// in the caller need to be marked as executed.
std::vector<SourceInfo> get_sources(Dwfl_Module *mod, Dwarf_Addr addr);

#endif
