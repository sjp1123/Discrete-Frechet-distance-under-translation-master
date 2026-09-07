#!/usr/bin/env python3
# Definitive factor table from factor_perpair.csv (arms A0,B0,D0,D1).
# Contrasts: K_0=A0/B0, S_0=B0/D0, M_D=D0/D1, T=A0/D1. Reports each on wall,
# arrangement, and internal(arr+fre) metrics with geomean + bootstrap 95% CI,
# on the intersection of pairs where all four arms completed. Checks the
# identity T ?= K_0*S_0*M_D per-pair.
import csv, math, random, sys
random.seed(20260901)
PATH = sys.argv[1] if len(sys.argv) > 1 else "/home/sjp/_x1/factor_perpair.csv"
rows = list(csv.DictReader(open(PATH)))
ARMS = ["A0","B0","D0","D1"]
d = {}
for r in rows:
    try: r["_int"] = float(r["arr_ms"]) + float(r["fre_ms"])
    except (ValueError, TypeError): r["_int"] = None
    d[(r["pair"], r["arm"])] = r
pairs = sorted({r["pair"] for r in rows})

def val(p, arm, metric):
    r = d.get((p, arm))
    if not r or r["status"] != "OK": return None
    if metric == "_int": return r["_int"]
    try:
        v = float(r[metric]); return v if v > 0 else None
    except (ValueError, TypeError): return None

def geomean(logs): return math.exp(sum(logs)/len(logs)) if logs else float("nan")
def boot(logs, B=10000, a=0.05):
    if not logs: return (float("nan"),)*2
    n=len(logs); g=[]
    for _ in range(B):
        s=sum(logs[random.randrange(n)] for _ in range(n)); g.append(math.exp(s/n))
    g.sort(); return (g[int(a/2*B)], g[int((1-a/2)*B)])

# censoring
cens = {a: sum(1 for p in pairs if (d.get((p,a)) and d[(p,a)]["status"]!="OK")) for a in ARMS}
print(f"== factor analysis: {PATH} ==  pairs={len(pairs)}")
print(f"censored per arm: " + "  ".join(f"{a}={cens[a]}" for a in ARMS))

# answer consistency vs A0 reference
mism=0
for p in pairs:
    a=val(p,"A0","wall_ms")
    ra=d.get((p,"A0"))
    if not ra or ra["status"]!="OK" or not ra["ans"]: continue
    for arm in ARMS[1:]:
        rr=d.get((p,arm))
        if rr and rr["status"]=="OK" and rr["ans"]:
            try:
                if abs(float(ra["ans"])-float(rr["ans"]))>1e-6: mism+=1
            except ValueError: pass
print(f"answer mismatches vs A0 (>1e-6): {mism}")

CONTRASTS = [("K_0","A0","B0"), ("S_0","B0","D0"), ("M_D","D0","D1"), ("T","A0","D1")]

for metric, mlabel, active in [("wall_ms","WALL",None), ("arr_ms","ARRANGEMENT",0.05), ("_int","INTERNAL(arr+fre)",0.05)]:
    print(f"\n################  metric = {mlabel}  ################")
    for name, num, den in CONTRASTS:
        logs=[]; nsum=dsum=0.0; used=0
        for p in pairs:
            vn=val(p,num,metric); vd=val(p,den,metric)
            if vn is None or vd is None: continue
            if active and (vn<active or vd<active): continue
            nsum+=vn; dsum+=vd; logs.append(math.log(vn/vd)); used+=1
        if not logs: print(f"  {name}={num}/{den}: no usable pairs"); continue
        rs=sorted(math.exp(x) for x in logs); gm=geomean(logs); lo,hi=boot(logs)
        print(f"  {name:4s}={num}/{den}: geomean={gm:.3f} CI[{lo:.3f},{hi:.3f}]  sum-ratio={nsum/dsum:.3f}  median={rs[len(rs)//2]:.3f} (n={used})")
    # identity check on all-4-complete pairs
    prod_logs=[]; t_logs=[]
    for p in pairs:
        vs={a: val(p,a,metric) for a in ARMS}
        if any(vs[a] is None for a in ARMS): continue
        if active and any(vs[a]<active for a in ARMS): continue
        prod_logs.append(math.log(vs["A0"]/vs["B0"]) + math.log(vs["B0"]/vs["D0"]) + math.log(vs["D0"]/vs["D1"]))
        t_logs.append(math.log(vs["A0"]/vs["D1"]))
    if prod_logs:
        print(f"  identity: geomean(K_0*S_0*M_D)={geomean(prod_logs):.3f}  vs  geomean(T)={geomean(t_logs):.3f}  (n={len(prod_logs)})")

# aggregate internal per arm (all-4-complete pairs)
print("\n-- aggregate internal time (pairs where all 4 arms completed) --")
comp=[p for p in pairs if all(val(p,a,"_int") is not None for a in ARMS)]
for a in ARMS:
    arr=sum(float(d[(p,a)]["arr_ms"]) for p in comp)
    fre=sum(float(d[(p,a)]["fre_ms"]) for p in comp)
    print(f"   {a}: Arr_sum={arr:.0f}ms  Frechet_sum={fre:.0f}ms  internal={arr+fre:.0f}ms  (n={len(comp)})")
