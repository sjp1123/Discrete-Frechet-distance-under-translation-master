#!/usr/bin/env python3
# X3/X5 exact maximal predicate — Python reference (NEXT_P0_X3_X5.md Appendix A).
# Validate the one-root sign algebra + degenerate-family detection here BEFORE
# porting to C++ (Gmpq). All arithmetic exact via fractions.Fraction; the only
# irrationality is a single sqrt(s) shared by every vertex, handled by comparing
# a + b*sqrt(s) against 0 without ever taking the root.
from fractions import Fraction as F

def sgn(x): return (x > 0) - (x < 0)

def sign_one_root(A, C, s):
    """sign(A + C*sqrt(s)) for A,C,s in Q, s>=0, in {-1,0,1}."""
    if s == 0 or C == 0:
        return sgn(A)
    if A >= 0 and C > 0: return 1
    if A <= 0 and C < 0: return -1
    D = A*A - C*C*s              # signs of A,C differ -> compare squares
    sd = sgn(D)
    return sd if A > 0 else -sd

def vertex(centers, i, j, sigma, r2):
    """Vertex p = ∂D_i ∩ ∂D_j (branch sigma in {+1,-1}).
    Returns dict with mask, active list, and the one-root data (g_k, w, s, sigma)
    for each active constraint — or None if no such vertex (disjoint/coincident/
    tangent-duplicate)."""
    (cix, ciy), (cjx, cjy) = centers[i], centers[j]
    dx, dy = cjx - cix, cjy - ciy
    d2 = dx*dx + dy*dy
    if d2 == 0 or d2 > 4*r2:                 # coincident / disjoint
        return None
    if d2 == 4*r2 and sigma == -1:           # tangent: both branches coincide
        return None
    s = (r2 - d2/F(4)) / d2                   # >= 0
    mx, my = (cix + cjx)/F(2), (ciy + cjy)/F(2)
    wx, wy = -dy, dx                          # |w|^2 == d2
    mask, active = 0, []
    gmap = {}
    for k, (ckx, cky) in enumerate(centers):
        gx, gy = mx - ckx, my - cky
        A = gx*gx + gy*gy + s*d2 - r2
        B = 2*(gx*wx + gy*wy)
        t = sign_one_root(A, sigma*B, s)
        if t <= 0:
            mask |= 1 << k
            if t == 0:
                active.append(k)
                gmap[k] = (gx, gy)
    return {"mask": mask, "active": active, "s": s, "w": (wx, wy),
            "sigma": sigma, "g": gmap, "d2": d2}

def cross(ax, ay, bx, by): return ax*by - ay*bx

def normals_positively_span(v):
    """Do the active outward normals n_k = p - c_k positively span R^2
    (0 strictly interior of their convex hull)?  If yes, ∩S has empty interior
    => the maximal family at this vertex is DEGENERATE (M-R identity, §4.3).
    Each n_k = g_k + sigma*sqrt(s)*w, so cross(n_a,n_b) = Ac + sigma*sqrt(s)*Bc
    with Ac,Bc rational -> sign via sign_one_root."""
    act = v["active"]; s = v["s"]; sig = v["sigma"]; wx, wy = v["w"]
    n = len(act)
    if n < 3:
        return False                          # need >=3 to surround a point
    g = [v["g"][k] for k in act]
    def cross_sign(a, b):
        gax, gay = g[a]; gbx, gby = g[b]
        Ac = cross(gax, gay, gbx, gby)                       # g_a x g_b
        Bc = cross(gax - gbx, gay - gby, wx, wy)             # (g_a-g_b) x w
        return sign_one_root(Ac, sig*Bc, s)
    # positively span  <=>  for every k, the others straddle its line
    # (some strictly on each side). Equivalently NOT all in a closed halfplane.
    for a in range(n):
        pos = neg = False
        for b in range(n):
            if b == a: continue
            c = cross_sign(a, b)
            if c > 0: pos = True
            elif c < 0: neg = True
        if not (pos and neg):
            return False                      # all others in one closed halfplane
    return True

