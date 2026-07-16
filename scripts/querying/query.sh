#!/bin/bash

OPTSTRING=":q:t:i:o:m:d"
QUERY_DATASET_DIRECTORY="$NBLAST_CPP_QUERY_DATASET_DIRECTORY"
TARGET_DATASET_DIRECTORY="$NBLAST_CPP_TARGET_DATASET_DIRECTORY"
INPUT_FILE=""
OUTPUT_FILE="scores.tsv"
MATRIX_FILE=""
DEBUG=""

while getopts "$OPTSTRING" opt; do
  case "$opt" in
  q) QUERY_DATASET_DIRECTORY="$OPTARG" ;;
  t) TARGET_DATASET_DIRECTORY="$OPTARG" ;;
  i) INPUT_FILE="$OPTARG" ;;
  o) OUTPUT_FILE="$OPTARG" ;;
  m) MATRIX_FILE="$OPTARG" ;;
  d) DEBUG="TRUE" ;;
  *) echo "Error. Invalid argument: $OPT" >&2 ;;
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

echo "filtering input..." >&2

filtered_input=$(awk '$1 != "NA" && $2 != "NA" { print $1, $2 }' "$INPUT_FILE")

echo "filtering done" >&2

echo "querying..." >&2

{
  echo -e "neuron1\tneuron2\tscore"
  echo "$filtered_input" | ./nblast++ -q "$MATRIX_FILE" -i "$QUERY_DATASET_DIRECTORY,$TARGET_DATASET_DIRECTORY"
} |
  if [[ -n "$OUTPUT_FILE" ]]; then cat >"$OUTPUT_FILE"; else cat; fi

echo "querying done." >&2
