#!/usr/bin/env python3
"""Generate small DFDuT instances in many families (curve files + list for the oracle + pair lists).
    python3 gen.py <outdir> <scale>   (scale multiplies the per-family counts)
"""
import os, random, sys

R = "/home/claude/Discrete-Frechet-distance-under-translation-master/paper_data"
OUT = sys.argv[1]
SCALE = float(sys.argv[2]) if len(sys.argv) > 2 else 1.0


def load_dir(d, names):
    out = []
    for nm in names:
        pts = []
        for line in open(os.path.join(d, nm)):
            p = line.split()
            if len(p) >= 2:
                pts.append((float(p[0]), float(p[1])))
        if pts:
            out.append(pts)
    return out


def subsample(c, k, rng):
    k = min(k, len(c))
    idx = sorted(rng.sample(range(len(c)), k))
    return [c[i] for i in idx]


def fam_uniform(rng, lo=1, hi=8, s=10.0, off=(0.0, 0.0)):
    n, m = rng.randint(lo, hi), rng.randint(lo, hi)
    P = [(off[0] + rng.uniform(-s, s), off[1] + rng.uniform(-s, s)) for _ in range(n)]
    Q = [(off[0] + rng.uniform(-s, s), off[1] + rng.uniform(-s, s)) for _ in range(m)]
    return P, Q


def fam_grid(rng):
    n, m = rng.randint(1, 8), rng.randint(1, 8)
    g = rng.choice([2, 3, 4])
    return ([(float(rng.randint(0, g)), float(rng.randint(0, g))) for _ in range(n)],
            [(float(rng.randint(0, g)), float(rng.randint(0, g))) for _ in range(m)])


def fam_cluster(rng):
    n, m = rng.randint(2, 8), rng.randint(2, 8)
    centres = [(rng.uniform(-5, 5), rng.uniform(-5, 5)) for _ in range(rng.randint(1, 3))]
    eps = 10 ** rng.uniform(-9, -2)
    pt = lambda: (lambda c: (c[0] + rng.uniform(-eps, eps), c[1] + rng.uniform(-eps, eps)))(rng.choice(centres))
    return [pt() for _ in range(n)], [pt() for _ in range(m)]


def fam_collinear(rng):
    n, m = rng.randint(1, 8), rng.randint(1, 8)
    a = rng.uniform(0, 3.14159)
    import math
    d = (math.cos(a), math.sin(a)) if rng.random() < 0.7 else (1.0, 0.0)
    o = (rng.uniform(-3, 3), rng.uniform(-3, 3))
    f = lambda: (lambda s: (o[0] + s * d[0], o[1] + s * d[1]))(rng.uniform(-10, 10))
    P = [f() for _ in range(n)]
    Q = [f() for _ in range(m)] if rng.random() < 0.7 else [(rng.uniform(-10, 10), rng.uniform(-10, 10)) for _ in range(m)]
    return P, Q


def fam_copy(rng):
    n = rng.randint(1, 8)
    P = [(rng.uniform(-10, 10), rng.uniform(-10, 10)) for _ in range(n)]
    sh = (rng.uniform(-100, 100), rng.uniform(-100, 100)) if rng.random() < 0.5 else (float(rng.randint(-50, 50)), float(rng.randint(-50, 50)))
    noise = 0.0 if rng.random() < 0.5 else 10 ** rng.uniform(-9, -3)
    Q = [(x + sh[0] + rng.uniform(-noise, noise), y + sh[1] + rng.uniform(-noise, noise)) for x, y in P]
    if rng.random() < 0.3:   # duplicate some points of Q (same Frechet distance class, different lengths)
        i = rng.randrange(len(Q)); Q.insert(i, Q[i])
    return P, Q


def fam_singleton(rng):
    n = rng.randint(1, 8)
    P = [(rng.uniform(-10, 10), rng.uniform(-10, 10)) for _ in range(n)]
    Q = [(rng.uniform(-10, 10), rng.uniform(-10, 10))]
    return (P, Q) if rng.random() < 0.5 else (Q, P)


def fam_revisit(rng):
    n, m = rng.randint(2, 8), rng.randint(2, 8)
    base = [(rng.uniform(-5, 5), rng.uniform(-5, 5)) for _ in range(3)]
    return [rng.choice(base) for _ in range(n)], [(x + 1.5, y - 0.5) for x, y in (rng.choice(base) for _ in range(m))]


def make_families(rng):
    chars = sorted(os.listdir(os.path.join(R, "characters_uci", "data")), key=lambda s: int(s.split(".")[0]))
    sigp = [l.split() for l in open(os.path.join(R, "..", "paper_bench", "queries", "sigspatial_pairs.txt")) if l.strip()]
    cdir, sdir = os.path.join(R, "characters_uci", "data"), os.path.join(R, "sigspatial", "data")
    ccache, scache = {}, {}

    def ccurve(nm):
        if nm not in ccache: ccache[nm] = load_dir(cdir, [nm])[0]
        return ccache[nm]

    def scurve(nm):
        if nm not in scache: scache[nm] = load_dir(sdir, [nm])[0]
        return scache[nm]

    def real_chars(rng, lo=2, hi=8):
        a, b = rng.choice(chars), rng.choice(chars)
        return subsample(ccurve(a), rng.randint(lo, hi), rng), subsample(ccurve(b), rng.randint(lo, hi), rng)

    def real_sig(rng, lo=2, hi=8):
        a, b = rng.choice(sigp)
        return subsample(scurve(a), rng.randint(lo, hi), rng), subsample(scurve(b), rng.randint(lo, hi), rng)

    return [
        ("uniform", 1500, lambda r: fam_uniform(r)),
        ("grid", 1500, fam_grid),
        ("cluster", 1200, fam_cluster),
        ("collinear", 1200, fam_collinear),
        ("copy", 1200, fam_copy),
        ("singleton", 600, fam_singleton),
        ("revisit", 1000, fam_revisit),
        ("offset_sig", 1200, lambda r: fam_uniform(r, s=1000.0, off=(1.36e7, 4.5e6))),
        ("big", 1000, lambda r: fam_uniform(r, s=1e5)),
        ("tiny", 1000, lambda r: fam_uniform(r, s=1e-3)),
        ("real_chars", 1500, real_chars),
        ("real_sig", 1500, real_sig),
        ("medium_uniform", 250, lambda r: fam_uniform(r, lo=8, hi=12)),
        ("medium_real_chars", 250, lambda r: real_chars(r, 8, 12)),
        ("medium_real_sig", 250, lambda r: real_sig(r, 8, 12)),
    ]


def main():
    rng = random.Random(20261001)
    os.makedirs(OUT, exist_ok=True)
    olist = open(os.path.join(OUT, "oracle_list.txt"), "w")
    for name, cnt, gen in make_families(rng):
        d = os.path.join(OUT, name)
        os.makedirs(d, exist_ok=True)
        pairs = open(os.path.join(OUT, f"{name}.pairs"), "w")
        for i in range(int(cnt * SCALE)):
            P, Q = gen(rng)
            a, b = f"{i:05d}_a.txt", f"{i:05d}_b.txt"
            for fn, C in ((a, P), (b, Q)):
                with open(os.path.join(d, fn), "w") as f:
                    for x, y in C:
                        f.write(f"{x!r} {y!r}\n")
            pairs.write(f"{a} {b}\n")
            olist.write(f"{name}/{i:05d} {os.path.join(d, a)} {os.path.join(d, b)}\n")
        pairs.close()
        print(name, int(cnt * SCALE))
    olist.close()


if __name__ == "__main__":
    main()
