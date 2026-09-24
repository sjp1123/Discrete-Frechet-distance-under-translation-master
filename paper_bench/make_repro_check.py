#!/usr/bin/env python3
"""results/REPRO_check.md: does the `original` arm reproduce the ESA 2020 experiments?

Three cross-checks, from strongest to weakest:
  1. the authors' own shipped decider outputs (experiments/{all-characters,same-characters,sigspatial}.txt)
     against `original` on the same 2^l query files (per set: black-box calls, time);
  2. the paper's Table 2 (4^l) against `original` on our 4^l sets (same pairs, paper formula);
  3. the paper's Table 4 against the Characters value computation (container, r2).
Characters decider: container, r1 (RESULTS_uci.md, raw_characters_uci_decider.tar.gz).
Sigspatial decider: user PC WSL2, r3 (RESULTS_sig.md, raw_sigspatial_decider_r3.tar.gz).

    python3 paper_bench/make_repro_check.py
"""
import csv
import io
import os
import re
import tarfile

HERE = os.path.dirname(os.path.abspath(__file__)); ROOT = os.path.dirname(HERE)
R = os.path.join(HERE, "results")
NL = "\n"
order = [(l, "minus") for l in range(-1, -11, -1)] + [(l, "plus") for l in range(-10, 3)]


def authors(name):
    rows = [l.split() for l in open(os.path.join(ROOT, "experiments", f"{name}.txt")) if l.strip()]
    assert len(rows) == 23
    return {order[i]: (float(r[0]), float(r[3])) for i, r in enumerate(rows)}   # (ms per instance, calls per instance)


def ours(tar, prefix, rep):
    """Per-set mean (ms per instance, calls per instance) of the original arm on the 2^l files, from the raw CSVs
    (not from the rounded Markdown reports)."""
    d = {}
    with tarfile.open(os.path.join(R, tar)) as tf:
        for m in tf:
            mm = re.fullmatch(prefix + r"_(-?\d+)_(plus|minus)_original_" + rep + r"\.csv", m.name)
            if not mm: continue
            rows = list(csv.DictReader(io.TextIOWrapper(tf.extractfile(m), encoding="utf-8")))
            assert len(rows) == 1000, (m.name, len(rows))
            d[(int(mm.group(1)), mm.group(2))] = (sum(float(r["time_ms"]) for r in rows) / 1000, sum(int(r["bbcalls"]) for r in rows) / 1000)
    assert len(d) == 23, (tar, prefix, len(d))
    return d


def our4(tar, prefix, rep):
    T = Ar = F = C = N = 0
    with tarfile.open(os.path.join(R, tar)) as tf:
        for m in tf:
            if re.fullmatch(prefix + r"_-?\d+_(plus|minus)_original_" + rep + r"\.csv", m.name):
                for r in csv.DictReader(io.TextIOWrapper(tf.extractfile(m), encoding="utf-8")):
                    T += float(r["time_ms"]); Ar += float(r["n6_arr_ms"]); F += float(r["n6_fre_ms"]); C += int(r["bbcalls"]); N += 1
    assert N == 23000, (tar, prefix, N)
    return T, Ar, F, C, N


sets = [("all-characters", ("raw_characters_uci_decider.tar.gz", "characters_uci_all_paperq", "r1"), None, "container, r1"),
        ("same-characters", ("raw_characters_uci_decider.tar.gz", "characters_uci_same_paperq", "r1"), None, "container, r1"),
        ("sigspatial", ("raw_sigspatial_decider_r3.tar.gz", "sigspatial_paperq", "r3"), None, "user PC, WSL2, r3")]
A = {n: authors(n) for n, _, _, _ in sets}
O = {n: ours(*src) for n, src, _, _ in sets}

