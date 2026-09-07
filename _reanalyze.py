#!/usr/bin/env python3
# R1-R3 re-analysis (재분석 지침서). Pure stdlib. Reads existing CSVs only.
import csv, math, sys
from collections import defaultdict
RES = "/mnt/c/Users/shiju/Downloads/Discrete-Frechet-distance-under-translation-master/Discrete-Frechet-distance-under-translation-master/_x1_results"

# ---------- pure-python stats ----------
def mean(x): return sum(x)/len(x) if x else float('nan')
def sd(x):
    n=len(x)
    if n<2: return float('nan')
    m=mean(x); return math.sqrt(sum((v-m)**2 for v in x)/(n-1))
def betacf(a,b,x):
    MAXIT=200; EPS=3e-12; FPMIN=1e-300
    qab=a+b; qap=a+1; qam=a-1; c=1.0; d=1-qab*x/qap
    if abs(d)<FPMIN: d=FPMIN
    d=1/d; h=d
    for m in range(1,MAXIT+1):
        m2=2*m
        aa=m*(b-m)*x/((qam+m2)*(a+m2)); d=1+aa*d
        if abs(d)<FPMIN: d=FPMIN
        c=1+aa/c
        if abs(c)<FPMIN: c=FPMIN
        d=1/d; h*=d*c
        aa=-(a+m)*(qab+m)*x/((a+m2)*(qap+m2)); d=1+aa*d
        if abs(d)<FPMIN: d=FPMIN
        c=1+aa/c
        if abs(c)<FPMIN: c=FPMIN
        d=1/d; de=d*c; h*=de
        if abs(de-1)<EPS: break
    return h
def betai(a,b,x):
    if x<=0: return 0.0
    if x>=1: return 1.0
    lb=math.lgamma(a+b)-math.lgamma(a)-math.lgamma(b)+a*math.log(x)+b*math.log(1-x)
    bt=math.exp(lb)
    if x<(a+1)/(a+b+2): return bt*betacf(a,b,x)/a
    return 1-bt*betacf(b,a,1-x)/b
def t_sf_two(t,df):  # two-sided p-value for Student t
    if df<=0: return float('nan')
    x=df/(df+t*t); return betai(df/2,0.5,x)
def ttest_1samp(x,mu):
    n=len(x); m=mean(x); s=sd(x)
    if n<2 or s==0: return (float('nan'),float('nan'))
    t=(m-mu)/(s/math.sqrt(n)); return (t, t_sf_two(t,n-1))
def wilcoxon(diffs):
    nz=[d for d in diffs if d!=0]; n=len(nz)
    if n==0: return (0,0,1.0)
    order=sorted(range(n),key=lambda k:abs(nz[k])); ranks=[0.0]*n; k=0
    while k<n:
        j=k
        while j+1<n and abs(nz[order[j+1]])==abs(nz[order[k]]): j+=1
        avg=(k+1+j+1)/2.0
        for t in range(k,j+1): ranks[order[t]]=avg
        k=j+1
    Wp=sum(ranks[k] for k in range(n) if nz[k]>0); Wm=sum(ranks[k] for k in range(n) if nz[k]<0)
    W=min(Wp,Wm); meanW=n*(n+1)/4; sdW=math.sqrt(n*(n+1)*(2*n+1)/24)
    z=(W-meanW)/sdW if sdW>0 else 0; p=math.erfc(abs(z)/math.sqrt(2))
    return (W,n,p)
def ols(xs,ys):
    n=len(xs); mx=mean(xs); my=mean(ys)
    sxx=sum((x-mx)**2 for x in xs)
    if sxx==0: return (float('nan'),float('nan'))
    b=sum((xs[i]-mx)*(ys[i]-my) for i in range(n))/sxx; a=my-b*mx
    return (a,b)

