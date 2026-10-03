import csv, os, sys
from functools import lru_cache
# usage: check_lmf.py <instdir> <fam> <csv>...  : LMF values vs exact oracle, tol = 1e-7 + 4e-15*max|coord|
D, fam = sys.argv[1], sys.argv[2]
oracle = {}
for line in open(os.path.join(D, "oracle_out.txt")):
    p = line.split()
    if len(p) > 5: oracle[p[0]] = float(p[5])
@lru_cache(None)
def maxc(i):
    M = 0.
    for s in "ab":
        for l in open(os.path.join(D, fam, f"{i}_{s}.txt")):
            x, y = map(float, l.split()); M = max(M, abs(x), abs(y))
    return M
pairs = [l.split() for l in open(os.path.join(D, f"{fam}.pairs")) if l.strip()]
for f in sys.argv[3:]:
    if not os.path.exists(f): print(f, "missing"); continue
    L = list(csv.DictReader(open(f)))
    bad = []; hang = []; maxe = 0; up = dn = 0
    for (a, b), r in zip(pairs, L):
        i = a.split("_")[0]; key = f"{fam}/{i}"; opt = oracle[key]
        if r["status"] != "OK": hang.append((i, r["status"])); continue
        e = float(r["value"]) - opt; maxe = max(maxe, abs(e))
        if abs(e) > 1e-7 + 4e-15 * maxc(i):
            bad.append((i, e)); up += e > 0; dn += e < 0
    print(f"{os.path.basename(f)}: n={len(L)} wrong={len(bad)} (too large {up}, too small {dn}) hang/crash={len(hang)} {hang[:5]} max|err|={maxe:.2e}")
    for i, e in bad[:12]: print(f"   {i} err={e:+.3e}")
