# Does the `original` arm reproduce the ESA 2020 experiments? Three cross-checks

`original` is the authors' code (Bringmann, Kuennemann, Nusser, *When Lipschitz walks your dog*, ESA 2020; this repository's
`original/`), built with their CMake flags (RelWithDebInfo, `-fopenmp`) and driven by `paper_bench/paper_bench.cpp`, which reads
the same `MEASUREMENT` timers and counters as their `src/fut_paper_experiments.cpp` around `FrechetUnderTranslation` with the
default parameters (epsilon 1e-7, depth 40, cut limit 12), one measurement per instance. The algorithm is unmodified; the only
change is the clock of the measurement library (`steady_clock` instead of libstdc++'s `high_resolution_clock` = `system_clock`,
which WSL2 steps at time syncs; `experiment_log` 6.12). The instances are the authors' own files
(`original/test_data/fut_*_benchmark_queries/`) on the authors' data (Characters: the UCI file, `paper_data/characters_uci`;
Sigspatial: `shortest-sf.tgz`, `paper_data/sigspatial`).

Three things can be compared, from strongest to weakest:

1. **The authors' own shipped outputs** (`experiments/{all-characters,same-characters,sigspatial}.txt`, in the repository)
   against our `original` on **exactly the same query files** (their 2^l decider files). Same code, same instances,
   different machine: the black-box call counts should agree set by set.
2. **The paper's Table 2** (decider, 4^l factors) against our `original` on 4^l sets built from the same pairs with the paper's
   formula. The paper's 4^l instance files are not shipped, so the pair sample may differ.
3. **The paper's Table 4** (value computation, the 21,000 `characters_full` pairs, which are shipped) against our `original`.

Recorded distances are the ground truth for data identity: `original` reproduces the authors' delta* files to within 1.3e-8
(Characters, 2,000 pairs; `VERIFY_uci.md`) and 1.33e-8 (Sigspatial, 998 of 1,000 pairs; `experiment_log` 6.9, and recomputed
from `raw_sigspatial_lmf_r3.tar.gz` against `queries/sigspatial_paperq_computed_distances.check`).

The Characters rows come from the container runs, which predate the clock change; they show no clock anomaly (`experiment_log` 6.12).

## 1. Authors' shipped decider outputs vs `original` on the same 2^l files

Rows are the 23 sets of each benchmark (NO queries l = -1 .. -10, YES queries l = -10 .. 2). `calls` = mean black-box calls per
instance, `ms` = mean time per instance. The authors' numbers are the run they shipped in `experiments/` (their machine); ours
are all-characters: container, r1, same-characters: container, r1, sigspatial: user PC, WSL2, r3.