def exact_masks(centers, r2):
    """Enumerate all pairwise-vertex masks (deduped), return list of
    (mask, is_degenerate) for each distinct mask that is inclusion-maximal."""
    seen = {}
    n = len(centers)
    for i in range(n):
        for j in range(i+1, n):
            for sigma in (+1, -1):
                v = vertex(centers, i, j, sigma, r2)
                if v is None: continue
                m = v["mask"]
                deg = normals_positively_span(v)
                # keep degeneracy if ANY realizing vertex is degenerate
                if m not in seen: seen[m] = deg
                else: seen[m] = seen[m] or deg
    # inclusion-maximal filter over distinct masks
    masks = list(seen.keys())
    maximal = []
    for m in masks:
        if not any((m & ~m2) == 0 and m != m2 for m2 in masks):  # m strict subset of some m2?
            maximal.append(m)
    return [(m, seen[m]) for m in maximal]

# ─────────────────────────── unit tests ───────────────────────────
def popcount(m): return bin(m).count("1")

def T(name, cond):
    print(f"  [{'PASS' if cond else 'FAIL'}] {name}")
    return cond

def run():
    ok = True
    r2 = F(1)
    # 1) two centers at distance r=1 -> 2 intersection points, each mask {0,1}
    C = [(F(0),F(0)), (F(1),F(0))]
    vp = vertex(C,0,1,+1,r2); vm = vertex(C,0,1,-1,r2)
    ok &= T("2 discs dist r: two distinct vertices", vp and vm and vp["mask"]==0b11 and vm["mask"]==0b11)
    ok &= T("2 discs dist r: popcount 2, not degenerate", popcount(vp["mask"])==2 and not normals_positively_span(vp))

    # 2) EXACT degenerate triple (rational): centers on radius-5 circle around origin,
    #    normals surrounding origin -> empty interior -> degenerate maximal family
    C = [(F(5),F(0)), (F(-3),F(4)), (F(-3),F(-4))]; r2 = F(25)
    res = exact_masks(C, r2)
    full = next((d for (m,d) in res if m==0b111), None)
    ok &= T("degenerate triple: {0,1,2} is a maximal mask", any(m==0b111 for (m,_) in res))
    ok &= T("degenerate triple: flagged DEGENERATE (empty interior)", full is True)

    # 3) NON-degenerate triple: rational points on radius-5 circle, all in a halfplane
    C = [(F(5),F(0)), (F(4),F(3)), (F(3),F(4))]; r2 = F(25)
    res = exact_masks(C, r2)
    v012 = vertex(C,0,1,+1,r2)
    # origin is common; find a vertex with mask {0,1,2}
    found_deg = any(m==0b111 and d for (m,d) in res)
    ok &= T("non-degenerate triple: {0,1,2} NOT flagged degenerate", not found_deg)

    # 4) tangent: two centers at distance exactly 2r -> single vertex (sigma dedup)
    C = [(F(0),F(0)), (F(2),F(0))]; r2 = F(1)
    vp = vertex(C,0,1,+1,r2); vm = vertex(C,0,1,-1,r2)
    ok &= T("tangent dist 2r: sigma=-1 returns None (dedup)", vm is None and vp is not None)

    # 5) coincident centers -> None
    C = [(F(0),F(0)), (F(0),F(0))]; r2 = F(1)
    ok &= T("coincident discs: None", vertex(C,0,1,+1,r2) is None)

    # 6) square, diagonal < 2r -> center covered by all four (mask all-ones exists)
    C = [(F(0),F(0)), (F(1),F(0)), (F(0),F(1)), (F(1),F(1))]; r2 = F(1)  # diag sqrt2 < 2
    res = exact_masks(C, r2)
    ok &= T("square diag<2r: a full mask {0,1,2,3} appears", any(popcount(m)==4 for (m,_) in res))

    print("\nALL PASS" if ok else "\nSOME FAILED"); return ok

if __name__ == "__main__":
    import sys
    sys.exit(0 if run() else 1)