L = ["# Does the `original` arm reproduce the ESA 2020 experiments? Three cross-checks", "",
     "`original` is the authors' code (Bringmann, Kuennemann, Nusser, *When Lipschitz walks your dog*, ESA 2020; this repository's",
     "`original/`), built with their CMake flags (RelWithDebInfo, `-fopenmp`) and driven by `paper_bench/paper_bench.cpp`, which reads",
     "the same `MEASUREMENT` timers and counters as their `src/fut_paper_experiments.cpp` around `FrechetUnderTranslation` with the",
     "default parameters (epsilon 1e-7, depth 40, cut limit 12), one measurement per instance. The algorithm is unmodified; the only",
     "change is the clock of the measurement library (`steady_clock` instead of libstdc++'s `high_resolution_clock` = `system_clock`,",
     "which WSL2 steps at time syncs; `experiment_log` 6.12). The instances are the authors' own files",
     "(`original/test_data/fut_*_benchmark_queries/`) on the authors' data (Characters: the UCI file, `paper_data/characters_uci`;",
     "Sigspatial: `shortest-sf.tgz`, `paper_data/sigspatial`).", "",
     "Three things can be compared, from strongest to weakest:", "",
     "1. **The authors' own shipped outputs** (`experiments/{all-characters,same-characters,sigspatial}.txt`, in the repository)",
     "   against our `original` on **exactly the same query files** (their 2^l decider files). Same code, same instances,",
     "   different machine: the black-box call counts should agree set by set.",
     "2. **The paper's Table 2** (decider, 4^l factors) against our `original` on 4^l sets built from the same pairs with the paper's",
     "   formula. The paper's 4^l instance files are not shipped, so the pair sample may differ.",
     "3. **The paper's Table 4** (value computation, the 21,000 `characters_full` pairs, which are shipped) against our `original`.", "",
     "Recorded distances are the ground truth for data identity: `original` reproduces the authors' delta* files to within 1.3e-8",
     "(Characters, 2,000 pairs; `VERIFY_uci.md`) and 1.33e-8 (Sigspatial, 998 of 1,000 pairs; `experiment_log` 6.9, and recomputed",
     "from `raw_sigspatial_lmf_r3.tar.gz` against `queries/sigspatial_paperq_computed_distances.check`).", "",
     "The Characters rows come from the container runs, which predate the clock change; they show no clock anomaly (`experiment_log` 6.12).", "",
     "## 1. Authors' shipped decider outputs vs `original` on the same 2^l files", "",
     "Rows are the 23 sets of each benchmark (NO queries l = -1 .. -10, YES queries l = -10 .. 2). `calls` = mean black-box calls per",
     "instance, `ms` = mean time per instance. The authors' numbers are the run they shipped in `experiments/` (their machine); ours",
     "are " + ", ".join(f"{n}: {where}" for n, _, _, where in sets) + ".", "",
     "| set | " + " | ".join(f"{n}: calls authors / ours / diff | ms authors / ours" for n, _, _, _ in sets) + " |",
     "|---|" + "---|---|" * 3]
for k in order:
    cells = []
    for n, _, _, _ in sets:
        a = A[n][k]; o = O[n][k]
        diff = f"{100 * (o[1] - a[1]) / a[1]:+.1f} %" if a[1] > 0.5 else "n/a"
        cells.append(f"{a[1]:.2f} / {o[1]:.2f} / {diff} | {a[0]:.4f} / {o[0]:.4f}")
    L.append(f"| l={k[0]} {k[1]} | " + " | ".join(cells) + " |")
cells = []; within2 = counted = within02 = 0; tot_diff = []; time_ratio = []; worst = None; exceptions = []
for n, _, _, where in sets:
    a = sum(v[1] for v in A[n].values()) / 23; o = sum(v[1] for v in O[n].values()) / 23
    ta = sum(v[0] for v in A[n].values()) / 23; to = sum(v[0] for v in O[n].values()) / 23
    cells.append(f"**{a:.2f} / {o:.2f} / {100 * (o - a) / a:+.1f} %** | **{ta:.4f} / {to:.4f} (x{to / ta:.2f})**")
    tot_diff.append(f"{100 * (o - a) / a:+.0f} %"); time_ratio.append(f"x{to / ta:.2f} ({n}, {where})")
    for k in order:
        if A[n][k][1] > 0.5:
            counted += 1; rel = (O[n][k][1] - A[n][k][1]) / A[n][k][1]
            if abs(rel) <= 0.02: within2 += 1
            else: exceptions.append(f"{n} l={k[0]} {k[1]} ({100 * rel:+.0f} %)")
            if abs(rel) <= 0.002: within02 += 1
            if worst is None or abs(rel) > abs(worst[0]): worst = (rel, n, k, A[n][k][1], O[n][k][1])
L.append("| **all 23 sets** | " + " | ".join(cells) + " |")
L += ["", f"Reading: on {within2} of the {counted} sets with a meaningful count (more than 0.5 calls per instance) the call counts agree to",
      f"within 2 % ({within02} to within 0.2 %). The {len(exceptions)} exceptions are mostly the sets closest to delta*, where the search has to",
      "exhaust its box and the branch-and-bound path is sensitive to floating-point decisions and to the arrangement kernel version:",
      "; ".join(exceptions) + ".",
      f"(The largest: {worst[1]} l={worst[2][0]} {worst[2][1]}, {worst[3]:.0f} vs {worst[4]:.0f} calls; `experiment_log` 6.8 shows +28 % on one instance from compile",
      f"flags alone.) These few sets carry nearly all of the difference in the totals ({', '.join(tot_diff)}). Times differ by machine and,",
      "where our totals have more calls, by the extra work: " + ", ".join(time_ratio) + ".", "",
      "## 2. Paper Table 2 (4^l instances) vs `original` on our 4^l sets", ""]
paper = {"all-characters": (628043, 42781931, 5, 50462, 191177, 385145, 237043, 120149),
         "same-characters": (429623, 26661524, 5, 44312, 157780, 226469, 148898, 60156),
         "sigspatial": (1207560, 31420517, 5, 43861, 913266, 249268, 155332, 73934)}
L += ["| benchmark | calls/instance: paper / ours | ms/instance: paper / ours | construction + black-box calls inside the arrangement algorithm (N6_ARR + N6_FRECHET), share: paper / ours | of which construction: paper / ours | everything else (100 % minus that), share: paper / ours |",
      "|---|---|---|---|---|---|"]
