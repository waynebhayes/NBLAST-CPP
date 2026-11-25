# nat.nblast reference for nblast++ -A: segment midpoints + segment directions
# (nblast++'s model), alpha from nat::dotprops(k), UseAlpha=TRUE, smat_alpha.fcwb.
# Usage (from this dir): Rscript r_alpha.R ../fctraces20-swc 20 smat_alpha.fcwb.tsv r_scores.tsv
options(rgl.useNULL = TRUE)
suppressPackageStartupMessages({ library(nat); library(nat.nblast) })
a <- commandArgs(TRUE); swc_dir <- a[1]; k <- as.integer(a[2])
sm <- smat_alpha.fcwb
up <- function(x) sub(".*,(.*)\\]", "\\1", x)
tsv <- cbind(uperDst = up(rownames(sm)), as.data.frame(unclass(sm)))
names(tsv)[-1] <- paste0("cos_", up(colnames(sm)))
write.table(tsv, a[3], sep = "\t", quote = FALSE, row.names = FALSE)
seg_dps <- function(f) {
  d <- read.table(f, comment.char = "#", col.names = c("id","type","x","y","z","r","parent"))
  ch <- d[d$parent != -1, ]; pa <- d[match(ch$parent, d$id), ]
  P <- as.matrix(ch[, c("x","y","z")]); Q <- as.matrix(pa[, c("x","y","z")])
  dp <- dotprops((P + Q) / 2, k = k, .progress = "none")
  v <- Q - P; dp$vect <- v / sqrt(rowSums(v^2))
  dp
}
fs <- list.files(swc_dir, pattern = "\\.swc$", full.names = TRUE)
dps <- as.neuronlist(lapply(fs, seg_dps)); names(dps) <- sub("\\.swc$", "", basename(fs))
M <- nblast(dps, dps, normalised = TRUE, UseAlpha = TRUE, smat = sm, .progress = "none")
S <- (M + t(M)) / 2
out <- data.frame(q = rep(colnames(S), each = nrow(S)), t = rep(rownames(S), ncol(S)), r = as.vector(S))
write.table(out, a[4], sep = "\t", quote = FALSE, row.names = FALSE)
