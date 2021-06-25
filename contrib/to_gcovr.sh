#!/bin/sh
set -e

export PATH="$PATH:${0%/*}"

if [ $# -ne 1 ]; then
	echo "USAGE: ${0##*/} DIRECTORY" 1>&2
	exit 1
fi

find "${1}" -name '*.gcov.json.gz' -type f | to_gcovr.py
