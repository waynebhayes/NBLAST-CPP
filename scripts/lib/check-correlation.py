#!/usr/bin/env python3
"""Exit 0 if the Pearson r between two score files (neuron1 neuron2 score, optional
header) is >= threshold over the pairs they share. Usage: check-correlation.py a.tsv b.tsv min_r"""
import csv, math, sys

def read(path):
    d = {}
    for row in csv.reader(open(path), delimiter="\t"):
        try:
            v = float(row[2])
        except (ValueError, IndexError):
            continue  # header or malformed
        if not math.isnan(v):
            d[(row[0], row[1])] = v
    return d

a, b, min_r = read(sys.argv[1]), read(sys.argv[2]), float(sys.argv[3])
keys = [k for k in a if k in b]
if len(keys) < 2:
    print(f"only {len(keys)} shared pairs"); sys.exit(1)
x, y = [a[k] for k in keys], [b[k] for k in keys]
mx, my = sum(x) / len(x), sum(y) / len(y)
sxy = sum((p - mx) * (q - my) for p, q in zip(x, y))
den = math.sqrt(sum((p - mx) ** 2 for p in x) * sum((q - my) ** 2 for q in y))
r = sxy / den if den else 0.0
print(f"r={r:.5f} over {len(keys)} pairs")
sys.exit(0 if r >= min_r else 1)
