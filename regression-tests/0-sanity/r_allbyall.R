#!/usr/bin/env Rscript
# All-by-all NBLAST for fctraces20 using nat.nblast.
.libPaths(c(path.expand("~/R/library"), .libPaths()))
# Outputs: query <tab> target <tab> score  (one line per pair, no header)
# Usage: Rscript r_allbyall.R <swc_dir> <smat_tsv> <output_tsv>

options(rgl.useNULL = TRUE)
suppressPackageStartupMessages({
  library(nat)
  library(nat.nblast)
})

args       <- commandArgs(trailingOnly = TRUE)
swc_dir    <- if (length(args) >= 1) args[1] else stop("Usage: r_allbyall.R <swc_dir> <smat_tsv> <out_tsv>")
smat_file  <- if (length(args) >= 2) args[2] else stop("smat_tsv required")
out_file   <- if (length(args) >= 3) args[3] else "r_scores.tsv"

cat("Reading SWC files from", swc_dir, "...\n")
neurons <- read.neurons(swc_dir, pattern = "\\.swc$", format = "swc")
cat(sprintf("Loaded %d neurons.\n", length(neurons)))

cat("Computing dotprops (k=5)...\n")
dps <- dotprops(neurons, k = 5)

cat("Loading scoring matrix from", smat_file, "...\n")
tbl <- read.table(smat_file, header = TRUE, sep = "\t", row.names = 1,
                  check.names = FALSE)
colnames(tbl) <- sub("^cos_", "", colnames(tbl))
smat <- as.matrix(tbl)
dist_upper <- as.numeric(rownames(smat))
dp_upper   <- as.numeric(colnames(smat))
attr(smat, "distbreaks")    <- c(0, dist_upper)
attr(smat, "dotprodbreaks") <- c(0, dp_upper)
class(smat) <- c("scoringmatrix", "table")

cat("Running all-by-all...\n")
mat <- nblast_allbyall(dps, smat = smat, normalisation = "mean")

cat("Writing output to", out_file, "...\n")
ids <- rownames(mat)
con <- file(out_file, "w")
for (i in seq_along(ids)) {
  for (j in seq_along(ids)) {
    cat(ids[i], ids[j], mat[i, j], sep = "\t", file = con)
    cat("\n", file = con)
  }
}
close(con)
cat(sprintf("Done. Wrote %d scores.\n", length(ids)^2))
