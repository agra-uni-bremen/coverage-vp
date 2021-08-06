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
