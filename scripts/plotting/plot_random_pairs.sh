#!/bin/bash

. .venv/bin/activate

python3 exp/tools/scatter.py \
  exp/logbins-r-query-random-pairs-out.tsv \
  exp/logbins-query-random-pairs-out.tsv \
  --label1 "NBLAST" \
  --label2 "NBLAST++" \
  --title "NBLAST vs NBLAST++ Pairwise Score Comparison" \
  --alpha 0.4 \
  --size 10 \
  --out exp/logbins-logbins-random-pairs-scatter.png
