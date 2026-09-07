#!/usr/bin/env python3
# R5 (ε-degenerate maximal families) + R6 (n_isolated) from x3a_sweep.csv.
import csv, math
from collections import defaultdict
P="/home/sjp/_x1/x3a_sweep.csv"
rows=list(csv.DictReader(open(P)))
def g(r,k): return float(r[k])
print(f"== R5/R6 ({len(rows)} builds) ==  columns: {list(rows[0].keys())}")
# group by dataset,n
agg=defaultdict(lambda: defaultdict(float))
for r in rows:
    ds,nl,_=r["path"].split(":"); key=(ds,int(nl[1:]))
    a=agg[key]
    a["builds"]+=1
    a["hi"]+=g(r,"K_band_distinct"); a["lo"]+=g(r,"K_band_distinct_lo")
    a["Kex"]+=g(r,"K_exact_distinct")
    a["isol"]+=g(r,"n_isolated"); a["isol_max"]=max(a["isol_max"],g(r,"n_isolated"))
    a["N"]=g(r,"N")
print("\n R5 — ε-degenerate (boundary-dependent) maximal families: (M_hi - M_lo)/M_hi")
print(f"   {'ds':>4} {'n':>3} {'N':>5} {'ΣM_hi':>8} {'ΣM_lo':>8} {'eps_rate':>9} {'ΣK_exact':>9}")
for ds in ("geo","syn"):
    for n in sorted(k[1] for k in agg if k[0]==ds):
        a=agg[(ds,n)]; hi=a["hi"]; lo=a["lo"]
        eps=(hi-lo)/hi if hi else 0
        print(f"   {ds:>4} {n:>3} {int(a['N']):>5} {hi:>8.0f} {lo:>8.0f} {eps:>9.3f} {a['Kex']:>9.0f}")
print("\n R6 — isolated discs (Claim V |S|=1 exception): n_isolated")
print(f"   {'ds':>4} {'n':>3} {'builds':>7} {'Σn_isolated':>12} {'max_per_build':>13}")
for ds in ("geo","syn"):
    for n in sorted(k[1] for k in agg if k[0]==ds):
        a=agg[(ds,n)]
        print(f"   {ds:>4} {n:>3} {int(a['builds']):>7} {int(a['isol']):>12} {int(a['isol_max']):>13}")
tot_isol=sum(agg[k]['isol'] for k in agg)
print(f"\n   TOTAL isolated discs across all builds = {int(tot_isol)}"
      f"  ⇒ {'|S|>=2 hypothesis SUFFICES (no gap)' if tot_isol==0 else 'COMPLETENESS GAP — add isolated-disc centers'}")
