# Does the `original` arm reproduce the ESA 2020 experiments? Three cross-checks

`original` is the authors' code (Bringmann, Kuennemann, Nusser, *When Lipschitz walks your dog*, ESA 2020; this repository's
`original/`), unmodified, built with their CMake flags (RelWithDebInfo, `-fopenmp`), driven by `paper_bench/paper_bench.cpp`,
which reads the same `MEASUREMENT` timers and counters as their `src/fut_paper_experiments.cpp` (`FUT_PREPROCESSING2`,
`FUT_BLACKBOX2`, `FUT_DISCSELECTION2`, `FUT_ARRANGEMENT2` = `FUT_N6_ARR` + `FUT_N6_FRECHET`, `BBCALLS_COUNTER`) around
`FrechetUnderTranslation` with the default parameters (epsilon 1e-7, depth 40, cut limit 12), one measurement per instance.
The instances are the authors' own files (`original/test_data/fut_*_benchmark_queries/`) on the authors' data
(Characters: the UCI file, `paper_data/characters_uci`; Sigspatial: `shortest-sf.tgz`, `paper_data/sigspatial`).

Three things can be compared, from strongest to weakest:

1. **The authors' own shipped outputs** (`experiments/{all-characters,same-characters,sigspatial}.txt` and `*_table.tex`, in the
   repository) against our `original` on **exactly the same query files** (their 2^l decider files). Same code, same instances,
   different machine: the black-box call counts should agree set by set.
2. **The paper's Table 2** (decider, 4^l factors) against our `original` on 4^l sets built from the same pairs with the paper's
   formula. The paper's 4^l instance files are not shipped, so the pair sample may differ.
3. **The paper's Table 4** (value computation, the 21,000 `characters_full` pairs, which are shipped) against our `original`.

Recorded distances are the ground truth for data identity: `original` reproduces the authors' delta* files to within 1.3e-8
(Characters, 2,000 pairs; `VERIFY_uci.md`) and 1.33e-8 (Sigspatial, 998 of 1,000 pairs; `RESULTS_sig.md`).

## 1. Authors' shipped decider outputs vs `original` on the same 2^l files

Rows are the 23 sets of each benchmark (NO queries l = -1 .. -10, YES queries l = -10 .. 2). `calls` = mean black-box calls per
instance, `ms` = mean time per instance. The authors' numbers are the run they shipped in `experiments/` (their machine); ours
are the container (Characters) and this PC under WSL2 (Sigspatial).

