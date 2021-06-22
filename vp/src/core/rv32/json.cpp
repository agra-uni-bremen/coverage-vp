#include <nlohmann/json.hpp>
#include <filesystem>
#include <iostream>

#include "coverage.h"

using namespace rv32;
using namespace nlohmann;

void SourceLine::to_json(json &out) {
	json j;

	j["branches"] = json::array();
	j["count"] = exec_count;
	j["line_number"] = definition.line;
	j["unexecuted_block"] = exec_count == 0;
	j["function_name"] = func;

	out.push_back(j);
}

void Function::to_json(json &out) {
	out.push_back({
		{"blocks", total_blocks},
		{"blocks_executed", exec_blocks},
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
