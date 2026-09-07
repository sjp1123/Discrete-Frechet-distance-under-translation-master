#!/usr/bin/env python3
# Small-n curve pairs to drive the GLOBAL n6 arrangement (discs = n1*n2), so the
# arrangement grows with n and we can find the break-even where the maximal filter
# (orig->candidate, both CGAL) stops being a net loss.
import os, math, random
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "_nsweep")
os.makedirs(OUT, exist_ok=True)
NS = [6, 8, 10, 12, 14, 16, 18, 20, 24, 28, 32]
PAIRS = 2
random.seed(2026)

def random_walk(n, step=1.0):
    x = y = 0.0; pts = [(x, y)]; ang = random.uniform(0, 2*math.pi)
    for _ in range(n-1):
        ang += random.gauss(0, 0.4)
        x += step*math.cos(ang); y += step*math.sin(ang); pts.append((x, y))
    return pts

def perturb(pts, dtheta, tx, ty, noise):
    c, s = math.cos(dtheta), math.sin(dtheta); out = []
    for (x, y) in pts:
        out.append((c*x - s*y + tx + random.gauss(0, noise),
                    s*x + c*y + ty + random.gauss(0, noise)))
    return out

def write(path, pts):
    with open(path, "w", newline="\n") as f:
        for (x, y) in pts:
            f.write(f"{x:.6f} {y:.6f}\n")

for n in NS:
    for p in range(PAIRS):
        a = random_walk(n)
        b = perturb(a, random.uniform(-0.15, 0.15),
                    random.uniform(-3, 3), random.uniform(-3, 3), noise=0.6)
        write(f"{OUT}/n{n:04d}_p{p}_a.txt", a)
        write(f"{OUT}/n{n:04d}_p{p}_b.txt", b)
print("done", NS, "x", PAIRS, "->", OUT)
