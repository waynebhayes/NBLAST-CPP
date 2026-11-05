#!/usr/bin/env python3
"""Summarize timings.tsv: median seconds per (impl, n, phase) and speedup vs nat.nblast.
Usage: summarize-timing.py <timings.tsv> [out.png]"""
import sys, csv, statistics as st
from collections import defaultdict

BASE = "nat.nblast"
rows = list(csv.DictReader(open(sys.argv[1]), delimiter="\t"))
d = defaultdict(list)
for r in rows:
    d[(r["impl"], int(r["n"]), r["phase"])].append(float(r["seconds"]))
med = {k: st.median(v) for k, v in d.items()}
ns = sorted({k[1] for k in med})
impls = sorted({k[0] for k in med if k[0] != BASE}, key=lambda s: (len(s), s)) + [BASE]

for phase, title in (("score", "All-by-all scoring only"), ("total", "End to end (load + preprocess + score)")):
    print(f"\n{title}: median seconds (speedup vs {BASE})")
    print(f"{'n':>6} {'pairs':>9} | " + " ".join(f"{i:>20}" for i in impls))
    for n in ns:
        base = med.get((BASE, n, phase))
        cells = []
        for i in impls:
            v = med.get((i, n, phase))
            if v is None: cells.append(f"{'-':>20}")
            elif i == BASE or not base: cells.append(f"{v:>20.2f}")
            else: cells.append(f"{v:>11.2f} ({base / v:>5.1f}x)")
        print(f"{n:>6} {n*n:>9} | " + " ".join(cells))

if len(sys.argv) > 2:
    import matplotlib; matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    fig, ax = plt.subplots(1, 2, figsize=(11, 4.5))
    for i in impls:
        for a, ph in zip(ax, ("score", "total")):
            pts = [(n, med[(i, n, ph)]) for n in ns if (i, n, ph) in med]
            if pts: a.plot(*zip(*pts), "o-", label=i)
    for a, t in zip(ax, ("All-by-all scoring only", "End to end (load + preprocess + score)")):
        a.set(xscale="log", yscale="log", xlabel="neurons (N)", ylabel="seconds (median)", title=t)
        a.grid(alpha=.3, which="both"); a.legend()
    fig.tight_layout(); fig.savefig(sys.argv[2], dpi=150)
    print("plot:", sys.argv[2])
