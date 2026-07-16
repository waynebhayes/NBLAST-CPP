#!/bin/bash

OPTSTRING=":q:t:i:o:s:"
QUERY_DATASET_DIRECTORY="$NBLAST_CPP_QUERY_DATASET_DIRECTORY"
TARGET_DATASET_DIRECTORY="${NBLAST_CPP_TARGET_DATASET_DIRECTORY:-$NBLAST_CPP_QUERY_DATASET_DIRECTORY}"
INPUT_FILE=""
OUTPUT_FILE="r-query-out.tsv"
SMAT_FILE="smat.rda"

while getopts "$OPTSTRING" opt; do
  case "$opt" in
  q) QUERY_DATASET_DIRECTORY="$OPTARG" ;;
  t) TARGET_DATASET_DIRECTORY="$OPTARG" ;;
  i) INPUT_FILE="$OPTARG" ;;
  o) OUTPUT_FILE="$OPTARG" ;;
  s) SMAT_FILE="$OPTARG" ;;
  *) echo "Error. Invalid argument: $OPT" ;;
  esac
done

if [[ -n "$OUTPUT_FILE" ]]; then
  OUTPUT_FILE=$("./scripts/lib/timestamp-filename.sh" "out/$OUTPUT_FILE")
fi

mkdir -p out

Rscript scripts/querying/query-r.R "$QUERY_DATASET_DIRECTORY" "$TARGET_DATASET_DIRECTORY" "$INPUT_FILE" "$SMAT_FILE" "$OUTPUT_FILE"
