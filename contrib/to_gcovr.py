#!/usr/bin/env python3

import sys
import gzip
import json

dict = {
    "gcovr/format_version": 0.1,
    "files": [],
}

for line in sys.stdin.readlines():
    fp = line.rstrip()

    with gzip.open(fp) as file:
        j = json.loads(file.read())
        for file in j["files"]:
            jfile = {
                "file": file["file"],
                "lines": []
            }

            for line in file["lines"]:
                jfile["lines"].append({
                    "branches": [], # TODO
                    "count": line["count"],
                    "line_number": line["line_number"],
                    "gcovr/noncode": False,
                })

            dict["files"].append(jfile)

print(json.dumps(dict))
