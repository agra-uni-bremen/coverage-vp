#include <nlohmann/json.hpp>
#include <filesystem>
#include <iostream>

#include "coverage.h"

using namespace rv32;
using namespace nlohmann;

void SourceFile::to_json(json &out) {
	json j;
	json functions;

	for (auto &p : funcs) {
		Function &f = p.second;
		functions.push_back({
			{"blocks", f.total_blocks},
			{"blocks_executed", f.exec_blocks},
			{"demangled_name", f.name},
			{"end_column", f.definition.second.column},
			{"end_line", f.definition.second.line},
			{"execution_count", f.exec_count},
			{"name", f.name},
			{"start_column", f.definition.first.column},
			{"start_line", f.definition.first.line},
		});
	}

	j["file"] = std::filesystem::path(name).filename();
	j["lines"] = json::array();
	j["functions"] = functions;

	out.push_back(j);
}
