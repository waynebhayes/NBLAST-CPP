#!/bin/bash

OPTSTRING=":n:d:s:o:"
TEST_SET_QUANTITY=1000
DATASET_DIRECTORY="$NBLAST_CPP_QUERY_DATASET_DIRECTORY"
SEED=""
OUTPUT_FILE=""

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

while getopts "$OPTSTRING" opt; do
  case "$opt" in
  n) TEST_SET_QUANTITY="$OPTARG" ;;
  d) DATASET_DIRECTORY="$OPTARG" ;;
  s) SEED="$OPTARG" ;;
  o) OUTPUT_FILE="$OPTARG" ;;
  *) echo "Error. Invalid argument: $OPT" ;;
  esac
done

if [[ -z "$SEED" ]]; then
  filenames=$(find "$DATASET_DIRECTORY" -type f | shuf -n "$TEST_SET_QUANTITY")
else
  filenames=$(find "$DATASET_DIRECTORY" -type f | shuf -n "$TEST_SET_QUANTITY" --random-source=<(yes "$SEED"))
fi

if [[ -n "$OUTPUT_FILE" ]]; then
  OUTPUT_FILE=$("$SCRIPT_DIR/timestamp-filename.sh" "$OUTPUT_FILE")
fi

for file in $filenames; do
  basename "$file" ".swc"
done | if [[ -n "$OUTPUT_FILE" ]]; then cat >"$OUTPUT_FILE"; else cat; fi
