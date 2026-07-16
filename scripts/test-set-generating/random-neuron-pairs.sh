#!/bin/bash

OPTSTRING=":n:q:t:s:o:"
TEST_SET_QUANTITY=1000
QUERY_DATASET_DIRECTORY="$NBLAST_CPP_QUERY_DATASET_DIRECTORY"
TARGET_DATASET_DIRECTORY="$NBLAST_CPP_TARGET_DATASET_DIRECTORY"
SEED=""
OUTPUT_FILE="random-neuron-pairs.tsv"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

while getopts "$OPTSTRING" opt; do
  case "$opt" in
  n) TEST_SET_QUANTITY="$OPTARG" ;;
  q) QUERY_DATASET_DIRECTORY="$OPTARG" ;;
  t) TARGET_DATASET_DIRECTORY="$OPTARG" ;;
  s) SEED="$OPTARG" ;;
  o) OUTPUT_FILE="$OPTARG" ;;
  *) echo "Error. Invalid argument: $OPT" ;;
  esac
done

SEED_ARG=()
if [[ -n "$SEED" ]]; then
  SEED_ARG=(-s "$SEED")
fi

query=$("$SCRIPT_DIR/random-neuron-list.sh" -n "$TEST_SET_QUANTITY" -d "$QUERY_DATASET_DIRECTORY" "${SEED_ARG[@]}")
target=$("$SCRIPT_DIR/random-neuron-list.sh" -n "$TEST_SET_QUANTITY" -d "$TARGET_DATASET_DIRECTORY" "${SEED_ARG[@]}")

if [[ -n "$OUTPUT_FILE" ]]; then
  OUTPUT_FILE=$("scripts/lib/timestamp-filename.sh" "out/$OUTPUT_FILE")
fi

paste <(echo "$query") <(echo "$target") |
  if [[ -n "$OUTPUT_FILE" ]]; then cat >"$OUTPUT_FILE"; else cat; fi
