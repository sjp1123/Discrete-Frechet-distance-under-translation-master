#!/usr/bin/env python3
# Generate matched-n curve pairs for the X3-a scaling sweep / X5 degeneracy:
#   geo_*  : geolife_100 curves uniformly subsampled to length n (DEGENERATE)
#   syn_*  : smooth random walk a; b = rot(±0.15)+trans(±3)+noise(σ=0.6) (GEN. POS.)
# Writes to $HOME/_x3work/. Design: NEXT_P0_X3_X5.md §3.3.
import os, sys, math, random

HOME = os.path.expanduser("~")
GEO  = os.path.join(HOME, "geodata")
ROOT = "/mnt/c/Users/shiju/Downloads/Discrete-Frechet-distance-under-translation-master/Discrete-Frechet-distance-under-translation-master"
MAN  = os.path.join(ROOT, "test_cases", "geolife_100", "manifest.txt")
WORK = os.path.join(HOME, "_x3work"); os.makedirs(WORK, exist_ok=True)

NS     = [int(x) for x in (sys.argv[1].split(",") if len(sys.argv) > 1 else "8,12,16,24".split(","))]
PAIRS  = int(sys.argv[2]) if len(sys.argv) > 2 else 6

def read_curve(p):
    pts = []
    for line in open(p):
        t = line.split()
        if len(t) >= 2: pts.append((float(t[0]), float(t[1])))
    return pts

def subsample(pts, n):
    if len(pts) <= n: return pts
    return [pts[round(i*(len(pts)-1)/(n-1))] for i in range(n)]

def write_curve(p, pts):
    with open(p, "w") as f:
        for (x, y) in pts: f.write(f"{x!r} {y!r}\n")

# geolife pairs
man = [l.split() for l in open(MAN) if l.split()]
gcount = 0
for idx in range(min(PAIRS, len(man))):
    _, f1, f2 = man[idx][0], man[idx][1], man[idx][2]
    p1, p2 = os.path.join(GEO, f1), os.path.join(GEO, f2)
    if not (os.path.exists(p1) and os.path.exists(p2)): continue
    c1, c2 = read_curve(p1), read_curve(p2)
    for n in NS:
        write_curve(os.path.join(WORK, f"geo_n{n:02d}_p{idx}_a.txt"), subsample(c1, n))
        write_curve(os.path.join(WORK, f"geo_n{n:02d}_p{idx}_b.txt"), subsample(c2, n))
    gcount += 1

# synthetic pairs (general position)
def smooth_walk(n, rng, step=1.0, momentum=0.7):
    x=y=vx=vy=0.0; pts=[]
    for _ in range(n):
        vx = momentum*vx + (1-momentum)*rng.gauss(0, step)
        vy = momentum*vy + (1-momentum)*rng.gauss(0, step)
        x += vx; y += vy; pts.append((x, y))
    return pts

for idx in range(PAIRS):
    rng = random.Random(1000 + idx)
    for n in NS:
        a = smooth_walk(n, rng)
        th = rng.uniform(-0.15, 0.15); tx = rng.uniform(-3, 3); ty = rng.uniform(-3, 3)
        ct, st = math.cos(th), math.sin(th)
        b = [(ct*x - st*y + tx + rng.gauss(0, 0.6),
              st*x + ct*y + ty + rng.gauss(0, 0.6)) for (x, y) in a]
        write_curve(os.path.join(WORK, f"syn_n{n:02d}_p{idx}_a.txt"), a)
        write_curve(os.path.join(WORK, f"syn_n{n:02d}_p{idx}_b.txt"), b)

print(f"wrote to {WORK}: geolife pairs={gcount}, synthetic pairs={PAIRS}, n={NS}")
