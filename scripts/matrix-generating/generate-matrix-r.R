#!/usr/bin/env Rscript

options(rgl.useNULL = TRUE)
suppressPackageStartupMessages({
  library(nat)
  library(nat.nblast)
})

distbreaks    <- c(0, 0.25, 0.50, 1.00, 2.00, 4.00, 8.00, 16.00, 32.00, 64.00)
dotprodbreaks <- c(0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0)
args <- commandArgs(trailingOnly = TRUE)
swc_dir <- if (length(args) >= 1) args[1]
known_ids_file <- if (length(args) >= 2) args[2]
output_file <- if (length(args) >= 3) args[3] else "out.tsv"
neurons_cache <- if (length(args) >= 4) args[4] else "neurons-cache.rda"
dp_cache <- if (length(args) >= 5) args[5] else "dp-cache.rda"

print(swc_dir)
print(known_ids_file)
print(output_file)
print(neurons_cache)
print(dp_cache)

if (file.exists(neurons_cache)) {
  cat("Loading cached neurons from", neurons_cache, "...\n")
  load(neurons_cache)
} else {
  cat("Reading neurons from", swc_dir, "...\n")
  t_load <- system.time(
    neuronlist <- read.neurons(swc_dir, pattern = "\\.swc$", format = "swc")
  )
  cat(sprintf("Loaded %d neurons in %.1fs\n", length(neuronlist), t_load["elapsed"]))
  save(neuronlist, file = neurons_cache)
}

if (file.exists(dp_cache)) {
  cat("Loading cached dotprops from", dp_cache, "...\n")
  load(dp_cache)
} else {
  cat("Computing dotprops (k=5)...\n")
  t_dp <- system.time(
    dotprops <- dotprops(neuronlist, k = 5)
  )
  cat(sprintf("Computed dotprops in %.1fs\n", t_dp["elapsed"]))
  save(dotprops, file = dp_cache)
}

cat(sprintf("Total neurons available: %d\n", length(dotprops)))

known_ids <- readLines(known_ids_file)
known_dps <- dotprops[intersect(known_ids, names(dotprops))]
cat(sprintf("Known-match set: %d / %d neurons found\n",
            length(known_dps), length(known_ids)))

if (length(known_dps) == 0)
  stop("No neurons from the known id set given found in the dataset. Check neuron names.")


set.seed(42)
cat("Building scoring matrix...\n")
t_smat <- system.time(
  smat <- create_scoringmatrix(
    matching_neurons    = known_dps,
    nonmatching_neurons = dotprops,
    distbreaks          = distbreaks,
  )
)
cat(sprintf("Done in %.1fs\n", t_smat["elapsed"]))

# ── Save .rda ─────────────────────────────────────────────────────────────────

save(smat, file = "smat.rda")
cat("Saved R object to legacy-nblast/log_smat.rda\n")

# ── Write TSV (nblast++ format) ───────────────────────────────────────────────
# Row labels  : upper bound of each distance interval (numeric)
# Column labels: cos_0.1 … cos_1

dist_upper    <- distbreaks[-1]                    # 0.25 … 64
dotprod_upper <- dotprodbreaks[-1]                 # 0.1  … 1.0

col_headers <- paste0("cos_", dotprod_upper)
header_line <- paste(c("dist/angle", col_headers), collapse = "\t")

data_lines <- vapply(seq_len(nrow(smat)), function(i) {
  values <- formatC(smat[i, ], format = "f", digits = 4)
  paste(c(sprintf("%.2f", dist_upper[i]), values), collapse = "\t")
}, character(1))

writeLines(c(header_line, data_lines), output_file)
cat("Written TSV to", output_file, "\n")
