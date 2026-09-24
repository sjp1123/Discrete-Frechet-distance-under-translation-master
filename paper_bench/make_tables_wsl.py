#!/usr/bin/env python3
"""Tables D and E of results/TABLES_uci.md from the r3 re-measurement on the user PC (WSL2;
run_wsl_paired.sh: steady_clock, arms back-to-back on one vCPU; merge_wsl_r3.py archives).

  Table D  Sigspatial value computation (LMF) on the authors' 1,000 Sigspatial decider pairs,
           in the format of the paper's Table 4 (not in the paper: its Table 4 is Characters only)
  Table E  decision problem in the format of the paper's Table 2 for same-characters,
           all-characters and Sigspatial, 4^l sets (paper protocol) and 2^l (authors' files)

Everything from "## Table D" to the end of TABLES_uci.md is replaced.
    python3 paper_bench/make_tables_wsl.py
"""
import csv
import io
import math
import os
import re
import statistics
import tarfile

HERE = os.path.dirname(os.path.abspath(__file__)); R = os.path.join(HERE, "results")
NL = "\n"; ST = "&nbsp;&nbsp;" + chr(92) + "* "
PAPER_T2 = {  # paper Table 2 (4^l, authors' machine): total ms, calls, pre, bb Lipschitz, estimation, arr alg, construction, arr bb
    "same-characters": (429623, 26661524, 5, 44312, 157780, 226469, 148898, 60156),
    "all-characters": (628043, 42781931, 5, 50462, 191177, 385145, 237043, 120149),
    "sigspatial": (1207560, 31420517, 5, 43861, 913266, 249268, 155332, 73934),
}


def members(tar_name, pattern):
    out = {}
    with tarfile.open(os.path.join(R, tar_name)) as tf:
        for m in tf:
            if re.fullmatch(pattern, m.name):
                out[m.name] = list(csv.DictReader(io.TextIOWrapper(tf.extractfile(m), encoding="utf-8")))
    return out


def oom_kills():
    """{"file1 file2": kill_time_s} from the archived 12 GB run of the baseline on the two OOM pairs."""
    with tarfile.open(os.path.join(R, "raw_sigspatial_lmf_r3.tar.gz")) as tf:
        text = tf.extractfile("sigspatial_lmf_original_oom_12gb.txt").read().decode()
    rows = [l.split() for l in text.splitlines() if l and not l.startswith("#")]
    return {f"{r[1]} {r[2]}": float(r[4]) for r in rows}


def geomean_ci(ratios, B=2000, seed=1):
    """Geometric mean and bootstrap 95 % interval, as in analyze_uci.py."""
    import random
    logs = [math.log(x) for x in ratios]; g = math.exp(sum(logs) / len(logs)); rng = random.Random(seed); bs = []
    for _ in range(B):
        s = [rng.choice(logs) for _ in logs]; bs.append(math.exp(sum(s) / len(s)))
    bs.sort(); return g, bs[int(0.025 * B)], bs[int(0.975 * B)]


def table_rows(label, S, n, constr_name):
    return [f"| **{label}** | **{S['time']:,.0f} ms** ({S['time']/n:,.2f} ms per instance) | **{S['calls']:,}** ({S['calls']/n:,.2f} per instance) |",
            f"| - Preprocessing | {S['pre']:,.0f} ms | |",
            f"| - Black-box calls (Lipschitz) | {S['bb']:,.0f} ms | |",
            f"| - Arrangement estimation | {S['disc']:,.0f} ms | |",
            f"| - Arrangement algorithm | {S['arr']:,.0f} ms | |",
            f"| {ST}{constr_name} | {S['n6arr']:,.0f} ms | |",
            f"| {ST}Black-box calls | {S['n6fre']:,.0f} ms | |"]