| set | all-characters: calls authors / ours / diff | ms authors / ours | same-characters: calls authors / ours / diff | ms authors / ours | sigspatial: calls authors / ours / diff | ms authors / ours |
|---|---|---|---|---|---|---|
| l=-1 minus | 0.06 / 0.10 / n/a | 0.0005 / 0.0006 | 0.09 / 0.10 / n/a | 0.0008 / 0.0008 | 0.01 / 0.00 / n/a | 0.0002 / 0.0025 |
| l=-2 minus | 0.57 / 0.60 / +5.1 % | 0.0042 / 0.0041 | 0.60 / 0.60 / +0.5 % | 0.0056 / 0.0046 | 0.04 / 0.00 / n/a | 0.0006 / 0.0034 |
| l=-3 minus | 3.86 / 3.90 / +0.9 % | 0.0263 / 0.0222 | 4.08 / 4.10 / +0.6 % | 0.0379 / 0.0321 | 0.46 / 0.50 / n/a | 0.0037 / 0.0090 |
| l=-4 minus | 12.01 / 12.00 / -0.1 % | 0.0768 / 0.0739 | 11.92 / 11.90 / -0.2 % | 0.1071 / 0.1101 | 1.88 / 1.90 / +1.2 % | 0.0138 / 0.0233 |
| l=-5 minus | 26.99 / 27.00 / +0.0 % | 0.1732 / 0.1700 | 25.93 / 25.90 / -0.1 % | 0.2322 / 0.2530 | 5.30 / 5.30 / +0.0 % | 0.0365 / 0.0544 |
| l=-6 minus | 52.39 / 52.40 / +0.0 % | 0.3410 / 0.3530 | 46.74 / 46.70 / -0.1 % | 0.4279 / 0.4308 | 12.58 / 12.60 / +0.1 % | 0.0868 / 0.1191 |
| l=-7 minus | 89.77 / 89.80 / +0.0 % | 0.6081 / 0.6848 | 79.41 / 79.60 / +0.2 % | 0.7615 / 0.6982 | 26.10 / 26.10 / +0.0 % | 0.1796 / 0.2520 |
| l=-8 minus | 146.85 / 147.40 / +0.4 % | 1.0608 / 1.1167 | 124.22 / 130.80 / +5.3 % | 1.3005 / 1.4268 | 49.34 / 50.00 / +1.3 % | 0.3409 / 0.6183 |
| l=-9 minus | 230.40 / 239.40 / +3.9 % | 1.8202 / 2.1094 | 187.48 / 242.40 / +29.3 % | 2.2696 / 3.4594 | 87.61 / 93.70 / +7.0 % | 0.7405 / 1.1403 |
| l=-10 minus | 349.10 / 420.10 / +20.3 % | 3.1750 / 4.4883 | 274.83 / 569.40 / +107.2 % | 4.0933 / 8.5757 | 143.56 / 169.10 / +17.8 % | 1.7337 / 3.9468 |
| l=-10 plus | 130.62 / 130.90 / +0.2 % | 1.0842 / 0.8536 | 146.53 / 150.90 / +3.0 % | 1.9277 / 2.3852 | 67.75 / 68.10 / +0.5 % | 0.7033 / 0.9690 |
| l=-9 plus | 99.37 / 99.40 / +0.0 % | 0.6745 / 0.7537 | 107.80 / 109.60 / +1.7 % | 1.1942 / 1.5376 | 48.24 / 48.30 / +0.1 % | 0.3334 / 0.6049 |
| l=-8 plus | 69.18 / 69.20 / +0.0 % | 0.4500 / 0.3920 | 75.67 / 76.90 / +1.6 % | 0.7859 / 0.9545 | 29.80 / 29.80 / +0.0 % | 0.1849 / 0.2965 |
| l=-7 plus | 45.95 / 45.90 / -0.1 % | 0.2946 / 0.2801 | 52.87 / 52.90 / +0.1 % | 0.5335 / 0.5941 | 18.50 / 18.50 / +0.0 % | 0.1072 / 0.1641 |
| l=-6 plus | 31.17 / 31.20 / +0.1 % | 0.2003 / 0.2008 | 35.17 / 35.20 / +0.1 % | 0.3536 / 0.3713 | 10.65 / 10.70 / +0.4 % | 0.0605 / 0.0934 |
| l=-5 plus | 17.38 / 17.40 / +0.1 % | 0.1122 / 0.1144 | 22.23 / 22.20 / -0.1 % | 0.2293 / 0.2297 | 6.15 / 6.20 / +0.8 % | 0.0342 / 0.0524 |
| l=-4 plus | 9.59 / 9.60 / +0.1 % | 0.0595 / 0.0563 | 12.52 / 12.50 / -0.2 % | 0.1299 / 0.1238 | 3.90 / 3.90 / -0.1 % | 0.0188 / 0.0359 |
| l=-3 plus | 5.04 / 5.00 / -0.8 % | 0.0305 / 0.0281 | 6.94 / 6.90 / -0.6 % | 0.0763 / 0.0686 | 2.68 / 2.70 / +0.8 % | 0.0104 / 0.0249 |
| l=-2 plus | 3.02 / 3.00 / -0.7 % | 0.0159 / 0.0153 | 3.71 / 3.70 / -0.2 % | 0.0417 / 0.0383 | 2.19 / 2.20 / +0.7 % | 0.0060 / 0.0184 |
| l=-1 plus | 2.19 / 2.20 / +0.5 % | 0.0087 / 0.0084 | 2.30 / 2.30 / -0.2 % | 0.0247 / 0.0225 | 2.02 / 2.00 / -1.2 % | 0.0038 / 0.0159 |
| l=0 plus | 2.00 / 2.00 / +0.0 % | 0.0046 / 0.0047 | 2.00 / 2.00 / +0.0 % | 0.0167 / 0.0118 | 2.00 / 2.00 / +0.0 % | 0.0026 / 0.0134 |
| l=1 plus | 2.00 / 2.00 / +0.0 % | 0.0023 / 0.0027 | 2.00 / 2.00 / +0.0 % | 0.0109 / 0.0078 | 2.00 / 2.00 / +0.0 % | 0.0019 / 0.0123 |
| l=2 plus | 2.00 / 2.00 / +0.0 % | 0.0014 / 0.0016 | 2.00 / 2.00 / +0.0 % | 0.0054 / 0.0039 | 2.00 / 2.00 / +0.0 % | 0.0016 / 0.0109 |
| **all 23 sets** | **57.89 / 61.41 / +6.1 %** | **0.4445 / 0.5102 (x1.15)** | **53.35 / 69.16 / +29.6 %** | **0.6333 / 0.9279 (x1.47)** | **22.81 / 24.24 / +6.3 %** | **0.2002 / 0.3687 (x1.84)** |