| set | all-characters: calls authors / ours / diff | ms authors / ours | same-characters: calls authors / ours / diff | ms authors / ours | sigspatial: calls authors / ours / diff | ms authors / ours |
|---|---|---|---|---|---|---|
| l=-1 minus | 0.06 / 0.06 / n/a | 0.0005 / 0.0006 | 0.09 / 0.09 / n/a | 0.0008 / 0.0008 | 0.01 / 0.01 / n/a | 0.0002 / 0.0007 |
| l=-2 minus | 0.57 / 0.57 / +0.0 % | 0.0042 / 0.0041 | 0.60 / 0.60 / +0.0 % | 0.0056 / 0.0046 | 0.04 / 0.04 / n/a | 0.0006 / 0.0014 |
| l=-3 minus | 3.86 / 3.86 / +0.0 % | 0.0263 / 0.0222 | 4.08 / 4.08 / +0.0 % | 0.0379 / 0.0321 | 0.46 / 0.46 / n/a | 0.0037 / 0.0044 |
| l=-4 minus | 12.01 / 12.01 / +0.0 % | 0.0768 / 0.0739 | 11.92 / 11.92 / +0.0 % | 0.1071 / 0.1101 | 1.88 / 1.88 / +0.0 % | 0.0138 / 0.0127 |
| l=-5 minus | 26.99 / 26.99 / +0.0 % | 0.1732 / 0.1700 | 25.93 / 25.93 / +0.0 % | 0.2322 / 0.2530 | 5.30 / 5.30 / +0.0 % | 0.0365 / 0.0331 |
| l=-6 minus | 52.39 / 52.39 / +0.0 % | 0.3410 / 0.3530 | 46.74 / 46.74 / +0.0 % | 0.4279 / 0.4308 | 12.58 / 12.58 / +0.0 % | 0.0868 / 0.0777 |
| l=-7 minus | 89.77 / 89.77 / +0.0 % | 0.6081 / 0.6848 | 79.41 / 79.55 / +0.2 % | 0.7615 / 0.6982 | 26.10 / 26.10 / +0.0 % | 0.1796 / 0.1646 |
| l=-8 minus | 146.85 / 147.36 / +0.3 % | 1.0608 / 1.1167 | 124.22 / 130.80 / +5.3 % | 1.3005 / 1.4268 | 49.34 / 49.99 / +1.3 % | 0.3409 / 0.4052 |
| l=-9 minus | 230.40 / 239.40 / +3.9 % | 1.8202 / 2.1094 | 187.48 / 242.39 / +29.3 % | 2.2696 / 3.4594 | 87.61 / 93.70 / +7.0 % | 0.7405 / 1.1182 |
| l=-10 minus | 349.10 / 420.11 / +20.3 % | 3.1750 / 4.4883 | 274.83 / 569.40 / +107.2 % | 4.0933 / 8.5757 | 143.56 / 169.13 / +17.8 % | 1.7337 / 2.8600 |
| l=-10 plus | 130.62 / 130.89 / +0.2 % | 1.0842 / 0.8536 | 146.53 / 150.94 / +3.0 % | 1.9277 / 2.3852 | 67.75 / 68.13 / +0.6 % | 0.7033 / 0.7409 |
| l=-9 plus | 99.37 / 99.37 / +0.0 % | 0.6745 / 0.7537 | 107.80 / 109.58 / +1.6 % | 1.1942 / 1.5376 | 48.24 / 48.34 / +0.2 % | 0.3334 / 0.4290 |
| l=-8 plus | 69.18 / 69.18 / +0.0 % | 0.4500 / 0.3920 | 75.67 / 76.93 / +1.7 % | 0.7859 / 0.9545 | 29.80 / 29.80 / +0.0 % | 0.1849 / 0.2026 |
| l=-7 plus | 45.95 / 45.95 / +0.0 % | 0.2946 / 0.2801 | 52.87 / 52.87 / +0.0 % | 0.5335 / 0.5941 | 18.50 / 18.50 / +0.0 % | 0.1072 / 0.1138 |
| l=-6 plus | 31.17 / 31.17 / +0.0 % | 0.2003 / 0.2008 | 35.17 / 35.17 / +0.0 % | 0.3536 / 0.3713 | 10.65 / 10.65 / +0.0 % | 0.0605 / 0.0612 |
| l=-5 plus | 17.38 / 17.38 / +0.0 % | 0.1122 / 0.1144 | 22.23 / 22.23 / +0.0 % | 0.2293 / 0.2297 | 6.15 / 6.15 / +0.0 % | 0.0342 / 0.0360 |
| l=-4 plus | 9.59 / 9.59 / +0.0 % | 0.0595 / 0.0563 | 12.52 / 12.52 / +0.0 % | 0.1299 / 0.1238 | 3.90 / 3.90 / +0.0 % | 0.0188 / 0.0223 |
| l=-3 plus | 5.04 / 5.04 / +0.0 % | 0.0305 / 0.0281 | 6.94 / 6.94 / +0.0 % | 0.0763 / 0.0686 | 2.68 / 2.68 / +0.0 % | 0.0104 / 0.0140 |
| l=-2 plus | 3.02 / 3.02 / +0.0 % | 0.0159 / 0.0153 | 3.71 / 3.71 / +0.0 % | 0.0417 / 0.0383 | 2.19 / 2.19 / +0.0 % | 0.0060 / 0.0087 |
| l=-1 plus | 2.19 / 2.19 / +0.0 % | 0.0087 / 0.0084 | 2.30 / 2.30 / +0.0 % | 0.0247 / 0.0225 | 2.02 / 2.02 / +0.0 % | 0.0038 / 0.0078 |
| l=0 plus | 2.00 / 2.00 / +0.0 % | 0.0046 / 0.0047 | 2.00 / 2.00 / +0.0 % | 0.0167 / 0.0118 | 2.00 / 2.00 / +0.0 % | 0.0026 / 0.0047 |
| l=1 plus | 2.00 / 2.00 / +0.0 % | 0.0023 / 0.0027 | 2.00 / 2.00 / +0.0 % | 0.0109 / 0.0078 | 2.00 / 2.00 / +0.0 % | 0.0019 / 0.0042 |
| l=2 plus | 2.00 / 2.00 / +0.0 % | 0.0014 / 0.0016 | 2.00 / 2.00 / +0.0 % | 0.0054 / 0.0039 | 2.00 / 2.00 / +0.0 % | 0.0016 / 0.0034 |
| **all 23 sets** | **57.89 / 61.40 / +6.1 %** | **0.4445 / 0.5102 (x1.15)** | **53.35 / 69.16 / +29.6 %** | **0.6333 / 0.9279 (x1.47)** | **22.81 / 24.24 / +6.2 %** | **0.2002 / 0.2751 (x1.37)** |

