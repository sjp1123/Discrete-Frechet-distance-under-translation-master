#!/usr/bin/env python3
# Candidate 4 runtime analysis from _c4_bench.sh output.
# Same statistics as _factor_analyze.py: per-pair log-ratios, geomean +
# bootstrap 95% CI, on the intersection of pairs where both arms of a contrast
# completed.  Pure stdlib (no numpy/scipy in this WSL).
#
# Arms: A0 original | D1 candidate2 | C4S slack only | C4N pipeline only | C4 both
import csv, math, random, sys

random.seed(20260902)
PATH = sys.argv[1] if len(sys.argv) > 1 else "/home/sjp/_c4/bench.csv"
rows = list(csv.DictReader(open(PATH)))
ARMS = ["A0", "D1", "C4S", "C4N", "C4"]
LABEL = {"A0": "original (Epeck+DCEL)", "D1": "candidate2", "C4S": "c4 slack-only",
         "C4N": "c4 pipeline-only", "C4": "candidate4"}

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


print(f"== candidate4 runtime analysis: {PATH} ==  pairs={len(pairs)}")
cens = {a: sum(1 for p in pairs if d.get((p, a)) and d[(p, a)]["status"] != "OK") for a in ARMS}
print("censored (60s timeout): " + "  ".join(f"{a}={cens[a]}" for a in ARMS))

# ---- answer agreement vs D1 (candidate2), the baseline being replaced -------
print("\n-- answer agreement vs candidate2 --")
for arm in ARMS:
    if arm == "D1":
        continue
    diffs = []
    for p in pairs:
        rb, rr = d.get((p, "D1")), d.get((p, arm))
        if not rb or not rr or rb["status"] != "OK" or rr["status"] != "OK":
            continue
        try:
            diffs.append(abs(float(rb["ans"]) - float(rr["ans"])))
        except (ValueError, TypeError):
            pass
    if diffs:
        over = sum(1 for x in diffs if x > 1e-7)
        print(f"  {arm:4s} n={len(diffs):3d}  max|Δ|={max(diffs):.2e}  "
              f"mean|Δ|={sum(diffs)/len(diffs):.2e}  #(>eps=1e-7)={over}")

CONTRASTS = [
    ("MAIN   D1/C4 ", "D1", "C4", "candidate2 -> candidate4"),
    ("slack  D1/C4S", "D1", "C4S", "slack alignment alone"),
    ("pipe   D1/C4N", "D1", "C4N", "pipeline alone"),
    ("pipe|s C4S/C4", "C4S", "C4", "pipeline, given slack"),
    ("TOTAL  A0/C4 ", "A0", "C4", "original -> candidate4"),
    ("ref    A0/D1 ", "A0", "D1", "original -> candidate2 (prior work)"),
]

for metric, mlabel, floor in [("wall_ms", "WALL", None),
                              ("arr_ms", "ARRANGEMENT", 0.05),
                              ("_int", "INTERNAL (arr+fre)", 0.05)]:
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

# ---- absolute totals on the all-arms-complete subset ------------------------
common = [p for p in pairs if all(val(p, a, "wall_ms") is not None for a in ARMS)]
print(f"\n-- absolute wall totals (pairs where all 5 arms completed: n={len(common)}) --")
for a in ARMS:
    tot = sum(val(p, a, "wall_ms") for p in common)
    print(f"  {a:4s} {LABEL[a]:24s} {tot/1000:8.2f} s")

# ---- work counters ----------------------------------------------------------
print("\n-- decider work (arms that report it) --")
for a in ("D1", "C4S", "C4N", "C4"):
    b = q = 0
    for p in common:
        r = d.get((p, a))
        if not r or r["status"] != "OK":
            continue
        try:
            b += int(r["builds"]); q += int(r["queries"])
        except (ValueError, TypeError):
            pass
    print(f"  {a:4s} builds={b:8d}  decider_queries={q:9d}")
