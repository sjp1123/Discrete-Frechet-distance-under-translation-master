#!/usr/bin/env python3
# Independent DFDuT oracle: NO CGAL, NO arrangement — a brute-force grid search
# over translations with a plain discrete-Fréchet DP. Anchors the correctness of
# the CGAL-stack implementations (A0/D1/C4) to a fully separate method.
# Usage: _oracle.py fileP fileQ   ->  prints min_t discreteFrechet(P, Q+t)
import sys, math

def read_curve(path):
    pts=[]
    for line in open(path):
        f=line.split()
        if len(f)>=2:
            try: pts.append((float(f[0]), float(f[1])))
            except ValueError: pass
    return pts

def dfrechet(P, Q, tx, ty):
    n, m = len(P), len(Q)
    prev=[0.0]*m
    for j in range(m):
        d=math.hypot(P[0][0]-(Q[j][0]+tx), P[0][1]-(Q[j][1]+ty))
        prev[j]=d if j==0 else max(prev[j-1], d)
    for i in range(1, n):
        cur=[0.0]*m
        d0=math.hypot(P[i][0]-(Q[0][0]+tx), P[i][1]-(Q[0][1]+ty))
        cur[0]=max(prev[0], d0)
        for j in range(1, m):
            d=math.hypot(P[i][0]-(Q[j][0]+tx), P[i][1]-(Q[j][1]+ty))
            cur[j]=max(min(prev[j], prev[j-1], cur[j-1]), d)
        prev=cur
    return prev[m-1]

def centroid(P):
    return (sum(p[0] for p in P)/len(P), sum(p[1] for p in P)/len(P))

def extent(P):
    xs=[p[0] for p in P]; ys=[p[1] for p in P]
    return max(max(xs)-min(xs), max(ys)-min(ys), 1.0)

def search(P, Q):
    cP, cQ = centroid(P), centroid(Q)
    cx, cy = cP[0]-cQ[0], cP[1]-cQ[1]          # centroid-align translation
    H = max(extent(P), extent(Q))               # generous half-span
    best = (1e18, cx, cy)
    for (span, grid) in [(H, 120), (H/60, 80), (H/60/40, 80), (H/60/40/40, 60)]:
        bx, by = best[1], best[2]
        step = 2*span/grid
        for a in range(grid+1):
            tx = bx - span + a*step
            for b in range(grid+1):
                ty = by - span + b*step
                d = dfrechet(P, Q, tx, ty)
                if d < best[0]: best = (d, tx, ty)
    return best

if __name__=="__main__":
    P=read_curve(sys.argv[1]); Q=read_curve(sys.argv[2])
    d,tx,ty=search(P,Q)
    print(f"{d:.9f}")
