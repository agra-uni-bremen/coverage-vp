#ifndef RISCV_VP_DWARF_H
#define RISCV_VP_DWARF_H

#include <elfutils/libdwfl.h>

class DwarfException : public std::exception {
public:
	std::string msg;

	DwarfException(std::string _msg) : msg(_msg) {}
	const char *what(void) const throw() {
		return msg.c_str();
	}
};

static void throw_dwfl_error(std::string prefix) {
	const char *msg = dwfl_errmsg(dwfl_errno());
	if (msg) {
		throw DwarfException(prefix + ": " + std::string(msg));
	} else {
		throw DwarfException(prefix);
	}
}

#endif
