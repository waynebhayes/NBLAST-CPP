#!/bin/bash
# Compare nblast++ and R nat.nblast on random neuron pairs.
# Usage: ./compare.sh [swc_dir] [num_pairs]
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
SCRIPT_DIR="$REPO_ROOT/regression-tests/1-random"
SWC_DIR="${1:-$HOME/research/FlyCircuit}"
NUM_PAIRS="${2:-10000}"
SMAT="$REPO_ROOT/regression-tests/0-sanity/smat.fcwb.tsv"
PAIRS_FILE="$SCRIPT_DIR/random_pairs.tsv"
CPP_OUT="$SCRIPT_DIR/cpp_scores.tsv"
R_OUT="$SCRIPT_DIR/r_scores.tsv"
PLOT_OUT="$SCRIPT_DIR/correlation.png"

# ---------- 1. build nblast++ ----------
echo "==> Building nblast++..."
make -C "$REPO_ROOT" release

# ---------- 2. generate random pairs ----------
echo "==> Generating $NUM_PAIRS random pairs from $SWC_DIR..."
python3 "$REPO_ROOT/scripts/plotting/python/gen_random_pairs.py" \
  --dir "$SWC_DIR" --num-pairs "$NUM_PAIRS" --output "$PAIRS_FILE" --seed 42
echo "    Pairs written to $PAIRS_FILE"

# ---------- 3. run nblast++ ----------
echo "==> Running nblast++..."
{
  echo -e "neuron1\tneuron2\tscore"
  cat "$PAIRS_FILE" | "$REPO_ROOT/nblast++" -q "$SMAT" -i "$SWC_DIR"
} > "$CPP_OUT"
echo "    $(wc -l < "$CPP_OUT") lines written to $CPP_OUT"

# ---------- 4. run R nat.nblast ----------
echo "==> Running R nat.nblast..."
Rscript "$REPO_ROOT/scripts/querying/query-r.R" \
  "$SWC_DIR" "$SWC_DIR" "$PAIRS_FILE" "$SMAT" "$R_OUT"
echo "    $(wc -l < "$R_OUT") lines written to $R_OUT"

# ---------- 5. plot ----------
echo "==> Plotting correlation..."
. "$REPO_ROOT/.venv/bin/activate"
PYTHONPATH="$REPO_ROOT/scripts/plotting/python" python3 \
  "$REPO_ROOT/scripts/plotting/python/scatter.py" \
  "$R_OUT" "$CPP_OUT" \
  --label1 "R nat.nblast" --label2 "nblast++" \
  --title "Random pairs ($NUM_PAIRS): R vs C++" \
  --out "$PLOT_OUT"
echo ""
echo "Plot saved to: $PLOT_OUT"