# ==================== R1 — Amdahl ceiling ====================
print("="*70); print("R1 — AMDAHL CEILING (factor_perpair_geolife100.csv)"); print("="*70)
rows=list(csv.DictReader(open(f"{RES}/factor_perpair_geolife100.csv")))
by=defaultdict(dict)
for r in rows: by[r["pair"]][r["arm"]]=r
ARMS=["A0","B0","D0","D1"]
comp=[p for p in by if all(a in by[p] and by[p][a]["status"]=="OK" for a in ARMS)]
def num(r,k):
    try: return float(r[k])
    except: return float('nan')
W={a:sum(num(by[p][a],"wall_ms") for p in comp) for a in ARMS}
I={a:sum(num(by[p][a],"arr_ms")+num(by[p][a],"fre_ms") for p in comp) for a in ARMS}
n_exec=len(comp)
print(f"all-4-complete pairs = {n_exec}")
for a in ARMS: print(f"  {a}: Wall={W[a]:.0f}ms  I(arr+fre)={I[a]:.0f}ms")
def solve_C(hi,lo):   # (C+I_hi)/(C+I_lo)=W_hi/W_lo
    R=W[hi]/W[lo]
    if abs(R-1)<1e-9: return float('nan')
    return (I[hi]-R*I[lo])/(R-1)
print("\n  C solved 3 ways (should agree if C is arm-invariant):")
for hi,lo in [("A0","D1"),("A0","D0"),("B0","D0")]:
    C=solve_C(hi,lo); print(f"    ({hi},{lo}): C={C:.0f}ms")
C=solve_C("A0","D1")
print(f"\n  Using C from (A0,D1) = {C:.0f}ms")
print(f"  WSL/startup overhead in C (native ~geodata; ~1.4s/exec only on /mnt/c):")
print(f"     over {n_exec} pairs, worst-case 1.4s/exec ⇒ {1.4*n_exec:.0f}s if on /mnt/c (we used ~/geodata)")
for a in ARMS:
    share=I[a]/(C+I[a]) if (C+I[a])>0 else float('nan')
    ceil=1/(1-share) if share<1 else float('inf')
    print(f"  {a}: internal share={share:.1%}  end-to-end ceiling=1/(1-share)={ceil:.3f}x")
shareD1=I["D1"]/(C+I["D1"])
print(f"\n  >> CEILING (D1): even a zero-cost inner loop caps end-to-end speedup at {1/(1-shareD1):.3f}x")

# ==================== R2 — per-pair gamma + level ====================
print("\n"+"="*70); print("R2 — PER-PAIR γ + LEVEL COMPARISON (x3a_sweep.csv)"); print("="*70)
d=list(csv.DictReader(open(f"{RES}/x3a_sweep.csv")))
conv={}
for r in d: conv[r["path"]]=r        # converged (last) row per tag dataset:nNN:pP
recs=[]
for tag,r in conv.items():
    ds,nl,pl=tag.split(":")
    recs.append((ds,int(nl[1:]),int(pl[1:]),int(r["N"]),float(r["K_exact_distinct"]),float(r["V_pairs"]),float(r["K_band_distinct"])))
# per-pair gamma
print("\n Method A — per-pair γ (slope of log K_exact_distinct vs log N), t-test vs 2.0:")
gammas={}
for ds in ("geo","syn"):
    g=[]
    pids=sorted({p for (d0,nn,p,N,K,V,Kb) in recs if d0==ds})
    for p in pids:
        pts=sorted([(N,K) for (d0,nn,pp,N,K,V,Kb) in recs if d0==ds and pp==p and K>0])
        if len(pts)>=3:
            a,b=ols([math.log(N) for N,K in pts],[math.log(K) for N,K in pts]); g.append(b)
    gammas[ds]=g
    t,pt=ttest_1samp(g,2.0); W,nn,pw=wilcoxon([x-2.0 for x in g])
    ci=(mean(g)-1.96*sd(g)/math.sqrt(len(g)), mean(g)+1.96*sd(g)/math.sqrt(len(g)))
    print(f"   {ds}: n={len(g)} mean_γ={mean(g):.3f} sd={sd(g):.3f} CI=({ci[0]:.3f},{ci[1]:.3f})  t_p(vs2)={pt:.2g} wilcoxon_p={pw:.2g}")
