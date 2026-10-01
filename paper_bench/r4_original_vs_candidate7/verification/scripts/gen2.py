#!/usr/bin/env python3
"""Extra families: large translations (curves far apart), and the reviewer's constructions."""
import os, random, sys
sys.path.insert(0, "/root/verif")
from gen import fam_uniform
OUT = sys.argv[1]
rng = random.Random(77)
fams = [
    ("far_t_1e7", 800, lambda r: (fam_uniform(r, s=10.0)[0], fam_uniform(r, s=10.0, off=(1.36e7, 4.5e6))[1])),
    ("far_t_1e8", 800, lambda r: (fam_uniform(r, s=10.0)[0], fam_uniform(r, s=10.0, off=(1.0e8, -3.0e7))[1])),
    ("far_t_sig", 800, lambda r: (fam_uniform(r, s=1000.0, off=(1.36e7, 4.5e6))[0], fam_uniform(r, s=1000.0, off=(1.30e7, 4.6e6))[1])),
]
os.makedirs(OUT, exist_ok=True)
ol = open(os.path.join(OUT, "oracle_list.txt"), "w")
for name, cnt, g in fams:
    d = os.path.join(OUT, name); os.makedirs(d, exist_ok=True)
    pf = open(os.path.join(OUT, f"{name}.pairs"), "w")
    for i in range(cnt):
        P, Q = g(rng)
        a, b = f"{i:05d}_a.txt", f"{i:05d}_b.txt"
        for fn, C in ((a, P), (b, Q)):
            with open(os.path.join(d, fn), "w") as f:
                for x, y in C: f.write(f"{x!r} {y!r}\n")
        pf.write(f"{a} {b}\n"); ol.write(f"{name}/{i:05d} {os.path.join(d, a)} {os.path.join(d, b)}\n")
    pf.close()
# reviewer constructions (finding 1: calcDistance kd-tree bound; finding 5: zero-width initial box)
d = os.path.join(OUT, "review"); os.makedirs(d, exist_ok=True)
cases = {
    "00000": ([(0, 0), (0, -0.3), (0, 1.5), (0.1, 0)], [(0, 0)] * 4),
    "00001": ([(0, 0), (2, 0)], [(0, 0), (0, 0)]),
    "00002": ([(0, 0), (2, 0)], [(0, 0)]),
}
pf = open(os.path.join(OUT, "review.pairs"), "w")
for i, (P, Q) in cases.items():
    for fn, C in ((f"{i}_a.txt", P), (f"{i}_b.txt", Q)):
        with open(os.path.join(d, fn), "w") as f:
            for x, y in C: f.write(f"{float(x)!r} {float(y)!r}\n")
    pf.write(f"{i}_a.txt {i}_b.txt\n"); ol.write(f"review/{i} {os.path.join(d, i + '_a.txt')} {os.path.join(d, i + '_b.txt')}\n")
pf.close(); ol.close()
