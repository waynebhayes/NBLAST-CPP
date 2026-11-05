#!/usr/bin/env python3
"""Speedup vs thread count from run-timing.sh output.
speedup(n) = median time at 1 thread / median time at n threads, for a fixed neuron count N.

Curves (per phase):
  nblast++ (n) vs nblast++ (1)
  nblast++ (n) vs nat.nblast (1)      <- the single-threaded R baseline
  nat.nblast (n workers) vs nat.nblast (1), if run with -P
  ideal (y = x)

Usage: plot-speedup.py <timings.tsv> [out.png] [-n N]
  -n  neuron count to use (default: largest in the file)"""
import sys, csv, re, statistics as st
from collections import defaultdict

args = sys.argv[1:]
N = None
if "-n" in args:
    i = args.index("-n"); N = int(args[i + 1]); del args[i:i + 2]
path = args[0]
out = args[1] if len(args) > 1 else None

d = defaultdict(list)
for r in csv.DictReader(open(path), delimiter="\t"):
    d[(r["impl"], int(r["n"]), r["phase"])].append(float(r["seconds"]))
med = {k: st.median(v) for k, v in d.items()}
N = N or max(k[1] for k in med)

def threads(impl, prefix):
    m = re.fullmatch(re.escape(prefix) + r"\((\d+)\)", impl)
    if m: return int(m.group(1))
    return 1 if impl == prefix else None

def series(prefix, phase):
    pts = {}
    for (impl, n, ph), v in med.items():
        t = threads(impl, prefix)
        if n == N and ph == phase and t is not None: pts[t] = v
    return dict(sorted(pts.items()))

cpp = lambda ph: series("nblast++", ph)
r = lambda ph: series("nat.nblast", ph)

phases = (("score", "All-by-all scoring only"), ("total", "End to end"))
curves = {}
for ph, title in phases:
    c, rr = cpp(ph), r(ph)
    rows = []
    if 1 in c:
        rows.append(("nblast++ (n) vs nblast++ (1)", {t: c[1] / v for t, v in c.items()}))
    if 1 in rr and c:
        rows.append(("nblast++ (n) vs nat.nblast (1)", {t: rr[1] / v for t, v in c.items()}))
    if 1 in rr and len(rr) > 1:
        rows.append(("nat.nblast (n) vs nat.nblast (1)", {t: rr[1] / v for t, v in rr.items()}))
    curves[ph] = rows
    print(f"\n{title} (N={N}): speedup")
    ts = sorted({t for _, s in rows for t in s})
    print(f"{'threads':>8} | " + " | ".join(f"{n:>34}" for n, _ in rows))
    for t in ts:
        print(f"{t:>8} | " + " | ".join(f"{s[t]:>34.2f}" if t in s else f"{'-':>34}" for _, s in rows))

if out:
    import matplotlib; matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    fig, ax = plt.subplots(1, 2, figsize=(12, 5))
    for a, (ph, title) in zip(ax, phases):
        for name, s in curves[ph]:
            a.plot(list(s), list(s.values()), "o-", label=name)
        mx = max([t for _, s in curves[ph] for t in s] or [1])
        a.plot([1, mx], [1, mx], "k--", lw=1, label="ideal")
        a.set(xscale="log", yscale="log", xlabel="threads", ylabel="speedup = T(1 thread) / T(n threads)",
              title=f"{title} (N={N})")
        ts = sorted({t for _, s in curves[ph] for t in s})
        a.set_xticks(ts); a.set_xticklabels(ts)
        a.grid(alpha=.3, which="both"); a.legend()
    fig.tight_layout(); fig.savefig(out, dpi=150)
    print("plot:", out)
