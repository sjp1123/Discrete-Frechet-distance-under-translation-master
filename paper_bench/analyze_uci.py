#!/usr/bin/env python3
"""Summarise paper_bench/run_uci.sh results (Characters, original UCI file, the authors'
own instance files, one measurement per instance) -> results/RESULTS_uci.md.

LMF: results/characters_uci_lmf_<arm>_r1.csv over the 21,000 pairs of
characters_full_<s1>_<s2>.txt; reported for all pairs and for the 2,000
same-letter pairs (s1 == s2).  Paper (ESA 2020, Table 4, all 21,000): 140.0 ms and
12,387 black-box calls per instance, construction 52.3 % of the time.
candidate5_exact = candidate5_noslack with MAXREGION_EXACT=1 (filtered P2/P3 predicates,
rational fallback; see candidate5/lib/cgal_disk_arrangements/maximal_regions.cpp).
Decider: results/characters_uci_<all|same>_<tag>_<l>_<sign>_<arm>_r1.csv,
tag paperq = the authors' files (2^l factors), paperq4 = same pairs with (1 +- 4^l).
Rows are matched by line index (the authors' all-characters list contains two
repeated pairs, which are kept as separate instances as in the harness).
Paper (Table 2): all-characters 27.3 ms / 1,860 calls, same-characters 18.7 ms / 1,159
calls per instance.
"""
import csv, math, os, random, statistics
import sys; sys.stdout.reconfigure(encoding="utf-8")   # Windows console defaults to cp949
here = os.path.dirname(os.path.abspath(__file__)); R = os.path.join(here, "results")
EPS = 1e-7
LMF_REP = os.environ.get("LMF_REP", "r1")   # r1: first instance; r2: re-measurement of all arms back-to-back
DEC_REP = os.environ.get("DEC_REP", "r1")   # decider repetition (r3: WSL re-measurement with steady_clock)
# DATASET=characters_uci (default): characters_uci_<all|same>_... files, LMF grouped by letter sets.
# DATASET=sigspatial: run_sig.sh output, sigspatial_<tag>_... files, LMF on the 1,000 decider pairs.
DATASET = os.environ.get("DATASET", "characters_uci")
if DATASET == "sigspatial":
    TITLE = f"Sigspatial, the full 20,199-curve set, the authors' decider instances (one measurement each; LMF {LMF_REP}, decider {DEC_REP})"
    DS = [("", "Sigspatial")]; PREFIX = "sigspatial"; TAR_GLOB = "raw_sigspatial_*.tar.gz"
    PAPER_LMF = None; PAPER_DEC = {"": (None, None)}
    LMF_TITLE = "## Value computation (LMF, `calcDistance2`) on the 1,000 pairs of the authors' Sigspatial decider set\n"
    LMF_GROUPS = (("all 1,000 pairs", lambda s: True),)
else:
    TITLE = f"Characters on the original UCI file, the authors' instances (one measurement each; LMF repetition {LMF_REP})"
    DS = [("all", "all-characters"), ("same", "same-characters")]; PREFIX = "characters_uci"; TAR_GLOB = "raw_characters_uci_*.tar.gz"
    PAPER_LMF = (140.0, 12387, 52.3)
    PAPER_DEC = {"all": (27.3, 1860), "same": (18.7, 1159)}
    LMF_TITLE = "## Value computation (LMF, `calcDistance2`), the authors' `characters_full_*` pairs\n"
    LMF_GROUPS = (("all pairs (210 letter pairs)", lambda s: True), ("same-letter pairs (20 files)", lambda s: s[0] == s[1]))
def dsname(ds): return f"{PREFIX}_{ds}" if ds else PREFIX
# Sigspatial rows are keyed by the pair (the authors' list has no repeats and one arm may lack rows);
# Characters rows by line index (their all-characters list repeats two pairs).
def keyed(rs): return {(r["file1"], r["file2"]): r for r in rs} if DATASET == "sigspatial" else {i: r for i, r in enumerate(rs)}

import glob, io, tarfile
_TARS = None
def _open(fn):
    """Loose CSV in results/, else the same file name inside results/raw_characters_uci_*.tar.gz."""
    global _TARS
    if os.path.exists(fn): return open(fn)
    if _TARS is None: _TARS = [tarfile.open(t) for t in sorted(glob.glob(os.path.join(R, TAR_GLOB)))]
    base = os.path.basename(fn)
    for t in _TARS:
        try: return io.TextIOWrapper(t.extractfile(base), encoding="utf-8")
        except KeyError: pass
    return None

