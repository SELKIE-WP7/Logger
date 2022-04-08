#!/bin/bash
if [ $# -ne 4 ]; then
	echo "Usage: $0 name section source destination"
	exit 1
fi

TITLE=$1
SECTION=$2
INFILE=$3
OUTFILE=$4

if [ "${SECTION}" -lt 1 ] || [ "${SECTION}" -gt 8 ]; then
	echo "Invalid man page section"
	exit 1
fi

mkdir -p "$(dirname "${OUTFILE}")"

sed '1d' "${INFILE}"|pandoc -s --shift-heading-level-by=-1 -V title="${TITLE}" -V section="${SECTION}" -f markdown -t man | gzip -c > "${OUTFILE}"
