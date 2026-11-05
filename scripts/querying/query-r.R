#!/usr/bin/env Rscript

options(rgl.useNULL = TRUE)
suppressPackageStartupMessages({
  library(nat)
  library(nat.nblast)
})

args <- commandArgs(trailingOnly = TRUE)
query_swc_dir   <- if (length(args) >= 1) args[1] else stop("Usage: query-r.R <query_dir> <target_dir> <pairs_file> <smat_file> <output_file> [cache_dir]")
target_swc_dir  <- if (length(args) >= 2) args[2] else query_swc_dir
pairs_file      <- if (length(args) >= 3) args[3] else stop("pairs_file required")
smat_file       <- if (length(args) >= 4) args[4] else "smat.rda"
output_file     <- if (length(args) >= 5) args[5] else "out/r-query-out.tsv"
# dotprops are cached here, keyed by (directory, set of neuron IDs), so changing
# the pairs file can never serve stale data. Use "none" to disable caching.
cache_dir       <- if (length(args) >= 6) args[6] else file.path(Sys.getenv("HOME"), ".cache", "nblast-cpp-r")

cat("Query dataset:  ", query_swc_dir, "\n")
cat("Target dataset: ", target_swc_dir, "\n")
cat("Pairs file:     ", pairs_file, "\n")
cat("Scoring matrix: ", smat_file, "\n")
cat("Output:         ", output_file, "\n")
cat("Cache dir:      ", cache_dir, "\n")

same_dir <- normalizePath(query_swc_dir) == normalizePath(target_swc_dir)

# ── Read and filter pairs (first, so only the neurons we need get loaded) ─────

pairs <- read.table(pairs_file, header = FALSE, sep = "",
                    col.names = c("neuron1", "neuron2"),
                    stringsAsFactors = FALSE, fill = TRUE)
pairs <- pairs[!is.na(pairs$neuron1) & pairs$neuron1 != "NA" &
               !is.na(pairs$neuron2) & pairs$neuron2 != "NA", ]
cat(sprintf("Pairs to score: %d\n", nrow(pairs)))

# ── Load neurons + compute dotprops for only the IDs in the pairs ─────────────

load_dotprops <- function(swc_dir, ids, label) {
  ids <- sort(unique(ids))
  files <- file.path(swc_dir, paste0(ids, ".swc"))
  keep <- file.exists(files)
  ids <- ids[keep]; files <- files[keep]

  cache_file <- NULL
  if (cache_dir != "none") {
    dir.create(cache_dir, recursive = TRUE, showWarnings = FALSE)
    key_file <- tempfile()
    writeLines(c(normalizePath(swc_dir), ids), key_file)
    cache_file <- file.path(cache_dir, paste0("dp-", unname(tools::md5sum(key_file)), ".rda"))
    unlink(key_file)
    if (file.exists(cache_file)) {
      cat(sprintf("Loading cached %s dotprops (%d neurons) from %s\n", label, length(ids), cache_file))
      load(cache_file)  # -> dps
      return(dps)
    }
  }

  cat(sprintf("Reading %d %s neurons from %s ...\n", length(ids), label, swc_dir))
  t_load <- system.time(nl <- read.neurons(files, format = "swc"))
  names(nl) <- ids
  cat(sprintf("  loaded in %.1fs\n", t_load["elapsed"]))
  cat(sprintf("Computing %s dotprops (k=5)...\n", label))
  t_dp <- system.time(dps <- dotprops(nl, k = 5))
  cat(sprintf("  computed in %.1fs\n", t_dp["elapsed"]))
  if (!is.null(cache_file)) save(dps, file = cache_file)
  dps
}

if (same_dir) {
  # one set covering both columns, so neurons shared between them load once
  query_dps <- load_dotprops(query_swc_dir, c(pairs$neuron1, pairs$neuron2), "query+target")
  target_dps <- query_dps
} else {
  query_dps <- load_dotprops(query_swc_dir, pairs$neuron1, "query")
  target_dps <- load_dotprops(target_swc_dir, pairs$neuron2, "target")
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
