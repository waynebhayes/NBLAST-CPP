#!/bin/bash

OPTSTRING=":r:c:o:"
R_FILE=""
CPP_FILE=""
OUTPUT_FILE="scatter.png"
FIGS_DIRECTORY="exp/figs"

while getopts "$OPTSTRING" opt; do
  case "$opt" in
  r) R_FILE="$OPTARG" ;;
  c) CPP_FILE="$OPTARG" ;;
  o) OUTPUT_FILE="$OPTARG" ;;
  *) echo "Error. Invalid argument: $OPT" >&2 ;;
  esac
done

if [[ -z "$R_FILE" ]]; then
  echo "R scores file not provided. Use -r <file>" >&2
  exit 1
fi
if [[ -z "$CPP_FILE" ]]; then
  echo "C++ scores file not provided. Use -c <file>" >&2
  exit 1
fi

mkdir -p "$FIGS_DIRECTORY"
OUTPUT_FILE=$("./scripts/lib/timestamp-filename.sh" "$FIGS_DIRECTORY/$OUTPUT_FILE")

. .venv/bin/activate

PYTHONPATH="scripts/plotting/python" python3 scripts/plotting/python/scatter.py \
  "$R_FILE" \
  "$CPP_FILE" \
  --label1 "R scores" \
  --label2 "C++ scores" \
  --title "R vs C++ NBLAST Score Comparison" \
  --out "$OUTPUT_FILE"
