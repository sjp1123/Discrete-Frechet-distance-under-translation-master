#!/usr/bin/env python3
"""Summarise paper_bench/run_uci.sh results (Characters, original UCI file, the authors'
own instance files, one measurement per instance) -> results/RESULTS_uci.md.

LMF: results/characters_uci_lmf_<arm>_r1.csv over the 21,000 pairs of
characters_full_<s1>_<s2>.txt; reported for all pairs and for the 2,000
same-letter pairs (s1 == s2).  Paper (ESA 2020, Table 4, all 21,000): 140.0 ms and
12,387 black-box calls per instance, construction 52.3 % of the time.
Decider: results/characters_uci_<all|same>_<tag>_<l>_<sign>_<arm>_r1.csv,
tag paperq = the authors' files (2^l factors), paperq4 = same pairs with (1 +- 4^l).
Paper (Table 2): all-characters 27.3 ms / 1,860 calls, same-characters 18.7 ms / 1,159
calls per instance.
"""
import csv, math, os, random, statistics
here = os.path.dirname(os.path.abspath(__file__)); R = os.path.join(here, "results")
EPS = 1e-7
PAPER_LMF = (140.0, 12387, 52.3)
PAPER_DEC = {"all": (27.3, 1860), "same": (18.7, 1159)}

def rows(fn):
    if not os.path.exists(fn): return []
    out = []
    for r in csv.DictReader(open(fn)):
        if any(v in (None, "") for v in r.values()): continue
        out.append(r)
    return out

def geomean_ci(ratios, B=2000, seed=1):
    logs = [math.log(x) for x in ratios if x > 0]
    if not logs: return float("nan"), float("nan"), float("nan")
    g = math.exp(sum(logs) / len(logs)); rng = random.Random(seed); bs = []
    for _ in range(B):
        s = [rng.choice(logs) for _ in logs]; bs.append(math.exp(sum(s) / len(s)))
    bs.sort(); return g, bs[int(0.025 * B)], bs[int(0.975 * B)]

out = ["# Characters on the original UCI file, the authors' instances (one measurement each)\n"]

# ---------------- LMF ----------------
sets = {}
for l in open(os.path.join(here, "queries", "characters_uci_lmf_sets.txt")):
    a, b, s1, s2 = l.split(); sets[(a, b)] = (s1, s2)
arms = ["original", "candidate5", "candidate5_noslack"]
lmf = {a: {(r["file1"], r["file2"]): r for r in rows(os.path.join(R, f"characters_uci_lmf_{a}_r1.csv"))} for a in arms}
arms = [a for a in arms if lmf[a]]
if "original" in arms:
    out.append("## Value computation (LMF, `calcDistance2`), the authors' `characters_full_*` pairs\n")
    out.append(f"Paper Table 4 (all 21,000 instances, authors' machine): {PAPER_LMF[0]} ms, {PAPER_LMF[1]:,} black-box calls per instance, construction {PAPER_LMF[2]} % of time.\n")
    for label, keep in (("all pairs (210 letter pairs)", lambda s: True), ("same-letter pairs (20 files)", lambda s: s[0] == s[1])):
        common = [k for k in lmf["original"] if keep(sets[k]) and all(k in lmf[a] for a in arms)]
        if not common: continue
        out.append(f"### {label}: {len(common)} pairs measured on every arm\n")
        out.append("| arm | mean ms/instance | total s | bb calls/instance | construction % | arr. bb calls % | max abs diff vs original | pairs over 1e-7 (cand > orig) | vs original: sum-ratio, geomean [95% CI], median, faster |")
        out.append("|---|--:|--:|--:|--:|--:|--:|--:|---|")
        T0 = {k: float(lmf["original"][k]["time_ms"]) for k in common}
        for a in arms:
            t = [float(lmf[a][k]["time_ms"]) for k in common]
            arr = sum(float(lmf[a][k]["n6_arr_ms"]) for k in common); fre = sum(float(lmf[a][k]["n6_fre_ms"]) for k in common)
            calls = sum(int(lmf[a][k]["bbcalls"]) for k in common) / len(common)
            if a == "original":
                diff = "—"; over = "—"; cmp = "—"
            else:
                d = [float(lmf[a][k]["value"]) - float(lmf["original"][k]["value"]) for k in common]
                diff = f"{max(abs(x) for x in d):.3e}"; over = f"{sum(abs(x) > EPS for x in d)} ({sum(x > EPS for x in d)})"
                ratios = [T0[k] / float(lmf[a][k]["time_ms"]) for k in common]
                g, lo, hi = geomean_ci(ratios)
                cmp = f"{sum(T0.values()) / sum(t):.2f}, {g:.2f} [{lo:.2f}, {hi:.2f}], {statistics.median(ratios):.2f}, {sum(r > 1 for r in ratios)}/{len(common)}"
            out.append(f"| {a} | {sum(t)/len(t):.2f} | {sum(t)/1000:.1f} | {calls:,.0f} | {100*arr/sum(t):.1f} | {100*fre/sum(t):.1f} | {diff} | {over} | {cmp} |")
        out.append("")