# paired geo vs syn (same pair index)
common=sorted(set(p for p in range(50) if gammas))
gg=[]; gs=[]
for p in range(50):
    a=[b for (d0,nn,pp,N,K,V,Kb) in [] ]  # placeholder
# paired by pair index
pg={p:None for p in range(50)}
for ds in ("geo","syn"):
    for p in sorted({pp for (d0,nn,pp,N,K,V,Kb) in recs if d0==ds}):
        pts=sorted([(N,K) for (d0,nn,pp2,N,K,V,Kb) in recs if d0==ds and pp2==p and K>0])
        if len(pts)>=3:
            _,b=ols([math.log(N) for N,K in pts],[math.log(K) for N,K in pts])
            pg.setdefault((ds,p),b)
pairs_common=sorted({p for (ds,p) in pg if ds=='geo' and ('syn',p) in pg} if False else [])
gp=[pg[('geo',p)] for p in range(50) if ('geo',p) in pg and ('syn',p) in pg]
sp=[pg[('syn',p)] for p in range(50) if ('geo',p) in pg and ('syn',p) in pg]
if gp:
    W,nn,pw=wilcoxon([sp[i]-gp[i] for i in range(len(gp))])
    print(f"   paired γ(syn)-γ(geo): n={len(gp)} mean_diff={mean([sp[i]-gp[i] for i in range(len(gp))]):.3f} wilcoxon_p={pw:.2g}")
# Method B — level ratio at matched N
print("\n Method B — K_exact_distinct/N² level, geo vs syn at matched N (paired by N):")
Nset=sorted({N for (d0,nn,p,N,K,V,Kb) in recs})
geo_med=[]; syn_med=[]
print(f"   {'N':>5} {'geo_med':>9} {'syn_med':>9} {'ratio(syn/geo)':>14}")
for N in Nset:
    gk=sorted([K for (d0,nn,p,N2,K,V,Kb) in recs if d0=='geo' and N2==N])
    sk=sorted([K for (d0,nn,p,N2,K,V,Kb) in recs if d0=='syn' and N2==N])
    if gk and sk:
        gm=gk[len(gk)//2]; sm=sk[len(sk)//2]; geo_med.append(gm); syn_med.append(sm)
        print(f"   {N:>5} {gm:>9.0f} {sm:>9.0f} {sm/gm:>14.2f}")
if geo_med:
    W,nn,pw=wilcoxon([syn_med[i]-geo_med[i] for i in range(len(geo_med))])
    print(f"   Wilcoxon(syn vs geo medians across N): n={nn} p={pw:.2g}  median ratio={mean([syn_med[i]/geo_med[i] for i in range(len(geo_med))]):.2f}")

# ==================== R3 — density confound ====================
print("\n"+"="*70); print("R3 — DENSITY CONFOUND (x3a_sweep.csv)"); print("="*70)
print(f"   {'ds':>4} {'n':>3} {'N':>5} {'density=2V/N':>12} {'frac':>8}")
dens_by=defaultdict(list)
for ds in ("geo","syn"):
    for N in Nset:
        recN=[(V) for (d0,nn,p,N2,K,V,Kb) in recs if d0==ds and N2==N]
        if not recN: continue
        V=sorted(recN)[len(recN)//2]
        n=int(round(math.sqrt(N)))
        density=2*V/N; frac=V/(N*(N-1)/2)
        dens_by[ds].append((N,density))
        print(f"   {ds:>4} {n:>3} {N:>5} {density:>12.2f} {frac:>8.4f}")
for ds in ("geo","syn"):
    pts=dens_by[ds]
    if len(pts)>=2:
        _,b=ols([math.log(N) for N,de in pts],[math.log(de) for N,de in pts])
        print(f"   {ds}: d log(density)/d log(N) = {b:+.3f}  ({'rising' if b>0.1 else 'flat' if abs(b)<=0.1 else 'falling'})")
