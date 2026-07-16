#!/bin/bash
# Compare nblast++ and R nat.nblast all-by-all scores for fctraces20,
# produce a scatter plot, and (on good correlation) write the .verify file.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
SANITY_DIR="$REPO_ROOT/regression-tests/0-sanity"
SWC_DIR="$REPO_ROOT/regression-tests/fctraces20-swc"
SMAT="$SANITY_DIR/smat.fcwb.tsv"
NEURON_LIST="$SANITY_DIR/fctraces20-test.in"
CPP_OUT="$SANITY_DIR/cpp_scores.tsv"
R_OUT="$SANITY_DIR/r_scores.tsv"
PLOT_OUT="$SANITY_DIR/correlation.png"
VERIFY_FILE="$SANITY_DIR/fctraces20-test.verify"
SCATTER="$REPO_ROOT/scripts/plotting/python/scatter.py"

# ---------- 1. build nblast++ ----------
echo "==> Building nblast++..."
make -C "$REPO_ROOT" clean release

# ---------- 2. run nblast++ all-by-all ----------
echo "==> Running nblast++ all-by-all..."
cat "$NEURON_LIST" | "$REPO_ROOT/nblast++" \
  -q "$SMAT" -a \
  -i "$SWC_DIR" \
  > "$CPP_OUT"
echo "    $(wc -l < "$CPP_OUT") scores written to $CPP_OUT"

# ---------- 3. run R all-by-all ----------
echo "==> Running R nat.nblast all-by-all..."
Rscript --no-init-file "$SANITY_DIR/r_allbyall.R" "$SWC_DIR" "$SMAT" "$R_OUT"
echo "    $(wc -l < "$R_OUT") scores written to $R_OUT"

# ---------- 4. scatter plot ----------
echo "==> Plotting correlation..."
PYTHONPATH="$REPO_ROOT/scripts/plotting/python" python3 "$SCATTER" \
  "$R_OUT" "$CPP_OUT" \
  --label1 "R nat.nblast" --label2 "nblast++" \
  --title "fctraces20 all-by-all: R vs C++" \
  --out "$PLOT_OUT"

# ---------- 5. optionally write .verify ----------
echo ""
echo "Plot saved to: $PLOT_OUT"
echo ""
read -rp "Correlation looks good? Write $VERIFY_FILE? [y/N] " ans
if [[ "${ans,,}" == "y" ]]; then
  cp "$CPP_OUT" "$VERIFY_FILE"
  echo "Written: $VERIFY_FILE"
else
  echo "Skipped writing .verify."
fi
