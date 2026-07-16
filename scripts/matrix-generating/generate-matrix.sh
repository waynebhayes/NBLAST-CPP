#!/bin/bash

OPTSTRING=":q:t:i:o:n:d"
QUERY_DATASET_DIRECTORY="$NBLAST_CPP_QUERY_DATASET_DIRECTORY"
TARGET_DATASET_DIRECTORY="$NBLAST_CPP_TARGET_DATASET_DIRECTORY"
INPUT_FILE="$NBLAST_CPP_KNOWN_CLASSES_FILE"
OUTPUT_FILE="cpp-smat.tsv"
NUM_ITERS=10000
DEBUG=""

while getopts "$OPTSTRING" opt; do
  case "$opt" in
  q) QUERY_DATASET_DIRECTORY="$OPTARG" ;;
  t) TARGET_DATASET_DIRECTORY="$OPTARG" ;;
  i) INPUT_FILE="$OPTARG" ;;
  o) OUTPUT_FILE="$OPTARG" ;;
  n) NUM_ITERS="$OPTARG" ;;
  d) DEBUG="TRUE" ;;
  *) echo "Error. Invalid argument: $OPT" ;;
  esac
done

echo "building nblast-cpp..." >&2

if [[ -n "$DEBUG" ]]; then
  make clean debug >/dev/null
else
  make clean release >/dev/null
fi

echo "building done." >&2

echo "timestamping filename..." >&2

if [[ -n "$OUTPUT_FILE" ]]; then
  OUTPUT_FILE=$("./scripts/lib/timestamp-filename.sh" "out/$OUTPUT_FILE")
fi

echo "timestamping done" >&2

mkdir -p out

echo "generating matrix using $INPUT_FILE..." >&2

./nblast++ -g "$INPUT_FILE","$NUM_ITERS" -i "$QUERY_DATASET_DIRECTORY,$TARGET_DATASET_DIRECTORY" |
  if [[ -n "$OUTPUT_FILE" ]]; then cat >"$OUTPUT_FILE"; else cat; fi

echo "matrix generation done." >&2
