#!/usr/bin/env python3
# X3-b: M_D = D0/D1 (off/on) vs arrangement_cut_limit. Does maximal reduction help
# more as leaf arrangements grow? Geomean + bootstrap CI over completed pairs.
import csv, math, random, sys
from collections import defaultdict
random.seed(1)
PATH = sys.argv[1] if len(sys.argv) > 1 else "/home/sjp/_x1/x3b.csv"
rows = list(csv.DictReader(open(PATH)))
d = {}
for r in rows: d[(r["cut_limit"], r["pair"], r["mode"])] = r
cuts = sorted({r["cut_limit"] for r in rows}, key=int)
pairs = sorted({r["pair"] for r in rows})
def boot(logs,B=5000,a=0.05):
    if not logs: return (float('nan'),)*2
    n=len(logs); g=[]
    for _ in range(B):
        s=sum(logs[random.randrange(n)] for _ in range(n)); g.append(math.exp(s/n))
    g.sort(); return (g[int(a/2*B)], g[int((1-a/2)*B)])
print(f"== X3-b M_D vs cut_limit: {PATH} ==")
print(f"{'cut':>4} {'pairs':>5} {'cens':>4} {'M_D_wall':>18} {'M_D_internal':>18}")
for cut in cuts:
    def metric(mode, key):
        v=[]
        for p in pairs:
            on=d.get((cut,p,'on')); off=d.get((cut,p,'off'))
            if not on or not off or on['status']!='OK' or off['status']!='OK': continue
            try:
                if key=='wall': vn=float(off['wall_ms']); vd=float(on['wall_ms'])
                else: vn=float(off['arr_ms'])+float(off['fre_ms']); vd=float(on['arr_ms'])+float(on['fre_ms'])
            except ValueError: continue
            if vn>0 and vd>0: v.append(math.log(vn/vd))
        return v
    lw=metric('','wall'); li=metric('','internal')
    cens=sum(1 for p in pairs for m in ('on','off') if d.get((cut,p,m)) and d[(cut,p,m)]['status']!='OK')
    def fmt(l):
        if not l: return "n/a"
        g=math.exp(sum(l)/len(l)); lo,hi=boot(l); return f"{g:.3f}[{lo:.2f},{hi:.2f}]n={len(l)}"
    print(f"{cut:>4} {len(pairs):>5} {cens:>4} {fmt(lw):>18} {fmt(li):>18}")
print("M_D>1 ⇒ maximal helps; rising with cut_limit ⇒ larger arrangements benefit more (break-even trend)")
