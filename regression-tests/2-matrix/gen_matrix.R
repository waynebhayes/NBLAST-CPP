#!/usr/bin/env Rscript
# Generate a log-binned NBLAST scoring matrix using nat.nblast.
# Usage: Rscript gen_matrix.R <swc_dir> <known_ids_file> <output_tsv>

options(rgl.useNULL = TRUE)
suppressPackageStartupMessages({
  library(nat)
  library(nat.nblast)
})

args           <- commandArgs(trailingOnly = TRUE)
swc_dir        <- if (length(args) >= 1) args[1] else stop("Usage: gen_matrix.R <swc_dir> <known_ids_file> <output_tsv>")
known_ids_file <- if (length(args) >= 2) args[2] else stop("known_ids_file required")
output_tsv     <- if (length(args) >= 3) args[3] else "r_matrix.tsv"
neurons_cache  <- if (length(args) >= 4) args[4] else NULL
dp_cache       <- if (length(args) >= 5) args[5] else NULL

# ── Load neurons ──────────────────────────────────────────────────────────────

if (!is.null(neurons_cache) && file.exists(neurons_cache)) {
  cat("Loading cached neurons from", neurons_cache, "...\n")
  all_neurons <- get(load(neurons_cache)[1])
} else {
  cat("Reading neurons from", swc_dir, "...\n")
  t_load <- system.time(
    all_neurons <- read.neurons(swc_dir, pattern = "\\.swc$", format = "swc")
  )
  cat(sprintf("Loaded %d neurons in %.1fs\n", length(all_neurons), t_load["elapsed"]))
  if (!is.null(neurons_cache)) save(all_neurons, file = neurons_cache)
}

# ── Compute dotprops ──────────────────────────────────────────────────────────

if (!is.null(dp_cache) && file.exists(dp_cache)) {
  cat("Loading cached dotprops from", dp_cache, "...\n")
  all_dps <- get(load(dp_cache)[1])
} else {
  cat("Computing dotprops (k=5)...\n")
  t_dp <- system.time(
    all_dps <- dotprops(all_neurons, k = 5)
  )
  cat(sprintf("Computed dotprops in %.1fs\n", t_dp["elapsed"]))
  if (!is.null(dp_cache)) save(all_dps, file = dp_cache)
}

cat(sprintf("Total neurons: %d\n", length(all_dps)))

# ── Known-match set ───────────────────────────────────────────────────────────

known_ids  <- readLines(known_ids_file)
known_dps  <- all_dps[intersect(known_ids, names(all_dps))]
cat(sprintf("Known-match set: %d / %d neurons found\n",
            length(known_dps), length(known_ids)))

if (length(known_dps) == 0)
  stop("No known-match neurons found in dataset. Check neuron names.")

# ── Bins (matching smat.fcwb.tsv / nblast++ -r) ──────────────────────────────

distbreaks    <- c(0, 0.75, 1.5, 2, 2.5, 3, 3.5, 4, 5, 6, 7, 8, 9, 10, 12, 14, 16, 20, 25, 30, 40, 500)
dotprodbreaks <- seq(0, 1, by = 0.1)

# ── Build scoring matrix ──────────────────────────────────────────────────────

set.seed(42)
cat("Building scoring matrix...\n")
t_smat <- system.time(
  smat <- create_scoringmatrix(
    matching_neurons    = known_dps,
    nonmatching_neurons = all_dps,
    distbreaks          = distbreaks,
    dotprodbreaks       = dotprodbreaks
  )
)
cat(sprintf("Done in %.1fs\n", t_smat["elapsed"]))

# ── Write TSV (nblast++ format) ───────────────────────────────────────────────

dist_upper    <- distbreaks[-1]
dotprod_upper <- dotprodbreaks[-1]

col_headers <- paste0("cos_", dotprod_upper)
header_line <- paste(c("dist/angle", col_headers), collapse = "\t")

data_lines <- vapply(seq_len(nrow(smat)), function(i) {
  values <- formatC(smat[i, ], format = "f", digits = 4)
  paste(c(sprintf("%.2f", dist_upper[i]), values), collapse = "\t")
}, character(1))

writeLines(c(header_line, data_lines), output_tsv)
cat("Written to", output_tsv, "\n")
