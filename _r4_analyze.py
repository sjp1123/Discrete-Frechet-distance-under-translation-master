#!/usr/bin/env python3
# R4 — C_q ~ ply^alpha with a BUILD fixed-effects regression (removes the N
# confound: deep-ply candidates cluster in big subproblems whose queries are
# dearer). Pure stdlib. Reads x3q.csv (build_id,ply,t_query_ns,is_maximal).
import csv, math
from collections import defaultdict
P = "/home/sjp/_x1/x3q.csv"
rows = [r for r in csv.DictReader(open(P))]
lp=[]; lt=[]; bid=[]; ismax=[]; ply=[]
for r in rows:
    p=float(r["ply"]); t=float(r["t_query_ns"])
    if p>0 and t>0:
        lp.append(math.log(p)); lt.append(math.log(t)); bid.append(r["build_id"])
        ismax.append(int(r["is_maximal"])); ply.append(p)
n=len(lp)
def mean(x): return sum(x)/len(x)
def ols(xs,ys):
    mx=mean(xs); my=mean(ys); sxx=sum((x-mx)**2 for x in xs)
    b=sum((xs[i]-mx)*(ys[i]-my) for i in range(len(xs)))/sxx
    return my-b*mx, b, sxx
print(f"== R4: C_q~ply^alpha  (n={n} queries, {len(set(bid))} builds) ==")
# (a) pooled
a,b,_=ols(lp,lt); print(f"  (a) pooled          alpha = {b:.3f}")
# (b) within-build fixed effects: demean within build_id
gp=defaultdict(list); gt=defaultdict(list)
for i in range(n): gp[bid[i]].append(lp[i]); gt[bid[i]].append(lt[i])
mp={k:mean(v) for k,v in gp.items()}; mt={k:mean(v) for k,v in gt.items()}
dlp=[lp[i]-mp[bid[i]] for i in range(n)]; dlt=[lt[i]-mt[bid[i]] for i in range(n)]
Sxx=sum(x*x for x in dlp); Sxy=sum(dlp[i]*dlt[i] for i in range(n))
beta=Sxy/Sxx
# residuals & homoskedastic SE, dof = n - n_builds - 1
resid=[dlt[i]-beta*dlp[i] for i in range(n)]
ssr=sum(e*e for e in resid); dof=n-len(set(bid))-1
s2=ssr/dof; se=math.sqrt(s2/Sxx)
print(f"  (b) within-build FE alpha = {beta:.3f}  95%CI[{beta-1.96*se:.3f},{beta+1.96*se:.3f}]  (dof={dof})")
# (c) per-build slopes (builds with >=3 distinct ply)
slopes=[]
for k in gp:
    xs=gp[k]; ys=gt[k]
    if len(set(round(x,6) for x in xs))>=3:
        mx=mean(xs); sxx=sum((x-mx)**2 for x in xs)
        if sxx>0:
            my=mean(ys); slopes.append(sum((xs[i]-mx)*(ys[i]-my) for i in range(len(xs)))/sxx)
slopes.sort()
if slopes:
    med=slopes[len(slopes)//2]; q1=slopes[len(slopes)//4]; q3=slopes[3*len(slopes)//4]
    print(f"  (c) per-build slopes: n={len(slopes)} median={med:.3f} IQR[{q1:.3f},{q3:.3f}]")
# rho_work prediction with the within-build alpha
alpha=beta
num=sum(p**alpha for p in ply); den=sum(ply[i]**alpha for i in range(n) if ismax[i])
rho_count=n/sum(ismax)
print(f"\n  rho_count (X3q, all/maximal by count) = {rho_count:.3f}")
print(f"  rho_work  observed  (Σt_all/Σt_max)   = {sum(math.exp(lt[i]) for i in range(n))/sum(math.exp(lt[i]) for i in range(n) if ismax[i]):.3f}")
print(f"  rho_work  predicted (Σply^α/Σmax ply^α, α={alpha:.3f}) = {num/den:.3f}")
# reconcile with X1's 3.69
print(f"\n  NOTE reconcile: X1 reported rho_count=3.69 (fut_lmf survivor-point ratio m/K);")
print(f"  X3q rho_count={rho_count:.3f} here is queries(all)/queries(maximal) — different denominator.")
# ply distribution of maximal vs non-maximal
mx_ply=[ply[i] for i in range(n) if ismax[i]]; nm_ply=[ply[i] for i in range(n) if not ismax[i]]
print(f"  mean ply: maximal={mean(mx_ply):.2f} non-maximal={mean(nm_ply):.2f}")
