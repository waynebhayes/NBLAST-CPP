These files are the default matrices (see figure 1G in Costa et al., 2016).
https://github.com/natverse/nat.nblast/blob/v1.6.8/data/smat.fcwb.rda

Then I used R load(smat.fcwb.rda) and sed+awk to convert the R matrix to text
(smat.fcwb.txt), and thence used Unix sed+awk+cut+paste to create a
tab separated file (smat.fcwb.tsv)

We THINK the values in the scoring matrix are log2(p_match/p_rand) for
the dataset mentioned in their paper.
