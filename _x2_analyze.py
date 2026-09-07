#!/usr/bin/env python3
# Analyze x2_perpair.csv for K_0 = A0/B0 (Epeck / Epick), no-maximal, fut_lmf.
# Intersection of completed pairs (§6.4); censoring reported separately.
import csv, math, random, sys
random.seed(20260901)
PATH = sys.argv[1] if len(sys.argv) > 1 else "/home/sjp/_x1/x2_perpair.csv"
rows = list(csv.DictReader(open(PATH)))
d = {}
for r in rows: d[(r["pair"], r["arm"])] = r
pairs = sorted({r["pair"] for r in rows})

def geomean(logs): return math.exp(sum(logs)/len(logs)) if logs else float("nan")
def boot(logs, B=10000, a=0.05):
    if not logs: return (float("nan"),)*2
    n=len(logs); g=[]
    for _ in range(B):
        s=sum(logs[random.randrange(n)] for _ in range(n)); g.append(math.exp(s/n))
    g.sort(); return (g[int(a/2*B)], g[int((1-a/2)*B)])

# censoring
cens = {"A0":0, "B0":0}
for p in pairs:
    for arm in ("A0","B0"):
        r=d.get((p,arm))
        if r and r["status"]!="OK": cens[arm]+=1
print(f"== X2 K_0 analysis: {PATH} ==  pairs={len(pairs)}")
print(f"censored(timeout/err): A0={cens['A0']} B0={cens['B0']}")

# answer check on completed-both pairs
mism=0; both=0
for p in pairs:
    a=d.get((p,"A0")); b=d.get((p,"B0"))
    if a and b and a["status"]=="OK" and b["status"]=="OK":
        both+=1
        try:
            if a["ans"] and b["ans"] and abs(float(a["ans"])-float(b["ans"]))>1e-6: mism+=1
        except ValueError: pass
print(f"completed-both pairs={both}  answer mismatches(>1e-6)={mism}")

def contrast(metric, label, active=None):
    logs=[]; nsum=0.0; dsum=0.0; used=0; skip=0
    for p in pairs:
        a=d.get((p,"A0")); b=d.get((p,"B0"))
        if not a or not b or a["status"]!="OK" or b["status"]!="OK": continue
        try: va=float(a[metric]); vb=float(b[metric])
        except (ValueError, TypeError): skip+=1; continue
        if active and (va<active or vb<active): skip+=1; continue
        if va<=0 or vb<=0: skip+=1; continue
        nsum+=va; dsum+=vb; logs.append(math.log(va/vb)); used+=1
    if not logs: print(f"  [{label}] no usable pairs (used=0, skip={skip})"); return
    rs=sorted(math.exp(x) for x in logs); gm=geomean(logs); lo,hi=boot(logs)
    print(f"  [{label}]  pairs={used} (skip {skip})")
    print(f"     sum-ratio ΣA0/ΣB0 = {nsum/dsum:.3f}")
    print(f"     pair geomean      = {gm:.3f}  95%CI [{lo:.3f}, {hi:.3f}]")
    print(f"     median={rs[len(rs)//2]:.3f}  IQR[{rs[len(rs)//4]:.3f},{rs[3*len(rs)//4]:.3f}]  min={rs[0]:.3f} max={rs[-1]:.3f}")

print("\n-- K_0 by WALL (end-to-end) --");        contrast("wall_ms","K_0 wall")
print("\n-- K_0 by ARRANGEMENT build time (arr_ms), active>0.05ms --"); contrast("arr_ms","K_0 arr", active=0.05)
print("\n-- K_0 by INTERNAL (arr+fre) — need combined; computed below --")
# internal = arr+fre
for p in pairs:
    for arm in ("A0","B0"):
        r=d.get((p,arm))
        if r and r["status"]=="OK":
            try: r["_int"]=str(float(r["arr_ms"])+float(r["fre_ms"]))
            except (ValueError,TypeError): r["_int"]="0"
contrast("_int","K_0 internal", active=0.05)

# aggregate arrangement/predicate time
print("\n-- aggregate internal time on completed-both pairs --")
for arm in ("A0","B0"):
    arr=fre=0.0; nb=0
    for p in pairs:
        a=d.get((p,"A0")); b=d.get((p,"B0")); r=d.get((p,arm))
        if not(a and b and a["status"]=="OK" and b["status"]=="OK" and r): continue
        try: arr+=float(r["arr_ms"]); fre+=float(r["fre_ms"]); nb+=1
        except (ValueError,TypeError): pass
    print(f"   {arm}: Arr_sum={arr:.0f}ms  Frechet_sum={fre:.0f}ms  internal={arr+fre:.0f}ms  (over {nb} pairs)")
