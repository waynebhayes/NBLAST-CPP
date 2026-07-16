#!/bin/bash

OPTSTRING=":q:t:i:o:d"
QUERY_DATASET_DIRECTORY="$NBLAST_CPP_QUERY_DATASET_DIRECTORY"
INPUT_FILE=""
OUTPUT_FILE="r-smat.tsv"

while getopts "$OPTSTRING" opt; do
  case "$opt" in
  q) QUERY_DATASET_DIRECTORY="$OPTARG" ;;
  i) INPUT_FILE="$OPTARG" ;;
  o) OUTPUT_FILE="$OPTARG" ;;
  *) echo "Error. Invalid argument: $OPT" ;;
  esac
done

if [[ -n "$OUTPUT_FILE" ]]; then
  OUTPUT_FILE=$("./scripts/lib/timestamp-filename.sh" "out/$OUTPUT_FILE")
fi

mkdir -p out

Rscript scripts/matrix-generating/generate-matrix-r.R "$QUERY_DATASET_DIRECTORY" "$INPUT_FILE" "$OUTPUT_FILE"
