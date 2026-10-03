import csv, os, sys
# usage: cmp_dec.py [paperq|paperq4]  -- section 4.4 table: authors' shipped decider outputs vs original vs GitLab
#   ORIGDEC: original's CSVs of the same files (default ~/r4dec/original, r4 run_decider.sh)
#   GITDEC:  GitLab's CSVs (default ~/gitdec/gitlab, run_dec_gitlab.sh)
ROOT=os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)),"..","..",".."))
R=os.path.join(ROOT,"authors_gitlab")
H=os.path.expanduser("~")
ORIGDEC=os.environ.get("ORIGDEC",f"{H}/r4dec/original"); GITDEC=os.environ.get("GITDEC",f"{H}/gitdec/gitlab")
order=[(l,"minus") for l in range(-1,-11,-1)]+[(l,"plus") for l in range(-10,3)]
def authors(name):
    rows=[l.split() for l in open(f"{R}/experiments/{name}.txt") if l.strip()]
    return {order[i]:(float(r[0]),float(r[3])) for i,r in enumerate(rows)}
def load(d,name):
    p=f"{d}/{name}.csv"
    if not os.path.exists(p): return None
    return list(csv.DictReader(open(p)))
tag=sys.argv[1] if len(sys.argv)>1 else "paperq"
sets=[("characters_uci_same","same-characters"),("characters_uci_all","all-characters"),("sigspatial","sigspatial")]
for s,an in sets:
    A=authors(an) if tag=="paperq" else None
    print(f"\n## {s} ({tag})")
    print("| set | authors' shipped calls | original (repo) calls | authors code (GitLab) calls | answers differ orig vs GitLab | ms orig / GitLab |")
    print("|---|--:|--:|--:|--:|--:|")
    tot=[0,0,0,0,0,0]
    for l,k in order:
        name=f"{s}_{tag}_{l}_{k}"
        o=load(ORIGDEC,name); a=load(GITDEC,name)
        if o is None or a is None: continue
        oc=sum(int(r["bbcalls"]) for r in o)/len(o); ac=sum(int(r["bbcalls"]) for r in a)/len(a)
        om=sum(float(r["time_ms"]) for r in o)/len(o); am=sum(float(r["time_ms"]) for r in a)/len(a)
        diff=sum(x["answer"]!=y["answer"] for x,y in zip(o,a))
        au=f"{A[(l,k)][1]:.2f}" if A else "-"
        print(f"| {l} {k} | {au} | {oc:.2f} | {ac:.2f} | {diff} | {om:.3f} / {am:.3f} |")
        tot[0]+=A[(l,k)][1] if A else 0; tot[1]+=oc; tot[2]+=ac; tot[3]+=diff; tot[4]+=om; tot[5]+=am
    print(f"| **all** | {tot[0]/23:.2f} | {tot[1]/23:.2f} | {tot[2]/23:.2f} | {tot[3]} | {tot[4]/23:.3f} / {tot[5]/23:.3f} |")
