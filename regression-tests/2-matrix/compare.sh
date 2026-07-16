#!/bin/bash
# Generate scoring matrices with nblast++ and R nat.nblast, score a shared
# random pair set with each, then compare both the matrices and the scores.
# Usage: ./compare.sh [swc_dir] [num_random_iters] [num_score_pairs]
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
SCRIPT_DIR="$REPO_ROOT/regression-tests/2-matrix"
SWC_DIR="${1:-$HOME/research/FlyCircuit}"
NUM_ITERS="${2:-10000}"
NUM_PAIRS="${3:-10000}"
KNOWN_IDS="$REPO_ROOT/data/matches/fc-dl2.tsv"
CPP_MATRIX="$SCRIPT_DIR/cpp_matrix.tsv"
R_MATRIX="$SCRIPT_DIR/r_matrix.tsv"
PAIRS_FILE="$SCRIPT_DIR/random_pairs.tsv"
CPP_SCORES="$SCRIPT_DIR/cpp_scores.tsv"
R_SCORES="$SCRIPT_DIR/r_scores.tsv"
MATRIX_PLOT="$SCRIPT_DIR/matrix_correlation.png"
SCORE_PLOT="$SCRIPT_DIR/score_correlation.png"

# ── 1. build nblast++ ─────────────────────────────────────────────────────────
echo "==> Building nblast++..."
make -C "$REPO_ROOT" release

# ── 2. generate matrix with nblast++ ─────────────────────────────────────────
echo "==> Running nblast++ matrix generation ($NUM_ITERS random iters)..."
"$REPO_ROOT/nblast++" \
  -g "$KNOWN_IDS,$NUM_ITERS" \
  -r \
  -i "$SWC_DIR" \
  -o "$CPP_MATRIX"
echo "    Written to $CPP_MATRIX"

# ── 3. generate matrix with R nat.nblast ──────────────────────────────────────
echo "==> Running R nat.nblast matrix generation..."
Rscript "$SCRIPT_DIR/gen_matrix.R" \
  "$SWC_DIR" \
  "$KNOWN_IDS" \
  "$R_MATRIX" \
  "$REPO_ROOT/neurons-cache.rda" \
  "$REPO_ROOT/dp-cache.rda"
echo "    Written to $R_MATRIX"

# ── 4. generate random pairs ──────────────────────────────────────────────────
echo "==> Generating $NUM_PAIRS random pairs..."
python3 "$REPO_ROOT/scripts/plotting/python/gen_random_pairs.py" \
  --dir "$SWC_DIR" --num-pairs "$NUM_PAIRS" --output "$PAIRS_FILE" --seed 42
echo "    Written to $PAIRS_FILE"

# ── 5. score pairs with nblast++ (using cpp matrix) ──────────────────────────
echo "==> Scoring pairs with nblast++ (cpp matrix)..."
{
  echo -e "neuron1\tneuron2\tscore"
  cat "$PAIRS_FILE" | "$REPO_ROOT/nblast++" -q "$CPP_MATRIX" -i "$SWC_DIR"
} > "$CPP_SCORES"
echo "    $(wc -l < "$CPP_SCORES") lines written to $CPP_SCORES"

# ── 6. score pairs with R (using R matrix) ───────────────────────────────────
echo "==> Scoring pairs with R nat.nblast (R matrix)..."
Rscript "$REPO_ROOT/scripts/querying/query-r.R" \
  "$SWC_DIR" "$SWC_DIR" "$PAIRS_FILE" "$R_MATRIX" "$R_SCORES" \
  "$REPO_ROOT/query-neurons-cache.rda" \
  "$REPO_ROOT/query-dp-cache.rda"
echo "    $(wc -l < "$R_SCORES") lines written to $R_SCORES"

. "$REPO_ROOT/.venv/bin/activate"

# ── 7. plot matrix comparison ─────────────────────────────────────────────────
echo "==> Plotting matrix cell comparison..."
PYTHONPATH="$REPO_ROOT/scripts/plotting/python" python3 \
  "$REPO_ROOT/scripts/plotting/python/scatter.py" \
  "$R_MATRIX" "$CPP_MATRIX" \
  --matrix \
  --label1 "R nat.nblast matrix" --label2 "nblast++ matrix" \
  --title "Scoring matrix: R vs C++" \
  --out "$MATRIX_PLOT"

# ── 8. plot score comparison ──────────────────────────────────────────────────
echo "==> Plotting score comparison..."
PYTHONPATH="$REPO_ROOT/scripts/plotting/python" python3 \
  "$REPO_ROOT/scripts/plotting/python/scatter.py" \
  "$R_SCORES" "$CPP_SCORES" \
  --label1 "R nat.nblast scores" --label2 "nblast++ scores" \
  --title "Scores ($NUM_PAIRS pairs): R matrix vs C++ matrix" \
  --out "$SCORE_PLOT"

echo ""
echo "Matrix plot: $MATRIX_PLOT"
echo "Score plot:  $SCORE_PLOT"
