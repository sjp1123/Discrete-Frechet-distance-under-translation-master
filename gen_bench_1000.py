#!/usr/bin/env python3
import os, math, random

OUT = "test_cases/bench_1000"
os.makedirs(OUT, exist_ok=True)

NPAIRS = 1000
random.seed(20260612)

def random_walk(n, step=1.0):
    x, y = 0.0, 0.0
    pts = [(x, y)]
    ang = random.uniform(0, 2*math.pi)
    for _ in range(n-1):
        ang += random.gauss(0, 0.4)
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

manifest = open(f"{OUT}/manifest.txt", "w")
for i in range(NPAIRS):
    n = random.randint(100, 1000)
    a = random_walk(n)
    b = perturb(a, random.uniform(-0.15, 0.15),
                random.uniform(-3, 3), random.uniform(-3, 3), noise=0.6)
    tag = f"p{i:04d}"
    write(f"{OUT}/{tag}_a.txt", a)
    write(f"{OUT}/{tag}_b.txt", b)
    manifest.write(f"{tag} {n}\n")
manifest.close()
print(f"generated {NPAIRS} pairs in {OUT}")
