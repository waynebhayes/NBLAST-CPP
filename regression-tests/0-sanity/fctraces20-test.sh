#!/bin/bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
SANITY_DIR="$REPO_ROOT/regression-tests/0-sanity"
SWC_DIR="$REPO_ROOT/regression-tests/fctraces20-swc"
MATRIX="$SANITY_DIR/smat.fcwb.tsv"
INPUT_FILE="$SANITY_DIR/fctraces20-test.in"
OUTPUT_FILE="$SANITY_DIR/fctraces20-test.out"
VERIFY_FILE="$SANITY_DIR/fctraces20-test.verify"

cat "$INPUT_FILE" | "$REPO_ROOT/nblast++" -a -q "$MATRIX" -i "$SWC_DIR" > "$OUTPUT_FILE"
diff "$VERIFY_FILE" "$OUTPUT_FILE"