def table_d():
    m = members("raw_sigspatial_lmf_r3.tar.gz", r"sigspatial_lmf_.*_r3\.csv")
    A = {arm: {(r["file1"], r["file2"]): r for r in m[f"sigspatial_lmf_{arm}_r3.csv"]} for arm in ("original", "candidate5_exact", "candidate5_noslack")}
    keys = [k for k in A["candidate5_exact"] if k in A["original"]]
    n = len(keys)

    def agg(arm):
        S = dict(time=0.0, calls=0, pre=0.0, bb=0.0, disc=0.0, arr=0.0, n6arr=0.0, n6fre=0.0)
        for k in keys:
            r = A[arm][k]
            S["time"] += float(r["time_ms"]); S["calls"] += int(r["bbcalls"]); S["pre"] += float(r["pre2_ms"]); S["bb"] += float(r["bb2_ms"])
            S["disc"] += float(r["disc2_ms"]); S["arr"] += float(r["arr2_ms"]); S["n6arr"] += float(r["n6_arr_ms"]); S["n6fre"] += float(r["n6_fre_ms"])
        return S
    O, P, NS = agg("original"), agg("candidate5_exact"), agg("candidate5_noslack")
    t_o = {k: float(A["original"][k]["time_ms"]) for k in keys}; t_p = {k: float(A["candidate5_exact"][k]["time_ms"]) for k in keys}
    ratios = [t_o[k] / t_p[k] for k in keys]
    gm, gm_lo, gm_hi = geomean_ci(ratios); med = statistics.median(ratios); faster = sum(x > 1 for x in ratios)
    diff = max(abs(float(A["original"][k]["value"]) - float(A["candidate5_exact"][k]["value"])) for k in keys)
    slow = sorted(keys, key=lambda k: -t_o[k])
    rest = slow[10:]
    ro = sum(t_o[k] for k in rest) / len(rest); rp = sum(t_p[k] for k in rest) / len(rest)
    share1 = t_o[slow[0]] / O["time"]; share4 = sum(t_o[k] for k in slow[:4]) / O["time"]
    sum_wo1 = (O["time"] - t_o[slow[0]]) / (P["time"] - t_p[slow[0]])
    OOM_KILL_S = oom_kills()
    max_p = max(float(r["time_ms"]) for r in A["candidate5_exact"].values()) / 1000
    # the invalidated first run (r1, system_clock, arms on different vCPUs) for the sensitivity of the total ratio
    r1 = {arm: {(r["file1"], r["file2"]): float(r["time_ms"]) for r in rows}
          for arm, rows in ((a, members("raw_sigspatial_lmf.tar.gz", rf"sigspatial_lmf_{a}_r1\.csv")[f"sigspatial_lmf_{a}_r1.csv"]) for a in ("original", "candidate5_exact"))}
    r1_over_r3 = [r1["original"][k] / t_o[k] for k in slow[:10]]
    r1_sum = sum(r1["original"][k] for k in keys) / sum(r1["candidate5_exact"][k] for k in keys)
    r1_ok = [k for k in keys if r1["original"][k] > 0 and r1["candidate5_exact"][k] > 0]   # r1 has non-positive times
    r1_gm = math.exp(sum(math.log(r1["original"][k] / r1["candidate5_exact"][k]) for k in r1_ok) / len(r1_ok))
    # peak RSS per arm from run_wsl_paired.sh
    rss = {}
    with tarfile.open(os.path.join(R, "raw_sigspatial_lmf_r3.tar.gz")) as tf:
        for line in io.TextIOWrapper(tf.extractfile("sigspatial_lmf_rss_r3.txt"), encoding="utf-8"):
            i, arm, rc, kb = line.split(); rss.setdefault(arm, []).append(int(kb.split("=")[1]))
    oom = [" ".join(k) for k in A["candidate5_exact"] if k not in A["original"]]
    assert sorted(oom) == sorted(OOM_KILL_S), oom
    oom_p = [float(A["candidate5_exact"][tuple(k.split())]["time_ms"]) / 1000 for k in oom]
    L = ["## Table D - Sigspatial value computation, the authors' 1,000 decider pairs, in the format of the paper's Table 4 (user PC, WSL2, r3)", "",
         "Not in the paper (its Table 4 is Characters only). The authors' Sigspatial decider pairs on the full 20,199-curve set; value",
         "computation (`calcDistance2`), one measurement per instance; `run_wsl_paired.sh`: steady_clock, the arms of each pair back-to-back",
         f"on one vCPU, one process per pair and arm; proposed = candidate5 with MAXREGION_EXACT=1 MAXREGION_SLACK=0. {n} pairs measured on",
         f"both arms. On the other 2 the baseline is OOM-killed even under a 12 GB limit (at {OOM_KILL_S[oom[0]]:.0f} s and {OOM_KILL_S[oom[1]]:.0f} s in an",
         "earlier run with the old clock, `sigspatial_lmf_original_oom_12gb.txt` in the archive; an approximate lower bound on its time),",
         f"and the proposed arm finishes them in {oom_p[0]:.2f} s and {oom_p[1]:.2f} s. The rows below cover the {n} pairs.", "",
         "| Algorithm | Time | Black-Box Calls |", "|---|--:|--:|"]
    L += table_rows("LMF, baseline", O, n, "Construction") + table_rows("LMF, proposed", P, n, "Maximal-set enumeration")
    L += ["", f"Per-instance speed-up: geometric mean {gm:.2f} [95 % bootstrap interval {gm_lo:.2f}, {gm_hi:.2f}], median {med:.2f}, faster on {faster}/{n};",
          f"values agree to within {diff:.1e}; the slowest proposed instance, over all 1,000 pairs, takes {max_p:.2f} s.",
          f"Total-time ratio {O['time']/P['time']:.1f}x, but it rests on a few instances measured once: the slowest baseline instance is",
          f"{100*share1:.1f} % of the baseline total and the 4 slowest are {100*share4:.0f} %; without the slowest the ratio is {sum_wo1:.1f}x, and without",
          f"the 10 slowest the per-instance means are {ro:.1f} vs {rp:.1f} ms ({ro/rp:.2f}x). The same 10 slow instances took {min(r1_over_r3):.2f}-{max(r1_over_r3):.2f}x",
          f"their r3 baseline time in the invalidated first run, whose total ratio was {r1_sum:.1f}x. Read the total ratio as indicative only;",
          f"the per-instance statistics are stable (geometric mean {r1_gm:.2f} in the first run, {gm:.2f} here).",
          f"candidate5_noslack (same predicates without the exact filter): {NS['time']/n:.1f} ms per instance, {NS['calls']/n:,.1f} calls.",
          f"Peak RSS over the {n} pairs: baseline {max(rss['original'])/1024:,.0f} MB, proposed {max(rss['candidate5_exact'])/1024:,.0f} MB.", ""]
    return L