call_diffs = []; shares = []
for name, tar, prefix, rep in (("all-characters", "raw_characters_uci_decider.tar.gz", "characters_uci_all_paperq4", "r1"),
                               ("same-characters", "raw_characters_uci_decider.tar.gz", "characters_uci_same_paperq4", "r1"),
                               ("sigspatial", "raw_sigspatial_decider_r3.tar.gz", "sigspatial_paperq4", "r3")):
    T, Ar, F, C, N = our4(tar, prefix, rep); p = paper[name]
    d = 100 * (C / N - p[1] / 23000) / (p[1] / 23000); call_diffs.append(d)
    # one definition on both sides: construction + black-box calls of the arrangement algorithm (N6_ARR + N6_FRECHET);
    # the r1 container rows have no enclosing FUT_ARRANGEMENT1 timer, so that timer is not used here
    shares.append((f"{100 * (p[6] + p[7]) / p[0]:.0f}", f"{100 * (Ar + F) / T:.0f}"))
    L.append(f"| {name} | {p[1] / 23000:,.1f} / {C / N:,.1f} ({d:+.1f} %) | {p[0] / 23000:.2f} / {T / N:.2f} | {100 * (p[6] + p[7]) / p[0]:.1f} % / {100 * (Ar + F) / T:.1f} % | {100 * p[6] / p[0]:.1f} % / {100 * Ar / T:.1f} % | {100 * (p[0] - p[6] - p[7]) / p[0]:.1f} % / {100 * (T - Ar - F) / T:.1f} % |")
lo, hi = sorted([-max(call_diffs), -min(call_diffs)])
L += ["", "Reading: our sets use the same 1,000 pairs as the shipped 2^l files with the paper's factors (1 -+ 4^l); the paper does not",
      f"ship its 4^l files, so its sample may differ. Calls are {lo:.0f}-{hi:.0f} % lower on our side on all three benchmarks, and the stage",
      f"shares match (construction + its black-box calls {'/'.join(s[0] for s in shares)} % in the paper vs {'/'.join(s[1] for s in shares)} % here; the Sigspatial",
      "decider is dominated by arrangement estimation in both). Characters rows: container (r1); Sigspatial: user PC, WSL2 (r3).",
      "Table E's 'arrangement algorithm' row is the enclosing FUT_ARRANGEMENT1 timer (it also covers the untimed rest of that stage),",
      "and its Characters rows come from the WSL r3 run, not the container r1 run used here; its shares are therefore not directly",
      "comparable with this column.", "",
      "## 3. Paper Table 4 (value computation, the shipped 21,000 `characters_full` pairs) vs `original`", "",
      "| row | paper (authors' machine) | ours, r2 (container) | note |", "|---|--:|--:|---|",
      "| total time | 2,938,512 ms (140.0 ms/inst) | 2,842,300 ms (135.3 ms/inst) | machines differ; ratio 0.97 |",
      "| black-box calls | 260,128,449 (12,387.1/inst) | 257,162,361 (12,245.8/inst) | -1.1 % on identical instances |",
      "| preprocessing | 71,728 ms | 87,237 ms | |",
      "| black-box calls (Lipschitz) | 400,189 ms | 289,267 ms | |",
      "| arrangement estimation | 166,479 ms | 190,210 ms | |",
      "| arrangement algorithm | 2,250,493 ms (76.6 %) | 2,233,926 ms (78.6 %) | |",
      "| - construction | 1,537,500 ms (52.3 %) | 1,708,959 ms (60.1 %) | CGAL 5.6 here; Epeck kernel cost differs by version |",
      "| - black-box calls | 545,442 ms | 367,421 ms | |", "",
      "The authors' own shipped `experiments/characters_valcomp_full_total_table.tex`, a different run of the same benchmark by the",
      "authors, reports 175,529,881 calls (8,358/inst), 33 % below their paper's 12,387; our 12,246 is within 1.1 % of the paper.",
      "Call counts are therefore run-dependent even for the authors, and agreement at the percent level is as close as this",
      "quantity allows (`experiment_log` 6.8).", "",
      "## Verdict", "",
      "`original` runs the authors' code on the authors' instances and data: distances match their recorded delta* to 1e-8, and on",
      f"their own shipped query files the per-set black-box call counts match their shipped outputs to within 2 % on {within2} of {counted} sets,",
      "with the remaining differences confined to the hardest NO sets. Against the paper's tables the totals agree to 1 % (Table 4) and",
      f"{lo:.0f}-{hi:.0f} % (Table 2, instance files not shipped), with the same stage shares. Absolute times are machine-specific and are only",
      "compared within one machine (baseline vs proposed measured back-to-back on the same core or vCPU)."]
open(os.path.join(R, "REPRO_check.md"), "w", encoding="utf-8", newline=NL).write(NL.join(L) + NL)
print("REPRO_check.md:", len(L), "lines;", within2, "of", counted, "sets within 2 %; Table 2 call diffs", [f"{d:+.1f}" for d in call_diffs])
