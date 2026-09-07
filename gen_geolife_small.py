#!/usr/bin/env python3
# Pick 100 pairs of REAL Geolife trajectories with vertex count n in [10, 200].
import os, random

DATA = "original/test_data/benchmark/Geolife Trajectories 1.3/data"
OUT  = "test_cases/geolife_small"
os.makedirs(OUT, exist_ok=True)

def line_count(path):
    with open(path, "rb") as f:
        return f.read().count(b"\n")

inrange = []
for fn in os.listdir(DATA):
    if not fn.endswith(".txt"):
        continue
    n = line_count(os.path.join(DATA, fn))
    if 10 <= n <= 200:
        inrange.append((fn, n))

print(f"Geolife files with n in [10,200]: {len(inrange)}")
random.seed(11235)
random.shuffle(inrange)
sel = inrange[:200]
if len(sel) < 200:
    raise SystemExit(f"only {len(sel)} in-range files")

man = open(f"{OUT}/manifest.txt", "w")
for i in range(100):
    (f1, n1) = sel[2*i]; (f2, n2) = sel[2*i+1]
    man.write(f"{max(n1,n2)} {f1} {f2}\n")
man.close()
print(f"wrote 100 pairs to {OUT}/manifest.txt")
