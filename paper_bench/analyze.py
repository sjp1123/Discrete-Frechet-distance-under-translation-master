#!/usr/bin/env python3
"""Summarise paper_bench/results/*.csv into paper_bench/results/RESULTS.md.

Protocol: per query the MIN time over reps per arm; ratios are original/candidate5
(>1 = candidate5 faster).  Geomean of per-query log-ratios with a bootstrap 95% CI,
sum-ratio (total time), median, and the fraction of queries where candidate5 is
faster.  Correctness: LMF values |delta| against original with eps = 1e-7 (the tree's
convention, see candidate5/README.candidate5.md section 6); decider answers must equal
the expected answer (plus -> yes, minus -> no) for both arms and agree with each other.
"""
import csv, glob, math, os, random, statistics, sys, collections
here = os.path.dirname(os.path.abspath(__file__)); R = os.path.join(here, "results")
ARMS = ["original", "candidate5"]
DATASETS = [("characters_all", "Characters, all-characters pairs (2858 curves, 2000 pairs)"),
            ("characters_same", "Characters, same-character pairs (1429 labelled curves, 1000 pairs)"),
            ("sigspatial_subset", "Sigspatial, 101-curve GIS Cup sample subset (1000 pairs)")]
LS = [(l, "plus") for l in range(-10, 3)] + [(l, "minus") for l in range(-10, 0)]
rng = random.Random(1)

def load(pattern):
    """{(f1,f2[,dist]) -> row with min time over reps}, reps count"""
    files = sorted(glob.glob(pattern)); best = {}
    for fn in files:
        for r in csv.DictReader(open(fn)):
            k = (r["file1"], r["file2"], r.get("distance", ""))
            t = float(r["time_ms"])
            if k not in best or t < float(best[k]["time_ms"]): best[k] = r
    return best, len(files)

def geomean_ci(ratios, B=5000):
    logs = [math.log(x) for x in ratios if x > 0]
    g = math.exp(sum(logs)/len(logs)); bs = []
    for _ in range(B):
        s = [logs[rng.randrange(len(logs))] for _ in logs]; bs.append(math.exp(sum(s)/len(s)))
    bs.sort(); return g, bs[int(0.025*B)], bs[int(0.975*B)]

out = []
P = out.append
P("# original vs candidate5 on the paper's data sets\n")
P("Times: per-query MIN over reps, one core, in-process driver (`paper_bench`). Ratio = original / candidate5 (>1 = candidate5 faster).\n")
for ds, title in DATASETS:
    o, no = load(f"{R}/{ds}_lmf_original_r*.csv"); c, nc = load(f"{R}/{ds}_lmf_candidate5_r*.csv")
    if not o or not c: continue
    keys = [k for k in o if k in c]
    P(f"## {title}\n")
    P(f"### Value computation (LMF, `calcDistance2`) — {len(keys)} pairs, reps original={no} candidate5={nc}\n")
    # correctness
    d = [abs(float(o[k]["value"]) - float(c[k]["value"])) for k in keys]
    over = sum(1 for x in d if x > 1e-7); ident = sum(1 for x in d if x == 0)
    P(f"Answer agreement: max |Δ| = {max(d):.3e}, mean |Δ| = {sum(d)/len(d):.3e}, pairs over ε=1e-7: **{over}/{len(keys)}**, bit-identical: {ident}/{len(keys)}\n")
    # timing
    def tot(m, col): return sum(float(m[k][col]) for k in keys)
    rows = [("wall (LMF total)", "time_ms", "ms"), ("n6 arrangement stage", "n6_arr_ms", "ms"), ("n6 Fréchet (decider inside n6)", "n6_fre_ms", "ms"),
            ("Arrangement 2 (whole arrangement phase)", "arr2_ms", "ms"), ("Blackbox 2", "bb2_ms", "ms"), ("black-box calls", "bbcalls", "")]
    P("| metric | original | candidate5 | sum-ratio | geomean ratio [95% CI] | median | candidate5 faster |")
    P("|---|--:|--:|--:|--:|--:|--:|")
    for name, col, unit in rows:
        to, tc = tot(o, col), tot(c, col)
        ratios = [float(o[k][col]) / float(c[k][col]) for k in keys if float(o[k][col]) > 0 and float(c[k][col]) > 0]
        if len(ratios) >= 10:
            g, lo, hi = geomean_ci(ratios); med = statistics.median(ratios); faster = sum(1 for x in ratios if x > 1)
            gs = f"{g:.3f} [{lo:.3f}, {hi:.3f}]"; ms = f"{med:.3f}"; fs = f"{faster}/{len(ratios)}"
        else: gs = ms = fs = "–"
        fmt = (lambda v: f"{v:,.0f}") if col == "bbcalls" else (lambda v: f"{v/1000:.2f} s" if v > 5000 else f"{v:.1f} ms")
        P(f"| {name} | {fmt(to)} | {fmt(tc)} | {to/tc:.3f} | {gs} | {ms} | {fs} |")
    mo = statistics.mean(float(o[k]['time_ms']) for k in keys); mc = statistics.mean(float(c[k]['time_ms']) for k in keys)
    P(f"\nMean per pair: original {mo:.2f} ms, candidate5 {mc:.2f} ms.\n")
    # decider
    P(f"### Decision problem (`lessThan`), paper protocol: δ*(1±2^l), 1000 pairs per set\n")
    P("| set | expected | original ms/query | candidate5 ms/query | ratio | bb calls orig | bb calls c5 | wrong (orig / c5) | arms disagree |")
    P("|---|---|--:|--:|--:|--:|--:|--:|--:|")
    T = {a: 0.0 for a in ARMS}; W = {a: 0 for a in ARMS}; N = 0; BB = {a: 0 for a in ARMS}; DIS = 0
    for l, sign in LS:
        do, _ = load(f"{R}/{ds}_decider_{l}_{sign}_original_r*.csv"); dc, _ = load(f"{R}/{ds}_decider_{l}_{sign}_candidate5_r*.csv")
        ks = [k for k in do if k in dc]
        if not ks: continue
        exp = 1 if sign == "plus" else 0
        wo = sum(1 for k in ks if int(do[k]["answer"]) != exp); wc = sum(1 for k in ks if int(dc[k]["answer"]) != exp)
        dis = sum(1 for k in ks if do[k]["answer"] != dc[k]["answer"])
        to = sum(float(do[k]["time_ms"]) for k in ks); tc = sum(float(dc[k]["time_ms"]) for k in ks)
        bo = sum(int(do[k]["bbcalls"]) for k in ks); bc = sum(int(dc[k]["bbcalls"]) for k in ks)
        T["original"] += to; T["candidate5"] += tc; W["original"] += wo; W["candidate5"] += wc; N += len(ks); BB["original"] += bo; BB["candidate5"] += bc; DIS += dis
        P(f"| l={l} {sign} | {'yes' if exp else 'no'} | {to/len(ks):.4f} | {tc/len(ks):.4f} | {to/tc:.3f} | {bo/len(ks):.1f} | {bc/len(ks):.1f} | {wo} / {wc} | {dis} |")
    if N:
        P(f"| **all 23 sets** | | **{T['original']/N:.4f}** | **{T['candidate5']/N:.4f}** | **{T['original']/T['candidate5']:.3f}** | {BB['original']/N:.1f} | {BB['candidate5']/N:.1f} | **{W['original']} / {W['candidate5']}** | **{DIS}** |")
        P(f"\nDecider totals over {N} queries: original {T['original']/1000:.2f} s, candidate5 {T['candidate5']/1000:.2f} s.\n")
