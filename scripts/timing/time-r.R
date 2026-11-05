#!/usr/bin/env Rscript
# Time nat.nblast all-by-all on a list of neuron IDs.
# Usage: time-r.R <swc_dir> <ids_file> <smat_file> <n> <rep> <out_tsv> [workers] [dump_rds]
# Appends rows: impl n rep phase seconds   (phases: read, dotprops, score)
# workers > 1 splits the query neurons into interleaved chunks scored in forked processes
# (parallel::mclapply). Each worker calls the unmodified nblast(); only the scoring phase
# is parallel. dump_rds, if given, saves the symmetrised score matrix for verification.
options(rgl.useNULL = TRUE)
suppressPackageStartupMessages({ library(nat); library(nat.nblast); library(parallel) })

args <- commandArgs(trailingOnly = TRUE)
if (length(args) < 6) stop("Usage: time-r.R <swc_dir> <ids_file> <smat_file> <n> <rep> <out_tsv>")
swc_dir <- args[1]; ids_file <- args[2]; smat_file <- args[3]
n <- as.integer(args[4]); rep <- as.integer(args[5]); out <- args[6]
workers <- if (length(args) >= 7) as.integer(args[7]) else 1L
dump_rds <- if (length(args) >= 8) args[8] else NULL

ids   <- head(readLines(ids_file), n)
files <- file.path(swc_dir, paste0(ids, ".swc"))

tbl <- read.table(smat_file, header = TRUE, sep = "\t", row.names = 1, check.names = FALSE)
colnames(tbl) <- sub("^cos_", "", colnames(tbl))
smat <- as.matrix(tbl)
attr(smat, "distbreaks")    <- c(0, as.numeric(rownames(smat)))
attr(smat, "dotprodbreaks") <- c(0, as.numeric(colnames(smat)))
class(smat) <- c("scoringmatrix", "table")

t_read <- system.time(nl  <- read.neurons(files, format = "swc"))["elapsed"]
names(nl) <- ids
t_dp   <- system.time(dps <- dotprops(nl, k = 5))["elapsed"]
# same result as nblast++ all-by-all: mean of forward and reverse score, normalised
t_score <- system.time({
  if (workers <= 1) {
    m <- nblast(dps, dps, smat = smat, normalised = TRUE, .progress = "none")
  } else {
    # interleaved chunks balance neuron sizes across workers; columns = query neurons
    chunks <- split(names(dps), rep_len(seq_len(workers), length(dps)))
    parts <- mclapply(chunks, function(ch)
      nblast(dps[ch], dps, smat = smat, normalised = TRUE, .progress = "none"),
      mc.cores = workers)
    m <- do.call(cbind, parts)[, names(dps), drop = FALSE]
  }
  m <- (m + t(m)) / 2
})["elapsed"]
if (!is.null(dump_rds)) saveRDS(m, dump_rds)

rows <- data.frame(impl = if (workers <= 1) "nat.nblast" else sprintf("nat.nblast(%d)", workers), n = n, rep = rep,
                   phase = c("read", "dotprops", "score"),
                   seconds = c(t_read, t_dp, t_score))
write.table(rows, out, sep = "\t", quote = FALSE, row.names = FALSE,
            col.names = !file.exists(out), append = file.exists(out))
