#!/bin/bash
# Time nblast++ vs nat.nblast all-by-all over increasing neuron counts.
# Both implementations get the SAME nested neuron subsets (first N of one seeded shuffle),
# the same scoring matrix. nat.nblast is run unmodified, single-threaded, pinned to
# one core (the baseline). nblast++ is run once per thread count in -T.
#
# Usage: scripts/timing/run-timing.sh [-d swc_dir] [-n "10 50 100"] [-r reps] [-s smat]
#                                     [-c cpu_core] [-S seed] [-o out_dir] [-I impls] [-T "1 8 32"] [-P "1 8 32"]
#   -I  which to run: "cpp r" (default), "cpp", or "r"
#   -P  nat.nblast worker counts (default "1"). 1 = the unmodified single-process baseline;
#       >1 splits query neurons across forked R processes (each still calls plain nblast())
#   -T  nblast++ thread counts (default "1 32"); 1 = single core, directly comparable
#       to the single-core nat.nblast baseline, larger = performant parallel build
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
SWC_DIR="${NBLAST_CPP_QUERY_DATASET_DIRECTORY:-$HOME/research/FlyCircuit}"
SIZES="10 25 50 100 200 400"
REPS=3
SMAT="$REPO_ROOT/regression-tests/0-sanity/smat.fcwb.tsv"
CORE=2
SEED=42
IMPLS="cpp r"
THREADS="1 32"
WORKERS="1"
OUT_DIR="$REPO_ROOT/exp/timing/$(date +%Y%m%d_%H%M%S)"

while getopts "d:n:r:s:c:S:o:I:T:P:" opt; do
  case "$opt" in
  d) SWC_DIR="$OPTARG" ;; n) SIZES="$OPTARG" ;; r) REPS="$OPTARG" ;;
  s) SMAT="$OPTARG" ;; c) CORE="$OPTARG" ;; S) SEED="$OPTARG" ;;
  o) OUT_DIR="$OPTARG" ;; I) IMPLS="$OPTARG" ;; T) THREADS="$OPTARG" ;; P) WORKERS="$OPTARG" ;;
  *) echo "bad option" >&2; exit 1 ;;
  esac
done

mkdir -p "$OUT_DIR"
RESULTS="$OUT_DIR/timings.tsv"
IDS="$OUT_DIR/ids.txt"
MAXN=$(printf '%s\n' $SIZES | sort -n | tail -1)

# NBLAST_BIN: use a prebuilt binary (e.g. a per-host build) instead of building the shared one
if [[ -z "${NBLAST_BIN:-}" ]]; then
  make -C "$REPO_ROOT" release >/dev/null
  NBLAST_BIN="$REPO_ROOT/nblast++"
fi
find "$SWC_DIR" -name '*.swc' -printf '%f\n' | sed 's/\.swc$//' | sort \
  | shuf -n "$MAXN" --random-source=<(yes "$SEED") > "$IDS"

{ echo "host: $(hostname)  cpu: $(lscpu | sed -n 's/^Model name: *//p')  core: $CORE"
  echo "nblast++ threads: $THREADS nat.nblast workers: $WORKERS (unmodified library)"
  echo "sizes: $SIZES  reps: $REPS  seed: $SEED  swc_dir: $SWC_DIR  smat: $SMAT"
  echo "date: $(date -Is)"; } > "$OUT_DIR/meta.txt"
echo -e "impl\tn\trep\tphase\tseconds" > "$RESULTS"
echo "Output: $OUT_DIR  (max N = $MAXN)"

for n in $SIZES; do
  head -n "$n" "$IDS" > "$OUT_DIR/ids_$n.txt"
  for rep in $(seq 1 "$REPS"); do
    if [[ " $IMPLS " == *" cpp "* ]]; then
      for t in $THREADS; do
        if [[ $t -eq 1 ]]; then cpus="$CORE"; else cpus="0-$((t - 1))"; fi
        label="nblast++($t)"
        start=$(date +%s.%N)
        NBLAST_TIMING=1 OMP_NUM_THREADS=$t taskset -c "$cpus" "$NBLAST_BIN" -q "$SMAT" -i "$SWC_DIR" -a \
          < "$OUT_DIR/ids_$n.txt" 2> "$OUT_DIR/cpp_err.tmp" > /dev/null
        total=$(echo "$(date +%s.%N) - $start" | bc -l)
        while IFS=$'\t' read -r _ phase secs; do
          echo -e "$label\t$n\t$rep\t$phase\t$secs" >> "$RESULTS"
        done < <(grep '^TIMING' "$OUT_DIR/cpp_err.tmp")
        echo -e "$label\t$n\t$rep\ttotal\t$total" >> "$RESULTS"
      done
    fi
    if [[ " $IMPLS " == *" r "* ]]; then
      for w in $WORKERS; do
        if [[ $w -eq 1 ]]; then cpus="$CORE"; label="nat.nblast"; else cpus="0-$((w - 1))"; label="nat.nblast($w)"; fi
        r_tmp="$OUT_DIR/r_rows.tmp"; rm -f "$r_tmp"
        start=$(date +%s.%N)
        taskset -c "$cpus" Rscript "$REPO_ROOT/scripts/timing/time-r.R" \
          "$SWC_DIR" "$OUT_DIR/ids_$n.txt" "$SMAT" "$n" "$rep" "$r_tmp" "$w" > /dev/null 2>&1
        total=$(echo "$(date +%s.%N) - $start" | bc -l)   # includes R startup + package load
        tail -n +2 "$r_tmp" >> "$RESULTS"
        echo -e "$label\t$n\t$rep\ttotal\t$total" >> "$RESULTS"
      done
    fi
    echo "  n=$n rep=$rep done"
  done
done
echo "Wrote $RESULTS"
