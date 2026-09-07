#!/usr/bin/env python3
# X3-a / X5 analysis from x3a_sweep.csv. Per (dataset,n,pair) take the converged
# (last) build row. Report per dataset: γ = slope of log(K_exact_distinct) vs
# log(N), band over-return K_band_distinct/K_exact_distinct, degeneracy ratio
# n_degen_geom/K_exact_distinct, and K_exact_distinct/N² vs the proven bound N²/3.
import csv, math, sys
from collections import defaultdict
PATH = sys.argv[1] if len(sys.argv) > 1 else "/home/sjp/_x1/x3a_sweep.csv"
rows = list(csv.DictReader(open(PATH)))

# converged row per tag (last row wins)
conv = {}
for r in rows: conv[r["path"]] = r
# group by (dataset, n)
by = defaultdict(list)
for tag, r in conv.items():
    ds, nlbl, _ = tag.split(":")
    by[(ds, int(nlbl[1:]))].append(r)

def ols(xs, ys):
    n=len(xs); mx=sum(xs)/n; my=sum(ys)/n
    sxx=sum((x-mx)**2 for x in xs); sxy=sum((xs[i]-mx)*(ys[i]-my) for i in range(n))
    b=sxy/sxx; a=my-b*mx
    # R^2
    yhat=[a+b*x for x in xs]; sst=sum((y-my)**2 for y in ys); sse=sum((ys[i]-yhat[i])**2 for i in range(n))
    r2=1-sse/sst if sst>0 else float('nan')
    # slope SE for CI
    se=math.sqrt(sse/(n-2)/sxx) if n>2 and sxx>0 else float('nan')
    return a,b,r2,se

for ds in ("geo","syn"):
    print(f"\n================ dataset = {ds} ================")
    print(f"{'n':>3} {'N':>5} {'pairs':>5} {'V_pairs':>8} {'K_band_dist':>11} {'K_exact_dist':>12} {'overret':>7} {'Ke/N^2':>9} {'bound N^2/3':>11} {'degen_ratio':>11}")
    logN=[]; logK=[]
    for (d,n),rs in sorted(by.items()):
        if d!=ds: continue
        N=n*n
        vp=sum(int(r["V_pairs"]) for r in rs)/len(rs)
        kbd=sum(int(r["K_band_distinct"]) for r in rs)/len(rs)
        ked=sum(int(r["K_exact_distinct"]) for r in rs)/len(rs)
        dg =sum(int(r["n_degen_geom"]) for r in rs)/len(rs)
        overret = kbd/ked if ked else 0
        ratioN2 = ked/(N*N) if N else 0     # vs proven upper bound ~N^2/3 (leading)
        bound   = (N*N)/3.0
        degr = dg/ked if ked else 0
        viol = " <==OVER BOUND" if ked>bound else ""
        print(f"{n:>3} {N:>5} {len(rs):>5} {vp:>8.0f} {kbd:>11.1f} {ked:>12.1f} {overret:>7.2f} {ratioN2:>9.4f} {bound:>11.0f} {degr:>11.3f}{viol}")
        if ked>0: logN.append(math.log(N)); logK.append(math.log(ked))
    if len(logN)>=2:
        a,b,r2,se = ols(logN,logK)
        lo,hi = b-1.96*se, b+1.96*se
        print(f"  γ (slope of log K_exact_distinct vs log N) = {b:.3f}  95%CI[{lo:.3f},{hi:.3f}]  R²={r2:.3f}")
        print(f"  (γ<2 ⇒ input-sensitive: real data below worst-case Θ(N²) cell count)")