Reading: on 56 of the 64 sets with a meaningful count (more than 0.5 calls per instance) the call counts agree to
within 2 % (50 to within 0.2 %). The 8 exceptions are mostly the sets closest to delta*, where the search has to
exhaust its box and the branch-and-bound path is sensitive to floating-point decisions and to the arrangement kernel version:
all-characters l=-9 minus (+4 %); all-characters l=-10 minus (+20 %); same-characters l=-8 minus (+5 %); same-characters l=-9 minus (+29 %); same-characters l=-10 minus (+107 %); same-characters l=-10 plus (+3 %); sigspatial l=-9 minus (+7 %); sigspatial l=-10 minus (+18 %).
(The largest: same-characters l=-10 minus, 275 vs 569 calls; `experiment_log` 6.8 shows +28 % on one instance from compile
flags alone.) These few sets carry nearly all of the difference in the totals (+6 %, +30 %, +6 %). Times differ by machine and,
where our totals have more calls, by the extra work: x1.15 (all-characters, container, r1), x1.47 (same-characters, container, r1), x1.37 (sigspatial, user PC, WSL2, r3).

## 2. Paper Table 2 (4^l instances) vs `original` on our 4^l sets

| benchmark | calls/instance: paper / ours | ms/instance: paper / ours | construction + black-box calls inside the arrangement algorithm (N6_ARR + N6_FRECHET), share: paper / ours | of which construction: paper / ours | everything else (100 % minus that), share: paper / ours |
|---|---|---|---|---|---|
| all-characters | 1,860.1 / 1,616.3 (-13.1 %) | 27.31 / 16.77 | 56.9 % / 53.9 % | 37.7 % / 42.1 % | 43.1 % / 46.1 % |
| same-characters | 1,159.2 / 997.0 (-14.0 %) | 18.68 / 11.74 | 48.7 % / 45.8 % | 34.7 % / 36.4 % | 51.3 % / 54.2 % |
| sigspatial | 1,366.1 / 1,146.4 (-16.1 %) | 52.50 / 38.70 | 19.0 % / 17.6 % | 12.9 % / 14.0 % | 81.0 % / 82.4 % |

Reading: our sets use the same 1,000 pairs as the shipped 2^l files with the paper's factors (1 -+ 4^l); the paper does not
ship its 4^l files, so its sample may differ. Calls are 13-16 % lower on our side on all three benchmarks, and the stage
shares match (construction + its black-box calls 57/49/19 % in the paper vs 54/46/18 % here; the Sigspatial
decider is dominated by arrangement estimation in both). Characters rows: container (r1); Sigspatial: user PC, WSL2 (r3).
Table E's 'arrangement algorithm' row is the enclosing FUT_ARRANGEMENT1 timer (it also covers the untimed rest of that stage),
and its Characters rows come from the WSL r3 run, not the container r1 run used here; its shares are therefore not directly
comparable with this column.

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
their own shipped query files the per-set black-box call counts match their shipped outputs to within 2 % on 56 of 64 sets,
with the remaining differences confined to the hardest NO sets. Against the paper's tables the totals agree to 1 % (Table 4) and
13-16 % (Table 2, instance files not shipped), with the same stage shares. Absolute times are machine-specific and are only
compared within one machine (baseline vs proposed measured back-to-back on the same core or vCPU).
