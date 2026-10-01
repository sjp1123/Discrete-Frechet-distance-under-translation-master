#!/usr/bin/env python3
"""Real-data anchors: values of both arms vs the authors' delta* files, agreement between arms,
certificates (DFD at the reported translation, independent long double DP), and identity with r4."""
import csv, glob, os, subprocess

R = "/home/claude/Discrete-Frechet-distance-under-translation-master"
W = "/root/verif/real"
CH, SG = f"{R}/paper_data/characters_uci/data", f"{R}/paper_data/sigspatial/data"
AUTH = {"chars_all_pairs": f"{R}/original/test_data/fut_decider_benchmark_queries/characters_fut_decider_computed_distances.check",
        "chars_same_pairs": f"{R}/original/test_data/fut_decider_benchmark_queries/characters_fut_decider_samechar_computed_distances.check",
        "sig": f"{R}/original/test_data/fut_decider_benchmark_queries/sigspatial_fut_decider_computed_distances.check"}
LISTS = [("chars_all_pairs", CH, "original", "chars_all_pairs"), ("chars_all_pairs", CH, "candidate7", "chars_all_pairs"),
         ("chars_same_pairs", CH, "original", "chars_same_pairs"), ("chars_same_pairs", CH, "candidate7", "chars_same_pairs"),
         ("chars_full_sub", CH, "original", None), ("chars_full_sub", CH, "candidate7", None),
         ("sig_pairs_998", SG, "original", "sig"), ("sig_pairs_1000", SG, "candidate7", "sig")]


def auth(path):
    d = {}
    for l in open(path):
        p = l.split()
        if len(p) >= 3:
            d.setdefault((p[0], p[1]), []).append(float(p[2]))
    return d


vals = {}
print("| list | arm | pairs | max \\|v − δ*(authors)\\| | > 1e-7 | cert: max DFD(t) − v | cert: DFD(t) > v + 1e-7 | cert min DFD(t) − v |")
print("|---|---|--:|--:|--:|--:|--:|--:|")
for lst, cdir, arm, ak in LISTS:
    f = f"{W}/{lst}_{arm}.csv"
    if not os.path.exists(f):
        print(f"| {lst} | {arm} | (missing) |"); continue
    rows = list(csv.DictReader(open(f)))
    vals[(lst, arm)] = rows
    out = subprocess.run(["/root/verif/cert/dfdcert", f, cdir], capture_output=True, text=True).stdout.split("\n")
    diffs = [float(l.split()[4]) for l in out if l.strip()]
    a_err, a_bad = "-", "-"
    if ak:
        A = auth(AUTH[ak])
        errs = []
        for r in rows:
            k = (r["file1"], r["file2"])
            if k in A:
                errs.append(min(abs(float(r["value"]) - x) for x in A[k]))
        a_err, a_bad = f"{max(errs):.2e} ({len(errs)})", sum(e > 1e-7 for e in errs)
    print(f"| {lst} | {arm} | {len(rows):,} | {a_err} | {a_bad} | {max(diffs):.2e} | {sum(d > 1e-7 for d in diffs)} | {min(diffs):.2e} |")

# agreement between arms
print()
for lst in ("chars_all_pairs", "chars_same_pairs", "chars_full_sub"):
    if (lst, "original") in vals and (lst, "candidate7") in vals:
        a, b = vals[(lst, "original")], vals[(lst, "candidate7")]
        d = [abs(float(x["value"]) - float(y["value"])) for x, y in zip(a, b)]
        print(f"{lst}: max |c7 − orig| = {max(d):.2e}, > 1e-7: {sum(x > 1e-7 for x in d)}")
if ("sig_pairs_998", "original") in vals and ("sig_pairs_1000", "candidate7") in vals:
    o = {(r["file1"], r["file2"]): float(r["value"]) for r in vals[("sig_pairs_998", "original")]}
    d = [abs(float(r["value"]) - o[(r["file1"], r["file2"])]) for r in vals[("sig_pairs_1000", "candidate7")] if (r["file1"], r["file2"]) in o]
    print(f"sig: max |c7 − orig| = {max(d):.2e} over {len(d)}, > 1e-7: {sum(x > 1e-7 for x in d)}")
# identity with the r4 timing runs (same binaries' code + one print)
r4c = {}
for arm in ("original", "candidate7"):
    for p in sorted(glob.glob(f"/root/r4/{arm}/c*.csv")):
        r4c.setdefault(arm, []).extend(csv.DictReader(open(p)))
for arm in ("original", "candidate7"):
    if ("chars_full_sub", arm) in vals:
        sub = r4c[arm][::10]
        same = sum(a["value"] == b["value"] and a["bbcalls"] == b["bbcalls"] for a, b in zip(vals[("chars_full_sub", arm)], sub))
        print(f"chars_full_sub {arm}: identical to r4 (value, bbcalls) {same}/{len(sub)}")
