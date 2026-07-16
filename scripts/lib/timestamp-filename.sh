#!/bin/bash
# Usage: timestamp-filename.sh <filename>
# Prints the filename with a timestamp inserted before the extension.
# e.g. out.tsv -> out_20260624_143012.tsv

FILE="$1"
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
EXT="${FILE##*.}"
BASE="${FILE%.*}"
echo "${BASE}_${TIMESTAMP}.${EXT}"
