#ifndef RISCV_VP_INLINE_H
#define RISCV_VP_INLINE_H

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

std::vector<SourceInfo> get_sources(Dwfl_Module *mod, Dwarf_Addr addr);

#endif