# ---------------- decider ----------------
LS = [(l, "plus", 1) for l in range(-10, 3)] + [(l, "minus", 0) for l in range(-10, 0)]
for tag, desc in (("paperq", "the authors' query files (factors 1 ± 2^l)"), ("paperq4", "same pairs, factors (1 ± 4^l) as in the paper text")):
    for ds, name in (("all", "all-characters"), ("same", "same-characters")):
        table = []; tot = {"original": [0, 0, 0], "candidate5": [0, 0, 0]}; wrong_t = [0, 0]; dis_t = 0
        for l, sign, exp in LS:
            d = {a: {(r["file1"], r["file2"]): r for r in rows(os.path.join(R, f"characters_uci_{ds}_{tag}_{l}_{sign}_{a}_r1.csv"))} for a in tot}
            common = [k for k in d["original"] if k in d["candidate5"]]
            if not common: continue
            t = {a: sum(float(d[a][k]["time_ms"]) for k in common) for a in tot}
            c = {a: sum(int(d[a][k]["bbcalls"]) for k in common) for a in tot}
            wrong = [sum(int(d[a][k]["answer"]) != exp for k in common) for a in tot]
            dis = sum(d["original"][k]["answer"] != d["candidate5"][k]["answer"] for k in common)
            table.append(f"| l={l} {sign} | {'yes' if exp else 'no'} | {len(common)} | {t['original']/len(common):.4f} | {t['candidate5']/len(common):.4f} | {t['original']/t['candidate5']:.3f} | {c['original']/len(common):.1f} | {c['candidate5']/len(common):.1f} | {wrong[0]} / {wrong[1]} | {dis} |")
            for a in tot: tot[a][0] += t[a]; tot[a][1] += len(common); tot[a][2] += c[a]
            wrong_t[0] += wrong[0]; wrong_t[1] += wrong[1]; dis_t += dis
        if not table: continue
        n = tot["original"][1]
        out.append(f"## Decision problem, {name}, {desc}\n")
        out.append(f"Paper Table 2 ({name}, authors' machine): {PAPER_DEC[ds][0]} ms and {PAPER_DEC[ds][1]:,} black-box calls per instance.\n")
        out.append("| set | expected | n | original ms/instance | candidate5 ms/instance | ratio | bb calls orig | bb calls c5 | wrong (orig / c5) | arms disagree |")
        out.append("|---|---|--:|--:|--:|--:|--:|--:|--:|--:|")
        out += table
        o, c5 = tot["original"], tot["candidate5"]
        out.append(f"| **all sets** | | {n} | **{o[0]/n:.4f}** | **{c5[0]/n:.4f}** | **{o[0]/c5[0]:.3f}** | {o[2]/n:.1f} | {c5[2]/n:.1f} | **{wrong_t[0]} / {wrong_t[1]}** | **{dis_t}** |")
        out.append(f"\nTotals over {n} instances: original {o[0]/1000:.2f} s, candidate5 {c5[0]/1000:.2f} s.\n")

open(os.path.join(R, "RESULTS_uci.md"), "w").write("\n".join(out) + "\n")
print("\n".join(out))