def table_e():
    tars = {"characters_uci": "raw_characters_uci_decider_r3.tar.gz", "sigspatial": "raw_sigspatial_decider_r3.tar.gz"}
    bench = [("same-characters", "characters_uci", "characters_uci_same"), ("all-characters", "characters_uci", "characters_uci_all"), ("sigspatial", "sigspatial", "sigspatial")]
    cache = {k: members(v, r".*_r3\.csv") for k, v in tars.items()}

    def agg(src, prefix, tag, arm):
        S = dict(time=0.0, calls=0, pre=0.0, bb=0.0, disc=0.0, arr=0.0, n6arr=0.0, n6fre=0.0, n=0, wrong=0)
        for name, rows in cache[src].items():
            mm = re.fullmatch(prefix + "_" + tag + r"_(-?\d+)_(plus|minus)_" + arm + r"_r3\.csv", name)
            if not mm: continue
            exp = 1 if mm.group(2) == "plus" else 0
            for r in rows:
                S["time"] += float(r["time_ms"]); S["calls"] += int(r["bbcalls"]); S["pre"] += float(r["pre1_ms"]); S["bb"] += float(r["bb1_ms"])
                S["disc"] += float(r["disc1_ms"]); S["arr"] += float(r["arr1_ms"]); S["n6arr"] += float(r["n6_arr_ms"]); S["n6fre"] += float(r["n6_fre_ms"])
                S["n"] += 1; S["wrong"] += int(r["answer"]) != exp
        return S
    L = ["## Table E - Decision problem in the format of the paper's Table 2: same-characters, all-characters, Sigspatial (user PC, WSL2, r3)", "",
         "The authors' 23 x 1,000 decider instances per benchmark, each measured once; `run_wsl_paired.sh`: steady_clock, both arms on the",
         "same vCPU, alternating per query file. Stage timers are the authors' `updateProfileDec` ones (`FUT_PREPROCESSING1`, `FUT_BLACKBOX1`,",
         "`FUT_DISCSELECTION1`, `FUT_ARRANGEMENT1` with sub-timers `FUT_N6_ARR` and `FUT_N6_FRECHET`; the sub-rows omit the untimed rest of the",
         "stage). The 4^l sets follow the paper's text (its instance files",
         "are not shipped, so the pair sample may differ from the paper's); the 2^l sets are the authors' shipped files (their own outputs for",
         "those are in `experiments/`, see `REPRO_check.md`). Paper rows are the authors' machine and are shown for the stage shares only.",
         "The proposed arm is candidate5 with MAXREGION_EXACT=1 MAXREGION_SLACK=0, as in Tables C and D. Table A (container) ran the",
         "default candidate5 (band filter, caller slack); its call counts differ from these on 3 of 92,000 Characters instances, so Tables A",
         "and E differ in both machine and proposed configuration.", ""]
    summary = []
    for tag, title in (("paperq4", "4^l (the paper's protocol)"), ("paperq", "2^l (the authors' shipped query files)")):
        L += [f"### {title}", ""]
        for name, src, prefix in bench:
            O = agg(src, prefix, tag, "original"); P = agg(src, prefix, tag, "candidate5")
            assert O["n"] == P["n"] == 23000 and O["wrong"] == P["wrong"] == 0, (name, tag, O["n"], P["n"], O["wrong"], P["wrong"])
            L += [f"#### {name} (23 x 1,000 instances; both arms 0 wrong answers)", "", "| Algorithm | Time | Black-Box Calls |", "|---|--:|--:|"]
            if tag == "paperq4":
                p = PAPER_T2[name]
                L += [f"| *LMF as reported in [BKN20], Table 2 (authors' machine)* | *{p[0]:,} ms* ({p[0]/23000:.1f} ms per instance) | *{p[1]:,}* ({p[1]/23000:,.2f} per instance) |",
                      f"| *- Preprocessing / Black-box calls (Lipschitz) / Arrangement estimation* | *{p[2]:,} / {p[3]:,} / {p[4]:,} ms* | |",
                      f"| *- Arrangement algorithm (Construction, Black-box calls)* | *{p[5]:,} ms ({p[6]:,}, {p[7]:,})* | |"]
            L += table_rows("LMF, baseline", O, O["n"], "Construction") + table_rows("LMF, proposed", P, P["n"], "Maximal-set enumeration") + [""]
            summary.append((tag, name, O, P))
    L += ["### Summary", "", "| set | benchmark | ms/instance baseline -> proposed | speed-up | calls/instance | arrangement algorithm (ms) | its share of the baseline |", "|---|---|---|--:|---|---|--:|"]
    for tag, name, O, P in summary:
        n = O["n"]
        L.append(f"| {'4^l' if tag == 'paperq4' else '2^l'} | {name} | {O['time']/n:.2f} -> {P['time']/n:.2f} | {O['time']/P['time']:.2f}x | {O['calls']/n:,.0f} -> {P['calls']/n:,.0f} | {O['arr']:,.0f} -> {P['arr']:,.0f} | {100*O['arr']/O['time']:.1f} % |")
    L.append("")
    return L


def main():
    p = os.path.join(R, "TABLES_uci.md")
    t = open(p, encoding="utf-8").read().replace("\r", "")
    if "## Table D" in t:
        t = t[: t.index("## Table D")]
    new = t.rstrip(NL) + NL + NL + NL.join(table_d() + table_e()) + NL
    open(p, "w", encoding="utf-8", newline=NL).write(new)
    print(NL.join(table_d()))


if __name__ == "__main__":
    main()
