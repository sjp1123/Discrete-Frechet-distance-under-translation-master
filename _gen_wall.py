#!/usr/bin/env python3
# Larger curves so end-to-end fut_lmf wall-clock >> process startup, giving a clean
# read on "does adding the maximal filter to fut_lmf help the TOTAL runtime?"
import os, math, random
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "_wall")
os.makedirs(OUT, exist_ok=True)
NS = [100, 200, 400, 800]
PAIRS = 3
random.seed(7)

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