def rows(fn):
    f = _open(fn)
    if f is None: return []
    out = []
    for r in csv.DictReader(f):
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

out = [f"# {TITLE}\n"]

# ---------------- LMF ----------------
sets = {}
if DATASET != "sigspatial":
    for l in open(os.path.join(here, "queries", "characters_uci_lmf_sets.txt")):
        a, b, s1, s2 = l.split(); sets[(a, b)] = (s1, s2)
arms = ["original", "candidate5", "candidate5_noslack", "candidate5_exact"]
lmf = {a: keyed(rows(os.path.join(R, f"{PREFIX}_lmf_{a}_{LMF_REP}.csv"))) for a in arms}
arms = [a for a in arms if lmf[a]]
if "original" in arms:
    out.append(LMF_TITLE)
    if PAPER_LMF: out.append(f"Paper Table 4 (all 21,000 instances, authors' machine): {PAPER_LMF[0]} ms, {PAPER_LMF[1]:,} black-box calls per instance, construction {PAPER_LMF[2]} % of time.\n")
    for label, keep in LMF_GROUPS:
        common = [k for k in lmf["original"] if keep(sets.get((lmf["original"][k]["file1"], lmf["original"][k]["file2"]), ("", ""))) and all(k in lmf[a] for a in arms)]
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
        top4 = sum(sorted(T0.values(), reverse=True)[:4]) / sum(T0.values())
        if top4 > 0.5:
            out.append(f"\nThe sum ratio is tail-driven: the 4 slowest baseline instances are {100*top4:.0f} % of the baseline total; "
                       "read the geometric mean and median (see `TABLES_uci.md` Table D).")
        out.append("")

# ---------------- decider ----------------
LS = [(l, "plus", 1) for l in range(-10, 3)] + [(l, "minus", 0) for l in range(-10, 0)]
for tag, desc in (("paperq", "the authors' query files (factors 1 ± 2^l)"), ("paperq4", "same pairs, factors (1 ± 4^l) as in the paper text")):
    for ds, name in DS:
        table = []; tot = {"original": [0, 0, 0], "candidate5": [0, 0, 0]}; wrong_t = [0, 0]; dis_t = 0
        for l, sign, exp in LS:
            d = {a: keyed(rows(os.path.join(R, f"{dsname(ds)}_{tag}_{l}_{sign}_{a}_{DEC_REP}.csv"))) for a in tot}
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
        if DATASET == "sigspatial" and DEC_REP == "r3":
            out.append("candidate5 = candidate5 with MAXREGION_EXACT=1 MAXREGION_SLACK=0 (run_wsl_paired.sh, r3); in RESULTS_uci.md it is the default configuration.\n")
        if PAPER_DEC[ds][0] is not None: out.append(f"Paper Table 2 ({name}, authors' machine): {PAPER_DEC[ds][0]} ms and {PAPER_DEC[ds][1]:,} black-box calls per instance.\n")
        out.append("| set | expected | n | original ms/instance | candidate5 ms/instance | ratio | bb calls orig | bb calls c5 | wrong (orig / c5) | arms disagree |")
        out.append("|---|---|--:|--:|--:|--:|--:|--:|--:|--:|")
        out += table
        o, c5 = tot["original"], tot["candidate5"]
        out.append(f"| **all sets** | | {n} | **{o[0]/n:.4f}** | **{c5[0]/n:.4f}** | **{o[0]/c5[0]:.3f}** | {o[2]/n:.1f} | {c5[2]/n:.1f} | **{wrong_t[0]} / {wrong_t[1]}** | **{dis_t}** |")
        out.append(f"\nTotals over {n} instances: original {o[0]/1000:.2f} s, candidate5 {c5[0]/1000:.2f} s.\n")

open(os.path.join(R, os.environ.get("OUT", "RESULTS_sig.md" if DATASET == "sigspatial" else "RESULTS_uci.md")), "w", encoding="utf-8", newline=chr(10)).write("\n".join(out) + "\n")
print("\n".join(out))