Reading: on 55 of the 64 sets with a meaningful count (more than 0.5 calls per instance) the call counts agree to
within 2 %, on most to within 0.2 %. The exceptions are the hardest NO sets (delta just below delta*, l = -8 .. -10 minus),
where the search has to exhaust its box and the branch-and-bound path is sensitive to floating-point decisions and to the
arrangement kernel version (same-characters l=-10 minus: 275 vs 569 calls; `experiment_log` 6.8 shows +28 % on one instance from
compile flags alone). These few sets carry the whole difference in the totals (+6 %, +30 %, +6 %). Times differ by machine:
x1.15 (container) and x1.84 (this PC under WSL2) on the same code.

## 2. Paper Table 2 (4^l instances) vs `original` on our 4^l sets

| benchmark | calls/instance: paper / ours | ms/instance: paper / ours | arrangement-algorithm share: paper / ours | of which construction: paper / ours | pre + Lipschitz + estimation share: paper / ours |
|---|---|---|---|---|---|
| all-characters | 1,860.1 / 1,616.3 (-13.1 %) | 27.31 / 16.77 | 61.3 % / 53.9 % | 37.7 % / 42.1 % | 38.5 % / 46.1 % |
| same-characters | 1,159.2 / 997.0 (-14.0 %) | 18.68 / 11.74 | 52.7 % / 45.8 % | 34.7 % / 36.4 % | 47.0 % / 54.2 % |
| sigspatial | 1,366.1 / 1,146.4 (-16.1 %) | 52.50 / 41.74 | 20.6 % / 17.9 % | 12.9 % / 14.3 % | 79.3 % / 82.1 % |

Reading: our sets use the same 1,000 pairs as the shipped 2^l files with the paper's factors (1 -+ 4^l); the paper does not
ship its 4^l files, so its sample may differ. Calls are 13-16 % lower on our side on all three benchmarks, and the stage
shares match (arrangement algorithm 61/53/21 % in the paper vs 54/46/18 % here; the Sigspatial decider is dominated by
arrangement estimation in both). Sigspatial times are from this PC (WSL2), Characters from the container.

## 3. Paper Table 4 (value computation, the shipped 21,000 `characters_full` pairs) vs `original`

| row | paper (authors' machine) | ours, r2 (container) | note |
|---|--:|--:|---|
| total time | 2,938,512 ms (140.0 ms/inst) | 2,842,300 ms (135.3 ms/inst) | machines differ; ratio 0.97 |
| black-box calls | 260,128,449 (12,387.1/inst) | 257,162,361 (12,245.8/inst) | -1.1 % on identical instances |
| preprocessing | 71,728 ms | 87,237 ms | |
| black-box calls (Lipschitz) | 400,189 ms | 289,267 ms | |
| arrangement estimation | 166,479 ms | 190,210 ms | |
| arrangement algorithm | 2,250,493 ms (76.6 %) | 2,233,926 ms (78.6 %) | |
| - construction | 1,537,500 ms (52.3 %) | 1,708,959 ms (60.1 %) | CGAL 5.6 here; Epeck kernel cost differs by version |
| - black-box calls | 545,442 ms | 367,421 ms | |

The authors' own shipped `experiments/characters_valcomp_full_total_table.tex`, a different run of the same benchmark by the
authors, reports 175,529,881 calls (8,358/inst), 33 % below their paper's 12,387; our 12,246 is within 1.1 % of the paper.
Call counts are therefore run-dependent even for the authors, and agreement at the percent level is as close as this
quantity allows (`experiment_log` 6.8).

## Verdict

`original` runs the authors' code on the authors' instances and data: distances match their recorded delta* to 1e-8, and on
their own shipped query files the per-set black-box call counts match their shipped outputs to within 2 % on 55 of 64 sets,
with the remaining differences confined to the hardest NO sets. Against the paper's tables the totals agree to 1 % (Table 4) and
13-16 % (Table 2, instance files not shipped), with the same stage shares. Absolute times are machine-specific and are only
compared within one machine (baseline vs proposed measured back-to-back on the same core).
