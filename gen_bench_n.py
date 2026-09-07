#!/usr/bin/env python3
import os, math, random

OUT = "test_cases/bench_n_sweep"
os.makedirs(OUT, exist_ok=True)

# n values from 100 to 1000, 2 pairs each
NS = [100, 200, 400, 600, 800, 1000]
PAIRS = 2
random.seed(2026)

def random_walk(n, step=1.0):
    x, y = 0.0, 0.0
    pts = [(x, y)]
    ang = random.uniform(0, 2*math.pi)
    for _ in range(n-1):
        ang += random.gauss(0, 0.4)            # smoothly turning walk
        x += step*math.cos(ang); y += step*math.sin(ang)
        pts.append((x, y))
    return pts

def perturb(pts, dtheta, tx, ty, noise):
    c, s = math.cos(dtheta), math.sin(dtheta)
    out = []
    for (x, y) in pts:
        rx = c*x - s*y + tx + random.gauss(0, noise)
        ry = s*x + c*y + ty + random.gauss(0, noise)
        out.append((rx, ry))
    return out

def write(path, pts):
    with open(path, "w") as f:
        for (x, y) in pts:
            f.write(f"{x:.6f} {y:.6f}\n")

for n in NS:
    for p in range(PAIRS):
        a = random_walk(n)
        # b: small rotation + translation + noise -> nontrivial translation search
        b = perturb(a, random.uniform(-0.15, 0.15),
                    random.uniform(-3, 3), random.uniform(-3, 3),
                    noise=0.6)
        tag = f"n{n:04d}_p{p}"
        write(f"{OUT}/{tag}_a.txt", a)
        write(f"{OUT}/{tag}_b.txt", b)
        print(f"wrote {tag}: n={n}")

print("done")
