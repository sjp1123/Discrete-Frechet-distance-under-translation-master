#!/usr/bin/env python3
# C_q–ply analysis (NEXT §3.4): from x3q.csv (ply,t_query_ns,is_maximal), test the
# work-asymmetry proposition — maximal candidates carry the higher-ply, costlier
# queries, so ρ_work < ρ_count. Reports C_q(ply) regression + maximal-vs-not means.
import csv, math, sys
from collections import defaultdict
PATH = sys.argv[1] if len(sys.argv) > 1 else "/home/sjp/_x1/x3q.csv"
rows = [r for r in csv.DictReader(open(PATH))]
ply = [int(r["ply"]) for r in rows]
t   = [int(r["t_query_ns"]) for r in rows]
mx  = [int(r["is_maximal"]) for r in rows]
n = len(rows)
print(f"== C_q–ply analysis: {PATH} ==  queries={n}")
if n < 10: print("too few rows"); sys.exit(0)

# means by group
def mean(xs): return sum(xs)/len(xs) if xs else float('nan')
t_max  = [t[i] for i in range(n) if mx[i]];  p_max  = [ply[i] for i in range(n) if mx[i]]
t_nmax = [t[i] for i in range(n) if not mx[i]]; p_nmax = [ply[i] for i in range(n) if not mx[i]]
print(f"  maximal    : count={len(t_max):6d}  mean_ply={mean(p_max):6.2f}  mean_C_q={mean(t_max):8.0f} ns")
print(f"  non-maximal: count={len(t_nmax):6d}  mean_ply={mean(p_nmax):6.2f}  mean_C_q={mean(t_nmax):8.0f} ns")
if t_nmax and t_max:
    print(f"  C_q(maximal)/C_q(non-maximal) = {mean(t_max)/mean(t_nmax):.2f}   ply ratio = {mean(p_max)/mean(p_nmax):.2f}")

# C_q vs ply regression (OLS)
mp=mean(ply); mt=mean(t)
sxx=sum((x-mp)**2 for x in ply); sxy=sum((ply[i]-mp)*(t[i]-mt) for i in range(n))
b=sxy/sxx if sxx else float('nan'); a=mt-b*mp
yh=[a+b*ply[i] for i in range(n)]; sst=sum((x-mt)**2 for x in t); sse=sum((t[i]-yh[i])**2 for i in range(n))
r2=1-sse/sst if sst else float('nan')
print(f"  C_q(ply) OLS: C_q ≈ {a:.0f} + {b:.1f}·ply ns   R²={r2:.3f}  (positive slope ⇒ deeper cells cost more)")

# per-ply-bucket mean C_q
buck=defaultdict(list)
for i in range(n): buck[ply[i]].append(t[i])
print("  ply : n     mean_C_q(ns)")
for pv in sorted(buck)[:20]:
    xs=buck[pv]; print(f"   {pv:3d} : {len(xs):5d}  {mean(xs):8.0f}")

# work-vs-count reduction (within this sample; short-circuit-biased, cite X1 too)
sum_all=sum(t); sum_max=sum(t_max)
if sum_max:
    print(f"  ρ_count (all/maximal by count) = {n/max(1,len(t_max)):.2f}")
    print(f"  ρ_work  (Σt_all / Σt_maximal)  = {sum_all/sum_max:.2f}   (ρ_work<ρ_count ⇒ asymmetry)")
