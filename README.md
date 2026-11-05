# NBLAST-CPP

The goal of this project is to automate the comparison and matching of fly brain
neurons. NeuronBLAST, otherwise known as NBLAST (not to be confused with
Nucleotide BLAST), is an R package (`nat.nblast`) for scoring pairs of neurons
based upon their morphological similarity. NBLAST works by decomposing a query and
target neuron into short segments. The segments are matched using nearest neighbor
and cosine similarity and scored using a log-likelihood ratio scoring matrix
empirically defined by the statistics of known matches and non-matches.

NBLAST-CPP (`nblast++`) reimplements the core NBLAST methodology in modern C++20,
improving portability, performance, and extensibility. Once validated, it will be
run on the BANC (Brain and Nerve Cord) and FAFB (Full Adult Fly Brain) datasets,
with its results used to seed SANA (Simulated Annealing Network Aligner). SANA then
aligns the datasets together and finds high quality matches in a fraction of the
time.

## Status

- **Core scoring works and tracks R.** On 10,000 random FlyCircuit pairs the scores
  correlate with `nat.nblast` at Pearson r ≈ 0.9998 (max absolute difference 0.06),
  using the same scoring matrix. That comparison used R scores from an earlier run;
  a fresh R re-run is pending. The 20-neuron all-by-all sanity test reproduces R's
  scores (r = 1.0000).
- **Matrix generation works but is still being validated.** Generated matrices
  correlate well with R's (r ≈ 0.9995 on 20k pairs), but differ in bin count and in
  mean score, so training is not yet considered finished.
- **Performance work done.** Scoring is multithreaded (OpenMP), self-scores are
  computed once per neuron, each neuron file is loaded once, and all-by-all exploits
  score symmetry. Output is bit-identical for any thread count.
- **Timing framework exists** (`scripts/timing/`) for comparing against an
  unmodified, single-threaded `nat.nblast`. The full timing runs have not been
  done yet.
- **Not yet done:** running on BANC/FAFB, feeding results to SANA, and paper figures.

## Differences from the original NBLAST

- Nearest-neighbor lookups use nanoflann's KD-tree (instead of `nabor`).
- The tangent vector of a segment is the segment's own direction (the vector
  between a point and its parent). The original estimates it from the 5 nearest
  neighbors (`dotprops(k = 5)`).
- Segments are represented by their midpoints.
- Distance bins can be generated from data (geometric series), or you can use
  R's bins (`-r`) or log bins (`-l`). Optional interpolation between bins is `-I`.

## Building

Requires a C++20 compiler with OpenMP (tested with g++).

```
make release        # builds ./nblast++
make debug          # debug build with logging
make test           # builds and runs ./test_runner (unit tests)
```

## Modes

**Query mode** scores neurons using a scoring matrix.

```
# one query against one or more targets (positional neuron IDs, no .swc extension)
nblast++ -q ScoringMatrix.tsv -i QueryDir,TargetDir QueryID TargetID1 TargetID2

# many (query, target) pairs, read from stdin as "queryID targetID" lines
nblast++ -q ScoringMatrix.tsv -i QueryDir,TargetDir < pairs.tsv

# all-by-all over neuron IDs read from stdin (one per line)
nblast++ -q ScoringMatrix.tsv -i Dir -a < ids.txt
```

**Generator mode** trains a scoring matrix from known matches and random pairs.

```
nblast++ -g KnownMatchesFile,NumIters -i QueryDir,TargetDir [-o out.tsv] [-r | -l]
```

| Flag | Meaning |
| --- | --- |
| `-q matrix` | query mode with the given scoring matrix |
| `-g file,N` | generator mode: known-matches file, N random samples |
| `-i dir[,dir]` | query and target dataset directories (one dir = both) |
| `-o file` | output file for the generated matrix (default stdout) |
| `-a` | all-by-all (query mode, IDs from stdin) |
| `-s` | use sine instead of cosine similarity |
| `-I` | interpolate between matrix bins |
| `-r` / `-l` | use R's bins / log bins when generating a matrix |
| `-S factor` | multiply SWC coordinates by `factor` on load (`0.001` converts nm to µm) |
| `-d` | dump intermediate counts and timing to `out/` |
| `-h` | usage |

Threads: OpenMP is used for loading and scoring. Set `OMP_NUM_THREADS` to control
it (default: all cores). Set `NBLAST_TIMING=1` to print load/score phase timings to
stderr in all-by-all mode.

## Repository layout

```
src/
  app/        Main, Runner (query and generator modes)
  analysis/   Pipeline: all-by-all, pair scoring, matrix generation
  core/       Neuron (KD-tree + scoring), Point, Matrix
  io/         SWC and matrix file I/O
  utils/      argument parsing, logging, timer, nanoflann
unit-tests/        C++ unit tests (built by `make test`)
regression-tests/  comparisons against R nat.nblast
  0-sanity/        all-by-all on 20 FlyCircuit traces (run by run-tests.sh)
  1-random/        10,000 random pairs, C++ vs R
  2-matrix/        matrix trained by C++ vs matrix trained by R
scripts/
  matrix-generating/  generate a matrix with C++ or R
  querying/           score pairs with C++ or R
  test-set-generating/ random neuron lists / pairs
  plotting/           scatter, histogram and matrix comparison plots (Python)
  timing/             nblast++ vs nat.nblast timing harness
run-tests.sh       builds, runs unit tests and the sanity regression (--full: R comparisons)
```

`data/`, `exp/`, `out/`, `legacy-nblast/` and build outputs are git-ignored.
`data/` holds scoring matrices, known-match files and BANC/FAFB annotations;
`exp/` holds experiment outputs and figures; `legacy-nblast/` holds the original R
baseline scripts and timing notes.

## Testing

```
./run-tests.sh          # build, unit tests, sanity regression (fast)
./run-tests.sh --full   # also the two R comparisons below, each checked for Pearson r >= 0.99
```

`run-tests.sh` fails if the unit tests do not produce a pass/fail summary. `--full`
needs R with `nat.nblast` and can take many minutes (the R side dominates). The
two comparisons can also be run alone:

```
bash regression-tests/1-random/compare.sh   # 10k random pairs, C++ vs R
bash regression-tests/2-matrix/compare.sh   # matrix trained by C++ vs by R
```

## Timing against nat.nblast

```
scripts/timing/run-timing.sh -n "10 50 100 200" -r 3 -T "1 8 32" -P "1 8 32"
python3 scripts/timing/summarize-timing.py exp/timing/<run>/timings.tsv plot.png
```

Both implementations score identical, nested neuron subsets with the same matrix.
The `nat.nblast` library is never modified. `-P 1` (default) is the single-process
baseline pinned to one core. `-P N` (N > 1) splits the query neurons across N forked
R processes (`parallel::mclapply`), each calling plain `nblast()`; its scores are
identical to the serial run. `nblast++` is run once per thread count in `-T`. Use
`-T 1` against `-P 1` for the like-for-like comparison, and equal `-T`/`-P` values
for a thread-matched one. Phases reported: load/preprocess, all-by-all scoring, and
end-to-end wall time. Only the scoring phase of `nat.nblast` is parallelised;
reading and `dotprops` stay serial.

## In progress

- validating matrix training (bin counts, score offset vs R)
- full timing runs vs `nat.nblast`
- graphs for the paper
- rigorous regression testing (`--full` exists but has not been run end to end with a fresh R run)
- running on BANC and FAFB, then seeding SANA
