#!/usr/bin/env Rscript

options(rgl.useNULL = TRUE)
suppressPackageStartupMessages({
  library(nat)
  library(nat.nblast)
})

args <- commandArgs(trailingOnly = TRUE)
query_swc_dir   <- if (length(args) >= 1) args[1] else stop("Usage: query-r.R <query_dir> <target_dir> <pairs_file> <smat_file> <output_file>")
target_swc_dir  <- if (length(args) >= 2) args[2] else query_swc_dir
pairs_file      <- if (length(args) >= 3) args[3] else stop("pairs_file required")
smat_file       <- if (length(args) >= 4) args[4] else "smat.rda"
output_file     <- if (length(args) >= 5) args[5] else "out/r-query-out.tsv"
q_neurons_cache <- if (length(args) >= 6) args[6] else "query-neurons-cache.rda"
q_dp_cache      <- if (length(args) >= 7) args[7] else "query-dp-cache.rda"
t_neurons_cache <- if (length(args) >= 8) args[8] else "target-neurons-cache.rda"
t_dp_cache      <- if (length(args) >= 9) args[9] else "target-dp-cache.rda"

cat("Query dataset:  ", query_swc_dir, "\n")
cat("Target dataset: ", target_swc_dir, "\n")
cat("Pairs file:     ", pairs_file, "\n")
cat("Scoring matrix: ", smat_file, "\n")
cat("Output:         ", output_file, "\n")

same_dir <- normalizePath(query_swc_dir) == normalizePath(target_swc_dir)

# ── Load query neurons ────────────────────────────────────────────────────────

if (file.exists(q_neurons_cache)) {
  cat("Loading cached query neurons from", q_neurons_cache, "...\n")
  load(q_neurons_cache)
} else {
  cat("Reading query neurons from", query_swc_dir, "...\n")
  t_load <- system.time(
    query_neuronlist <- read.neurons(query_swc_dir, pattern = "\\.swc$", format = "swc")
  )
  cat(sprintf("Loaded %d query neurons in %.1fs\n", length(query_neuronlist), t_load["elapsed"]))
  save(query_neuronlist, file = q_neurons_cache)
}

# ── Load target neurons ───────────────────────────────────────────────────────

if (same_dir) {
  cat("Target dataset is the same as query — reusing neurons.\n")
  target_neuronlist <- query_neuronlist
} else if (file.exists(t_neurons_cache)) {
  cat("Loading cached target neurons from", t_neurons_cache, "...\n")
  load(t_neurons_cache)
} else {
  cat("Reading target neurons from", target_swc_dir, "...\n")
  t_load <- system.time(
    target_neuronlist <- read.neurons(target_swc_dir, pattern = "\\.swc$", format = "swc")
  )
  cat(sprintf("Loaded %d target neurons in %.1fs\n", length(target_neuronlist), t_load["elapsed"]))
  save(target_neuronlist, file = t_neurons_cache)
}

# ── Compute query dotprops ────────────────────────────────────────────────────

if (file.exists(q_dp_cache)) {
  cat("Loading cached query dotprops from", q_dp_cache, "...\n")
  load(q_dp_cache)
} else {
  cat("Computing query dotprops (k=5)...\n")
  t_dp <- system.time(
    query_dps <- dotprops(query_neuronlist, k = 5)
  )
  cat(sprintf("Computed query dotprops in %.1fs\n", t_dp["elapsed"]))
  save(query_dps, file = q_dp_cache)
}

# ── Compute target dotprops ───────────────────────────────────────────────────

if (same_dir) {
  cat("Target dotprops same as query — reusing.\n")
  target_dps <- query_dps
} else if (file.exists(t_dp_cache)) {
  cat("Loading cached target dotprops from", t_dp_cache, "...\n")
  load(t_dp_cache)
} else {
  cat("Computing target dotprops (k=5)...\n")
  t_dp <- system.time(
    target_dps <- dotprops(target_neuronlist, k = 5)
  )
  cat(sprintf("Computed target dotprops in %.1fs\n", t_dp["elapsed"]))
  save(target_dps, file = t_dp_cache)
}

cat(sprintf("Query neurons:  %d\n", length(query_dps)))
cat(sprintf("Target neurons: %d\n", length(target_dps)))

# ── Load scoring matrix ───────────────────────────────────────────────────────

cat("Loading scoring matrix from", smat_file, "...\n")
if (grepl("\\.(rda|Rda|RData)$", smat_file, ignore.case = TRUE)) {
  load(smat_file)  # -> smat
} else {
  tbl <- read.table(smat_file, header = TRUE, sep = "\t", row.names = 1,
                    check.names = FALSE)
  colnames(tbl) <- sub("^cos_", "", colnames(tbl))
  smat <- as.matrix(tbl)
  dist_upper <- as.numeric(rownames(smat))
  dp_upper   <- as.numeric(colnames(smat))
  attr(smat, "distbreaks")    <- c(0, dist_upper)
  attr(smat, "dotprodbreaks") <- c(0, dp_upper)
  class(smat) <- c("scoringmatrix", "table")
}
cat("Loaded scoring matrix.\n")

# ── Read and filter pairs ─────────────────────────────────────────────────────

pairs <- read.table(pairs_file, header = FALSE, sep = "",
                    col.names = c("neuron1", "neuron2"),
                    stringsAsFactors = FALSE, fill = TRUE)
pairs <- pairs[!is.na(pairs$neuron1) & pairs$neuron1 != "NA" &
               !is.na(pairs$neuron2) & pairs$neuron2 != "NA", ]
cat(sprintf("Pairs to score: %d\n", nrow(pairs)))

# ── Score pairs ───────────────────────────────────────────────────────────────

cat("Scoring pairs...\n")
t_query <- system.time({
  scores <- mapply(function(q_id, t_id) {
    if (!q_id %in% names(query_dps) || !t_id %in% names(target_dps)) return(NA_real_)
    forward  <- as.numeric(nblast(query_dps[q_id], target_dps[t_id], smat = smat, normalised = TRUE))[[1]]
    reverse  <- as.numeric(nblast(target_dps[t_id], query_dps[q_id], smat = smat, normalised = TRUE))[[1]]
    (forward + reverse) / 2
  }, pairs$neuron1, pairs$neuron2)
})
cat(sprintf("Scored %d pairs in %.1fs\n", nrow(pairs), t_query["elapsed"]))

n_na <- sum(is.na(scores))
if (n_na > 0)
  cat(sprintf("Warning: %d pairs had missing neurons and were scored as NA\n", n_na))

# ── Write output ──────────────────────────────────────────────────────────────

result <- data.frame(
  neuron1 = pairs$neuron1,
  neuron2 = pairs$neuron2,
  score   = scores,
  stringsAsFactors = FALSE
)
write.table(result, output_file, sep = "\t", quote = FALSE, row.names = FALSE)
cat("Written output to", output_file, "\n")
