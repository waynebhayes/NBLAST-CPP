#!/bin/bash
# All-by-all with alpha weighting (-A 20, smat_alpha.fcwb) on fctraces20.
# The .verify scores match nat.nblast UseAlpha=TRUE (r_scores.tsv, from
# r_alpha.R) to within 5e-6.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
ALPHA_DIR="$REPO_ROOT/regression-tests/3-alpha"
SWC_DIR="$REPO_ROOT/regression-tests/fctraces20-swc"
MATRIX="$ALPHA_DIR/smat_alpha.fcwb.tsv"
INPUT_FILE="$ALPHA_DIR/fctraces20-alpha-test.in"
OUTPUT_FILE="$ALPHA_DIR/fctraces20-alpha-test.out"
VERIFY_FILE="$ALPHA_DIR/fctraces20-alpha-test.verify"

cat "$INPUT_FILE" | "$REPO_ROOT/nblast++" -a -A 20 -q "$MATRIX" -i "$SWC_DIR" > "$OUTPUT_FILE"
diff "$VERIFY_FILE" "$OUTPUT_FILE"
