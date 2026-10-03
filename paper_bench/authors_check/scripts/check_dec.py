import csv, math, os, sys
from functools import lru_cache
# usage: check_dec.py <instdir> <fam> <csv>... : decider answers vs exact truth (same rules as verification/scripts/check.py)
D, fam = sys.argv[1], sys.argv[2]
oracle = {l.split()[0]: float(l.split()[5]) for l in open(os.path.join(D, "oracle_out.txt")) if len(l.split()) > 5}
@lru_cache(None)
def maxc(f, i):
    M = 0.
    for s in "ab":
        for l in open(os.path.join(D, f, f"{i}_{s}.txt")):
            x, y = map(float, l.split()); M = max(M, abs(x), abs(y))
    return M
truth = [l.split() for l in open(os.path.join(D, f"{fam}.truth")) if l.strip()]
for f in sys.argv[3:]:
    if not os.path.exists(f): print(f, "missing"); continue
    R = list(csv.DictReader(open(f)))
    wn = wy = tie = hang = 0; ex = []
    for (key, d, cls, kind), r in zip(truth, R):
        if r["status"] != "OK": hang += 1; ex.append(f"{r['status']} {key} {kind}"); continue
        ans = r["answer"] == "1"; dv, ov = float(d), oracle[key]
        up4 = ov
        for _ in range(4): up4 = math.nextafter(up4, math.inf)
        res = 16 * 2.0 ** -53 * (maxc(*key.split("/")) + dv)
        if cls == "YES" and (dv <= up4 or dv - ov < res):
            tie += (not ans); continue
        if cls == "YES" and not ans: wn += 1; ex.append(f"wrongNO {key} {kind}")
        elif cls == "NO" and ans: wy += 1; ex.append(f"wrongYES {key} {kind}")
    print(f"{os.path.basename(f)}: queries={len(R)} wrongNO={wn} wrongYES={wy} tieNO={tie} hang/crash={hang}")
    for e in ex[:6]: print("   ", e)