open(os.path.join(R, "RESULTS.md"), "w").write("\n".join(out) + "\n")
print("\n".join(out))

# ---- merged per-query CSVs (both arms, every rep) for verification ----
def reps(pattern):
    files = sorted(glob.glob(pattern)); per = {}
    for i, fn in enumerate(files, start=1):
        for r in csv.DictReader(open(fn)):
            per.setdefault((r["file1"], r["file2"], r.get("distance", "")), {})[i] = r
    return per, len(files)
for ds, _ in DATASETS:
    o, no = reps(f"{R}/{ds}_lmf_original_r*.csv"); c, nc = reps(f"{R}/{ds}_lmf_candidate5_r*.csv")
    if o and c:
        with open(f"{R}/{ds}_lmf_merged.csv", "w") as f:
            w = csv.writer(f)
            w.writerow(["file1","file2","n1","n2","value_original","value_candidate5","abs_diff"] + [f"t_original_r{i}" for i in range(1,no+1)] + [f"t_candidate5_r{i}" for i in range(1,nc+1)] + ["bbcalls_original","bbcalls_candidate5","n6_arr_original_min","n6_arr_candidate5_min"])
            for k in o:
                if k not in c: continue
                ro, rc = o[k], c[k]; a = next(iter(ro.values())); vo = float(a["value"]); vc = float(next(iter(rc.values()))["value"])
                w.writerow([k[0], k[1], a["n1"], a["n2"], a["value"], next(iter(rc.values()))["value"], "%.3e" % abs(vo - vc)] + [ro[i]["time_ms"] if i in ro else "" for i in range(1,no+1)] + [rc[i]["time_ms"] if i in rc else "" for i in range(1,nc+1)] + [a["bbcalls"], next(iter(rc.values()))["bbcalls"], min(float(x["n6_arr_ms"]) for x in ro.values()), min(float(x["n6_arr_ms"]) for x in rc.values())])
    with open(f"{R}/{ds}_decider_merged.csv", "w") as f:
        w = csv.writer(f); header = False
        for l, sign in LS:
            o, no = reps(f"{R}/{ds}_decider_{l}_{sign}_original_r*.csv"); c, nc = reps(f"{R}/{ds}_decider_{l}_{sign}_candidate5_r*.csv")
            if not o or not c: continue
            if not header:
                w.writerow(["l","sign","expected","file1","file2","distance","answer_original","answer_candidate5"] + [f"t_original_r{i}" for i in range(1,no+1)] + [f"t_candidate5_r{i}" for i in range(1,nc+1)] + ["bbcalls_original","bbcalls_candidate5"]); header = True
            for k in o:
                if k not in c: continue
                ro, rc = o[k], c[k]; a = next(iter(ro.values())); b = next(iter(rc.values()))
                w.writerow([l, sign, 1 if sign == "plus" else 0, k[0], k[1], k[2], a["answer"], b["answer"]] + [ro[i]["time_ms"] if i in ro else "" for i in range(1,no+1)] + [rc[i]["time_ms"] if i in rc else "" for i in range(1,nc+1)] + [a["bbcalls"], b["bbcalls"]])
