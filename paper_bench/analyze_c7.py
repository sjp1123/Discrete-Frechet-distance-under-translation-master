#!/usr/bin/env python3
"""Analyse the paired candidate7 timing run (run_wsl_c7.sh) -> results/C7_timing.md.

Input: results/raw_c7_timing.tar.gz with the run directory's lmf_chars/, lmf_sig/, decider/ (one
paper_bench CSV per job and arm), rss/, rc.txt and log.txt.  Arms: original (the authors' code),
candidate7 (proposed, this tree) and candidate5 (the method of the submitted abstract), each job
measured back-to-back on one pinned vCPU of this PC (WSL2, steady_clock).

    python paper_bench/analyze_c7.py
"""
import csv, io, math, os, random, statistics, sys, tarfile
from collections import defaultdict

sys.stdout.reconfigure(encoding="utf-8")
HERE = os.path.dirname(os.path.abspath(__file__))
RES = os.path.join(HERE, "results")
TAR = os.environ.get("C7_TAR", os.path.join(RES, "raw_c7_timing.tar.gz"))
OUT = os.environ.get("C7_OUT", os.path.join(RES, "C7_timing.md"))
ARMS = ["original", "candidate7", "candidate5"]
LABEL = {"original": "original (baseline)", "candidate7": "candidate7 (proposed)", "candidate5": "candidate5 (abstract)"}
EPS = 1e-7


def load():
    rows = defaultdict(list)   # (task, arm) -> rows in job order
    extra = {}
    with tarfile.open(TAR) as tf:
        names = sorted(m.name for m in tf.getmembers() if m.isfile())
        for n in names:
            parts = n.split("/")
            if len(parts) == 3 and parts[2].endswith(".csv"):
                task, arm, job = parts[0], parts[1], parts[2][:-4]
                for i, r in enumerate(csv.DictReader(io.TextIOWrapper(tf.extractfile(n), encoding="utf-8"))):
                    r["_job"], r["_i"] = job, i   # every arm runs the same job file in the same order
                    rows[(task, arm)].append(r)
            elif n in ("rc.txt", "log.txt"):
                extra[n] = tf.extractfile(n).read().decode()
            elif parts[0] == "rss":
                extra[n] = tf.extractfile(n).read().decode().split()
    return rows, extra


def f(r, k):
    return float(r[k])


def gmean_ci(ratios, B=2000, seed=1):
    logs = [math.log(x) for x in ratios if x > 0]
    g = math.exp(sum(logs) / len(logs))
    rng = random.Random(seed)
    bs = sorted(math.exp(sum(rng.choice(logs) for _ in logs) / len(logs)) for _ in range(B))
    return g, bs[int(0.025 * B)], bs[int(0.975 * B)]


def key_lmf(task, r):
    # (job, row): the Characters list repeats two pairs, so file names are not a key
    return (r["_job"], r["_i"])


def cmp_line(base, prop, label):
    """base, prop: dict key -> time_ms (common keys)."""
    ks = [k for k in base if k in prop]
    rat = [base[k] / prop[k] for k in ks if prop[k] > 0 and base[k] > 0]
    g, lo, hi = gmean_ci(rat)
    sb, sp = sum(base[k] for k in ks), sum(prop[k] for k in ks)
    return (f"| {label} | {len(ks):,} | {sb/1000:,.1f} s | {sp/1000:,.1f} s | **{sb/sp:.2f}×** | **{g:.2f}×** [{lo:.2f}, {hi:.2f}] | "
            f"{statistics.median(rat):.2f}× | {sum(x > 1 for x in rat)/len(rat)*100:.1f} % |")


