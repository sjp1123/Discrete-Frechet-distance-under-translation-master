#!/usr/bin/env python3
# Pick 100 pairs of real Geolife trajectories whose vertex count n is in [100,1000].
import os, random

DATA = "original/test_data/benchmark/Geolife Trajectories 1.3/data"
OUT  = "test_cases/geolife_100"
os.makedirs(OUT, exist_ok=True)

def line_count(path):
    with open(path, "rb") as f:
        data = f.read()
    # count non-empty records (CRLF or LF); each line = one lat/lon vertex
    return data.count(b"\n")

# scan all curve files, keep those with n in [100,1000]
inrange = []
for fn in os.listdir(DATA):
    if not fn.endswith(".txt"):
        continue
    n = line_count(os.path.join(DATA, fn))
    if 100 <= n <= 1000:
        inrange.append((fn, n))

print(f"files with n in [100,1000]: {len(inrange)}")

random.seed(24680)
random.shuffle(inrange)

need = 200  # 100 pairs
sel = inrange[:need]
if len(sel) < need:
    raise SystemExit(f"only {len(sel)} in-range files, need {need}")

man = open(f"{OUT}/manifest.txt", "w")
for i in range(100):
    (f1, n1) = sel[2*i]
    (f2, n2) = sel[2*i+1]
    nrep = max(n1, n2)            # bucket label = harder (longer) curve
    man.write(f"{nrep} {f1} {f2}\n")
man.close()
print(f"wrote 100 pairs to {OUT}/manifest.txt")
