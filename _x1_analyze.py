#!/usr/bin/env python3
# Analyze _x1/perpair.csv for the X1 D0/D1 contrast (and mask-only decomposition).
# Pure stdlib: sum-ratio, pair-level geomean, bootstrap 95% CI, Wilcoxon signed-rank.
import csv, math, random, sys
from collections import defaultdict

random.seed(20260901)
PATH = sys.argv[1] if len(sys.argv) > 1 else "/home/sjp/_x1/perpair.csv"

rows = list(csv.DictReader(open(PATH)))
# index by (pair, mode)
d = {}
for r in rows:
    d[(r["pair"], r["mode"])] = r
pairs = sorted({r["pair"] for r in rows})
modes = ["on", "off", "mask-only"]

def f(r, k): return float(r[k])
def i(r, k): return int(float(r[k]))

def internal(r): return f(r, "arr_ms") + f(r, "fre_ms")

def geomean_logs(logs):
    return math.exp(sum(logs)/len(logs)) if logs else float("nan")

def bootstrap_ci(logs, B=10000, alpha=0.05):
    if not logs: return (float("nan"), float("nan"))
    n = len(logs); gms = []
    for _ in range(B):
        s = sum(logs[random.randrange(n)] for _ in range(n))
        gms.append(math.exp(s/n))
    gms.sort()
    return (gms[int(alpha/2*B)], gms[int((1-alpha/2)*B)])

def wilcoxon(diffs):
    # signed-rank on nonzero diffs; returns (W, n_effective, approx z, approx p two-sided)
    nz = [x for x in diffs if x != 0]
    n = len(nz)
    if n == 0: return (0, 0, 0.0, 1.0)
    order = sorted(range(n), key=lambda k: abs(nz[k]))
    ranks = [0.0]*n; k = 0
    while k < n:
        j = k
        while j+1 < n and abs(nz[order[j+1]]) == abs(nz[order[k]]): j += 1
        avg = (k+1 + j+1)/2.0
        for t in range(k, j+1): ranks[order[t]] = avg
        k = j+1
    Wp = sum(ranks[k] for k in range(n) if nz[k] > 0)
    Wm = sum(ranks[k] for k in range(n) if nz[k] < 0)
    W = min(Wp, Wm)
    mean = n*(n+1)/4.0
    sd = math.sqrt(n*(n+1)*(2*n+1)/24.0)
    z = (W - mean)/sd if sd > 0 else 0.0
    # two-sided normal approx
    p = math.erfc(abs(z)/math.sqrt(2))
    return (W, n, z, p)

def report_contrast(name, num_mode, den_mode, metric, active_only=True):
    # ratio = time(num)/time(den) per pair; e.g. M_D = off/on
    logs = []; num_sum = 0.0; den_sum = 0.0; used = 0; skipped = 0
    for p in pairs:
        rn = d.get((p, num_mode)); rd = d.get((p, den_mode))
        if not rn or not rd: continue
        vn = metric(rn); vd = metric(rd)
        if active_only and (i(rd, "builds") == 0 or i(rn, "builds") == 0):
            skipped += 1; continue
        if vn <= 0 or vd <= 0:
            skipped += 1; continue
        num_sum += vn; den_sum += vd
        logs.append(math.log(vn/vd)); used += 1
    if not logs:
        print(f"  [{name}] no usable pairs"); return
    ratios = sorted(math.exp(x) for x in logs)
    gm = geomean_logs(logs); lo, hi = bootstrap_ci(logs)
    med = ratios[len(ratios)//2]
    q1 = ratios[len(ratios)//4]; q3 = ratios[3*len(ratios)//4]
    W, ne, z, pval = wilcoxon(logs)
    print(f"  [{name}]  pairs_used={used} (skipped {skipped})")
    print(f"     sum-ratio      = {num_sum/den_sum:.3f}   (Σ{num_mode} / Σ{den_mode}, throughput view)")
    print(f"     pair geomean   = {gm:.3f}   95%CI [{lo:.3f}, {hi:.3f}]   (typical-pair view)")
    print(f"     median={med:.3f}  IQR[{q1:.3f},{q3:.3f}]  min={ratios[0]:.3f}  max={ratios[-1]:.3f}")
    print(f"     Wilcoxon signed-rank on log-ratios: W={W:.0f} n={ne} z={z:.2f} p~{pval:.2e}")
    return logs

print(f"== X1 analysis: {PATH} ==")
print(f"total pairs={len(pairs)}")
# answer consistency check
mism = 0
for p in pairs:
    on = d.get((p,"on")); off = d.get((p,"off"))
    if on and off and on["ans"] and off["ans"]:
        try:
            if abs(float(on["ans"]) - float(off["ans"])) > 1e-6: mism += 1
        except ValueError: pass
print(f"answer mismatches (|off-on|>1e-6): {mism}")
nbuild0 = sum(1 for p in pairs if d.get((p,"on")) and i(d[(p,"on")],"builds")==0)
print(f"pairs with builds==0 on 'on' (arrangement never engaged): {nbuild0}")

print("\n-- M_D = D0/D1 (off/on), INTERNAL (arr+fre), active pairs --")
mlog_internal = report_contrast("M_D internal", "off", "on", internal)
print("\n-- M_D = D0/D1 (off/on), PREDICATE only (fre_ms), active pairs --")
report_contrast("M_D predicate", "off", "on", lambda r: f(r,"fre_ms"))
print("\n-- M_D = D0/D1 (off/on), WALL (end-to-end proxy), ALL pairs --")
report_contrast("M_D wall", "off", "on", lambda r: f(r,"wall_ms"), active_only=False)

print("\n-- mask cost isolation: mask-only/off INTERNAL (should show mask pass cost) --")
report_contrast("mask overhead", "mask-only", "off", internal)

# aggregate query / C_q per mode (active pairs)
print("\n-- aggregate counts (active pairs, builds>0) --")
for mode in modes:
    q=0; fre=0.0; m=0; K=0; b=0
    for p in pairs:
        r=d.get((p,mode))
        if not r or i(r,"builds")==0: continue
        q+=i(r,"queries"); fre+=f(r,"fre_ms"); m+=i(r,"m"); K+=i(r,"K"); b+=i(r,"builds")
    cq = fre*1e6/q if q else 0
    print(f"   {mode:9s}: builds={b} m={m} K={K} K/m={ (K/m if m else 0):.3f} queries={q} fre_ms={fre:.0f} C_q={cq:.0f}ns")
