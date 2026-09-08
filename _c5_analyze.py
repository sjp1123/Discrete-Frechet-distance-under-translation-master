#!/usr/bin/env python3
# candidate5 analysis: same statistics as _c4_analyze.py / _factor_analyze.py —
# per-pair log-ratios, geomean + 10k bootstrap 95% CI, on the intersection of
# pairs where both arms of a contrast completed.  Pure stdlib.
#
# Arms: A0 original | C5L control (=A0 code path) | C5N pipeline only | C5 both
import csv, math, random, sys

random.seed(20260908)
PATH = sys.argv[1]
rows = list(csv.DictReader(open(PATH)))
ARMS = ["A0", "C5L", "C5N", "C5"]
LABEL = {"A0": "original (Epeck+DCEL)", "C5L": "c5 legacy control",
         "C5N": "c5 Cech, no slack", "C5": "c5 Cech + slack"}

d = {}
for r in rows:
    try:
        r["_int"] = float(r["arr_ms"]) + float(r["fre_ms"])
    except (ValueError, TypeError):
        r["_int"] = None
    d[(r["pair"], r["arm"])] = r
pairs = sorted({r["pair"] for r in rows})


def val(p, arm, metric):
    r = d.get((p, arm))
    if not r or r["status"] != "OK":
        return None
    if metric == "_int":
        return r["_int"]
    try:
        v = float(r[metric])
        return v if v > 0 else None
    except (ValueError, TypeError):
        return None


def geomean(logs):
    return math.exp(sum(logs) / len(logs)) if logs else float("nan")


def boot(logs, B=10000, a=0.05):
    if not logs:
        return (float("nan"),) * 2
    n = len(logs)
    g = []
    for _ in range(B):
        s = sum(logs[random.randrange(n)] for _ in range(n))
        g.append(math.exp(s / n))
    g.sort()
    return (g[int(a / 2 * B)], g[int((1 - a / 2) * B)])


print(f"== candidate5 analysis: {PATH} ==  pairs={len(pairs)}")
cens = {a: sum(1 for p in pairs if d.get((p, a)) and d[(p, a)]["status"] != "OK")
        for a in ARMS}
print("censored: " + "  ".join(f"{a}={cens[a]}" for a in ARMS))

print("\n-- answer agreement vs A0 (original, exact Epeck) --")
for arm in ARMS:
    if arm == "A0":
        continue
    diffs = []
    for p in pairs:
        rb, rr = d.get((p, "A0")), d.get((p, arm))
        if not rb or not rr or rb["status"] != "OK" or rr["status"] != "OK":
            continue
        try:
            diffs.append(abs(float(rb["ans"]) - float(rr["ans"])))
        except (ValueError, TypeError):
            pass
    if diffs:
        over = sum(1 for x in diffs if x > 1e-7)
        exact = sum(1 for x in diffs if x == 0.0)
        print(f"  {arm:4s} n={len(diffs):3d}  max|d|={max(diffs):.2e}  "
              f"mean|d|={sum(diffs)/len(diffs):.2e}  #(>eps=1e-7)={over}  "
              f"#(bit-identical)={exact}")

CONTRASTS = [
    ("MAIN  A0/C5 ", "A0", "C5",  "original -> original+Cech (slack-aligned)"),
    ("pipe  A0/C5N", "A0", "C5N", "pipeline only, no slack alignment"),
    ("slack C5N/C5", "C5N", "C5", "slack alignment given the pipeline"),
    ("ctrl  A0/C5L", "A0", "C5L", "control: same binary, legacy emitter"),
]

for metric, mlabel, floor in [("wall_ms", "WALL", None),
                              ("arr_ms", "ARRANGEMENT stage", 0.05),
                              ("_int", "INTERNAL (arr+fre)", 0.05),
                              ("bbcalls", "BLACK-BOX CALLS", None)]:
    print(f"\n################  metric = {mlabel}  ################")
    for name, num, den, note in CONTRASTS:
        logs, nsum, dsum = [], 0.0, 0.0
        for p in pairs:
            vn, vd = val(p, num, metric), val(p, den, metric)
            if vn is None or vd is None:
                continue
            if floor and (vn < floor or vd < floor):
                continue
            nsum += vn
            dsum += vd
            logs.append(math.log(vn / vd))
        if not logs:
            print(f"  {name}: no usable pairs")
            continue
        rs = sorted(math.exp(x) for x in logs)
        lo, hi = boot(logs)
        win = sum(1 for x in rs if x > 1.0)
        print(f"  {name}: geomean={geomean(logs):.3f} CI[{lo:.3f},{hi:.3f}]  "
              f"sum-ratio={nsum/dsum:.3f}  median={rs[len(rs)//2]:.3f}  "
              f"p10={rs[len(rs)//10]:.3f} p90={rs[(9*len(rs))//10]:.3f}  "
              f"faster={win}/{len(rs)}  ({note})")

common = [p for p in pairs if all(val(p, a, "wall_ms") is not None for a in ARMS)]
print(f"\n-- absolute totals (pairs where all arms completed: n={len(common)}) --")
print(f"  {'arm':4s} {'':24s} {'wall':>9s} {'arr':>9s} {'arr+fre':>9s} {'bbcalls':>10s}")
for a in ARMS:
    w = sum(val(p, a, "wall_ms") for p in common)
    ar = sum(val(p, a, "arr_ms") or 0 for p in common)
    it = sum(val(p, a, "_int") or 0 for p in common)
    bb = sum(int(d[(p, a)]["bbcalls"]) for p in common)
    print(f"  {a:4s} {LABEL[a]:24s} {w/1000:8.2f}s {ar/1000:8.2f}s "
          f"{it/1000:8.2f}s {bb:10d}")