def main():
    rows, extra = load()
    out = []
    w = out.append
    w("# candidate7: correctness on the paper's data and paired timings against original\n")
    w("Machine: this PC (Intel Core Ultra 5 125H, WSL2 Ubuntu 22.04, g++ 11.4, CGAL 5.4, Boost 1.74), "
      "paper_bench built from the current sources of each arm (`RelWithDebInfo`, same flags). Every job "
      "(a chunk of 100 Characters pairs, one Sigspatial pair, or one decider query file) ran the three arms "
      "back-to-back on the same pinned vCPU, in an order rotated per job; 8 such streams ran in parallel "
      "(vCPUs 1,3,…,15). Clock: `steady_clock`. candidate7 and candidate5 run with `MAXREGION=cech "
      "MAXREGION_EXACT=1 MAXREGION_SLACK=0` (candidate7's defaults); original has no knobs. "
      "Raw data: `results/raw_c7_timing.tar.gz`.\n")
    rc = extra.get("rc.txt", "")
    bad_rc = [l for l in rc.splitlines() if "rc=" in l and not l.endswith("rc=0")]
    skipped = [l for l in rc.splitlines() if "skipped" in l]
    w(f"Process exits: {len([l for l in rc.splitlines() if l.endswith('rc=0')]):,} with rc=0; "
      f"{len(bad_rc)} non-zero; {len(skipped)} skipped (the original on the 2 Sigspatial pairs that needed > 12 GB in r3).")
    for l in bad_rc:
        w(f"- `{l}`")
    w("")

    # ------------------------------------------------------------------ correctness
    w("## 1. Correctness\n")
    w("### Decider (paperq4 files: YES expected at (1 + 4^l)·δ*, NO at (1 − 4^l)·δ*)\n")
    w("| data set | queries per arm | wrong: original | wrong: candidate7 | wrong: candidate5 | candidate7 ≠ original |")
    w("|---|--:|--:|--:|--:|--:|")
    dec = {a: rows[("decider", a)] for a in ARMS}
    for ds, pref in (("all-characters", "d_characters_uci_all_"), ("same-characters", "d_characters_uci_same_"), ("Sigspatial", "d_sigspatial_")):
        cnt = {}
        wrong = {}
        by = {}
        for a in ARMS:
            rs = [r for r in dec[a] if r["_job"].startswith(pref)]
            cnt[a] = len(rs)
            wrong[a] = sum((r["answer"] == "1") != r["_job"].endswith("_plus") for r in rs)
            by[a] = {(r["_job"], r["_i"]): r["answer"] for r in rs}
        diff = sum(by["candidate7"][k] != v for k, v in by["original"].items() if k in by["candidate7"])
        w(f"| {ds} | {cnt['candidate7']:,} | {wrong['original']} | {wrong['candidate7']} | {wrong['candidate5']} | {diff} |")
    w("")
    w("### Value computation (LMF, `calcDistance2`)\n")
    w("| data set | pairs | max \\|candidate7 − original\\| | pairs over 1e-7 | max \\|candidate7 − candidate5\\| | pairs over 1e-7 |")
    w("|---|--:|--:|--:|--:|--:|")
    for task, ds in (("lmf_chars", "Characters (21,000)"), ("lmf_sig", "Sigspatial (1,000)")):
        v = {a: {key_lmf(task, r): f(r, "value") for r in rows[(task, a)]} for a in ARMS}
        c7 = v["candidate7"]
        def dd(other):
            ks = [k for k in c7 if k in v[other]]
            d = [abs(c7[k] - v[other][k]) for k in ks]
            return len(ks), max(d), sum(x > EPS for x in d)
        n1, m1, o1 = dd("original")
        n2, m2, o2 = dd("candidate5")
        w(f"| {ds} | {len(c7):,} (vs original {n1:,}) | {m1:.2e} | {o1} | {m2:.2e} | {o2} |")
    w("")

    # ------------------------------------------------------------------ LMF timings
    w("## 2. Value computation (LMF), in the paper's Table 4 format\n")
    for task, ds in (("lmf_chars", "Characters, the authors' 21,000 pairs"), ("lmf_sig", "Sigspatial, the authors' 1,000 decider pairs")):
        w(f"### {ds}\n")
        base = {key_lmf(task, r): r for r in rows[(task, "original")]}
        common = None
        for a in ARMS:
            ks = {key_lmf(task, r) for r in rows[(task, a)]}
            common = ks if common is None else common & ks
        common = sorted(common)
        note = "" if len(common) == len(rows[(task, "candidate7")]) else f" (pairs measured on every arm: {len(common):,} of {len(rows[(task, 'candidate7')]):,})"
        w(f"Times summed over the {len(common):,} pairs measured on all three arms{note}.\n")
        w("| | " + " | ".join(LABEL[a] for a in ARMS) + " |")
        w("|---|" + "--:|" * len(ARMS))
        R = {a: {key_lmf(task, r): r for r in rows[(task, a)]} for a in ARMS}
        def s(a, col):
            return sum(f(R[a][k], col) for k in common)
        n = len(common)
        w("| Time | " + " | ".join(f"{s(a,'time_ms'):,.0f} ms ({s(a,'time_ms')/n:,.1f} ms/inst.)" for a in ARMS) + " |")
        w("| Black-box calls | " + " | ".join(f"{s(a,'bbcalls'):,.0f} ({s(a,'bbcalls')/n:,.1f}/inst.)" for a in ARMS) + " |")
        for lab, col in (("– Preprocessing", "pre2_ms"), ("– Black-box calls (Lipschitz)", "bb2_ms"),
                         ("– Arrangement estimation", "disc2_ms"), ("– Arrangement algorithm / base case", "arr2_ms"),
                         ("  * Construction / maximal-set enumeration", "n6_arr_ms"), ("  * Black-box calls", "n6_fre_ms")):
            w(f"| {lab} | " + " | ".join(f"{s(a,col):,.0f} ms" for a in ARMS) + " |")
        w("")
        w("| comparison | pairs | baseline total | proposed total | total ratio | geometric mean [95% CI] | median | proposed faster |")
        w("|---|--:|--:|--:|--:|--:|--:|--:|")
        t = {a: {k: f(R[a][k], "time_ms") for k in common} for a in ARMS}
        w(cmp_line(t["original"], t["candidate7"], "original / candidate7"))
        w(cmp_line(t["original"], t["candidate5"], "original / candidate5"))
        w(cmp_line(t["candidate5"], t["candidate7"], "candidate5 / candidate7 (cost of the fixes, <1 = slower)"))
        w("")
        if task == "lmf_sig":
            only = [k for k in R["candidate7"] if k not in R["original"]]
            if only:
                w("Pairs without an original time (skipped: > 12 GB in r3): " + "; ".join(
                    f"{R['candidate7'][k]['file1']}/{R['candidate7'][k]['file2']} candidate7 {f(R['candidate7'][k],'time_ms')/1000:.2f} s, "
                    f"value {f(R['candidate7'][k],'value'):.6f}" for k in only) + ".\n")
            top = sorted(common, key=lambda k: -t["original"][k])[:4]
            share = sum(t["original"][k] for k in top) / sum(t["original"].values())
            w(f"The total ratio is dominated by a few pairs: the 4 slowest original pairs are {share*100:.0f} % of the original total. "
              "Read the geometric mean and the median for the typical pair.\n")
            rss = {a: [int(v[1]) for kk, v in extra.items() if kk.startswith("rss/s") and kk.endswith(f"_{a}.txt") and len(v) == 2] for a in ARMS}
            if all(rss.values()):
                w("Peak RSS per Sigspatial LMF process (MB): " + "; ".join(
                    f"{a} median {statistics.median(rss[a])/1024:.0f}, max {max(rss[a])/1024:.0f}" for a in ARMS) + ".\n")

    # ------------------------------------------------------------------ decider timings
    w("## 3. Decision problem, in the paper's Table 2 format (23 paperq4 files × 1,000 queries per data set)\n")
    for ds, pref in (("same-characters", "d_characters_uci_same_"), ("all-characters", "d_characters_uci_all_"), ("Sigspatial", "d_sigspatial_")):
        w(f"### {ds}\n")
        R = {a: [r for r in dec[a] if r["_job"].startswith(pref)] for a in ARMS}
        n = len(R["candidate7"])
        def s(a, col):
            return sum(f(r, col) for r in R[a])
        w("| | " + " | ".join(LABEL[a] for a in ARMS) + " |")
        w("|---|" + "--:|" * len(ARMS))
        w("| Time | " + " | ".join(f"{s(a,'time_ms'):,.0f} ms ({s(a,'time_ms')/len(R[a]):,.2f} ms/inst.)" for a in ARMS) + " |")
        w("| Black-box calls | " + " | ".join(f"{s(a,'bbcalls'):,.0f} ({s(a,'bbcalls')/len(R[a]):,.1f}/inst.)" for a in ARMS) + " |")
        for lab, col in (("– Preprocessing", "pre1_ms"), ("– Black-box calls (Lipschitz)", "bb1_ms"),
                         ("– Arrangement estimation", "disc1_ms"), ("– Arrangement algorithm / base case", "arr1_ms"),
                         ("  * Construction / maximal-set enumeration", "n6_arr_ms"), ("  * Black-box calls", "n6_fre_ms")):
            w(f"| {lab} | " + " | ".join(f"{s(a,col):,.0f} ms" for a in ARMS) + " |")
        w("")
        T = {a: {(r["_job"], r["_i"]): f(r, "time_ms") for r in R[a]} for a in ARMS}
        w("| comparison | queries | baseline total | proposed total | total ratio | geometric mean [95% CI] | median | proposed faster |")
        w("|---|--:|--:|--:|--:|--:|--:|--:|")
        w(cmp_line(T["original"], T["candidate7"], "original / candidate7"))
        w(cmp_line(T["original"], T["candidate5"], "original / candidate5"))
        w(cmp_line(T["candidate5"], T["candidate7"], "candidate5 / candidate7 (cost of the fixes, <1 = slower)"))
        w("")

    w("## 4. Per-level decider means (ms/query), as in the paper's Figure 4\n")
    w("| level | " + " | ".join(f"{ds} orig. | {ds} c7" for ds in ("same", "all", "Sigspatial")) + " |")
    w("|---|" + "--:|" * 6)
    levels = [f"{l}_minus" for l in range(-1, -11, -1)] + [f"{l}_plus" for l in range(-10, 3)]
    for lv in levels:
        cells = []
        for pref in ("d_characters_uci_same_paperq4_", "d_characters_uci_all_paperq4_", "d_sigspatial_paperq4_"):
            for a in ("original", "candidate7"):
                rs = [f(r, "time_ms") for r in dec[a] if r["_job"] == pref + lv]
                cells.append(f"{sum(rs)/len(rs):.2f}" if rs else "–")
        sign, l = ("1 − 4^" if lv.endswith("minus") else "1 + 4^"), lv.split("_")[0]
        w(f"| {sign}{l} | " + " | ".join(cells) + " |")
    w("")
    open(OUT, "w", encoding="utf-8", newline="\n").write("\n".join(out) + "\n")
    print("\n".join(out))


if __name__ == "__main__":
    main()
