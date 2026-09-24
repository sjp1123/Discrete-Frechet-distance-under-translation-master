# Paper tables — the authors' data and instances (Characters UCI file; Sigspatial full set)

Tables A-C (container, Characters): `RESULTS_uci_r2.md` for LMF (raw: `raw_characters_uci_lmf_r2.tar.gz`; original,
candidate5_noslack and candidate5_exact measured back-to-back on one idle core of the same container instance) and
`RESULTS_uci.md` for the decider (raw: `raw_characters_uci_decider.tar.gz`, earlier container instance —
absolute times are not comparable across the two instances). One measurement per instance.
baseline = original (CGAL arrangement); proposed = candidate5 with MAXREGION_EXACT=1 and MAXREGION_SLACK=0
(filtered P2/P3 predicates with rational fallback) for LMF, default candidate5 for the Table A decider.
Tables D-E (user PC, WSL2, r3; `experiment_log` 6.12): Sigspatial value computation and the decider on all three benchmarks,
proposed = candidate5 with MAXREGION_EXACT=1 and MAXREGION_SLACK=0 throughout; absolute times are not comparable with A-C.
Paper columns from Bringmann–Künnemann–Nusser, ESA 2020.

## Table A — Decision problem, (1 ± 4^ℓ)·δ instances, 1,000 pairs × 23 sets

| Data set | Time / instance (ms) baseline | proposed | speed-up | Black-box calls / instance baseline | proposed | Paper Table 2 (baseline) | wrong / disagree |
|---|--:|--:|--:|--:|--:|---|--:|
| all-characters | 16.77 | 7.46 | **2.25×** | 1,616 | 386 | 27.3 ms, 1,860 calls | 0 / 0 |
| same-characters | 11.74 | 6.35 | **1.85×** | 997 | 247 | 18.7 ms, 1,159 calls | 0 / 0 |

Note on the Paper Table 4 column: the black-box call count is not a deterministic function of the
instance. Rebuilding the same `original` code with `-O3 -march=native` instead of the default
RelWithDebInfo changes the count on 46 of the first 299 instances (total +0.10 %, one instance
+28 %) while every value agrees to 1.4e-14 (`raw_characters_uci_fpsens.tar.gz`, experiment_log §6.8);
the 1.1 % gap to the authors' 12,387.1 is within that environment sensitivity.

```latex
\begin{table}[t]
\centering
\caption{Decision problem on the instances of~\cite{BKN20}: the authors' 1{,}000 pairs
per data set, 23 sets with distance factors $(1\pm4^{\ell})$, 23{,}000 instances per row,
one measurement each on a single core. The last column gives the baseline numbers
reported in~\cite{BKN20} (Table~2) on the authors' machine.}
\label{tab:decider}
\begin{tabular}{lrrrrrr}
\toprule
& \multicolumn{3}{c}{Time / instance (ms)} & \multicolumn{2}{c}{Black-box calls / instance} & \cite{BKN20} \\
\cmidrule(lr){2-4}\cmidrule(lr){5-6}
Data set & baseline & proposed & speed-up & baseline & proposed & ms / calls \\
\midrule
all-characters  & 16.77 & 7.46 & 2.25$\times$ & 1{,}616 & 386 & 27.3 / 1{,}860 \\
same-characters & 11.74 & 6.35 & 1.85$\times$ &    997  & 247 & 18.7 / 1{,}159 \\
\bottomrule
\end{tabular}
\end{table}
```

## Table B — Value computation (LMF), the authors' `characters_full_*` pairs (one row, as in the paper's Table 4; the 2,000 same-letter pairs are only broken out in `RESULTS_uci.md`)

| Data set | n | Time / instance (ms) baseline | proposed | speed-up (sum / geomean) | Black-box calls / instance baseline | proposed | Construction share baseline → proposed | Paper Table 4 (baseline) | pairs off by > 10⁻⁷ |
|---|--:|--:|--:|--:|--:|--:|---|---|--:|
| Characters | 21,000 | 135.35 | 30.66 | **4.41× / 4.07×** [4.04, 4.10] | 12,246 | 3,146 | 60.1 % → 6.6 % | 140.0 ms, 12,387 calls, 52.3 % | 0 |

```latex
\begin{table}[t]
\centering
\caption{Value computation (LMF) on the 21{,}000 pairs of the \texttt{characters\_full}
benchmark of~\cite{BKN20}. One
measurement per instance, single core; speed-up is the ratio of total times.
\cite{BKN20} (Table~4) report 140.0\,ms and 12{,}387 black-box calls per instance
with 52.3\,\% of the time in arrangement construction on their machine. All 21{,}000
values agree with the baseline to within $10^{-7}$.}
\label{tab:lmf}
\begin{tabular}{lrrrrrrrr}
\toprule
& & \multicolumn{3}{c}{Time / instance (ms)} & \multicolumn{2}{c}{Black-box calls} & \multicolumn{2}{c}{Construction share} \\
\cmidrule(lr){3-5}\cmidrule(lr){6-7}\cmidrule(lr){8-9}
Data set & $n$ & baseline & proposed & speed-up & baseline & proposed & baseline & proposed \\
\midrule
Characters      & 21{,}000 & 135.35 & 30.66 & 4.41$\times$ & 12{,}246 & 3{,}146 & 60.1\,\% & 6.6\,\% \\
\bottomrule
\end{tabular}
\end{table}
```

## Table C — LMF profile in the format of the paper's Table 4 (21,000 `characters_full` instances, re-measurement)

Rows are the timers the authors' harness prints (`updateProfileValComp` in
`src/fut_paper_experiments.cpp`): Preprocessing = `FUT_PREPROCESSING2`, Black-box calls
(Lipschitz) = `FUT_BLACKBOX2`, Arrangement estimation = `FUT_DISCSELECTION2`, Arrangement
algorithm = `FUT_ARRANGEMENT2` with Construction = `FUT_N6_ARR` and Black-box calls =
`FUT_N6_FRECHET`. In the proposed arm `FUT_N6_ARR` times the maximal-set enumeration
(disc classification, components, 2^m DP, witnesses) that replaces the arrangement, and
`FUT_N6_FRECHET` the predicate evaluations at the witnesses. Sub-rows omit untimed overhead.
Exact-mode predicate statistics over the run: P2 197,226,956 evaluations, 0 rational
re-evaluations; P3 289,116,753 evaluations, 74 rational re-evaluations; overflow 0.
candidate5_noslack (same predicates without the filter) took 645,780 ms — the filter costs 0.3 %.

| Algorithm | Time | | Black-Box Calls |
|---|---|--:|--:|
| **LMF, baseline** (authors' code) | **2,842,300 ms** (135.3 ms per instance) | | **257,162,361** (12,245.8 per instance) |
| | - Preprocessing | 87,237 ms | |
| | - Black-box calls (Lipschitz) | 289,267 ms | |
| | - Arrangement estimation | 190,210 ms | |
| | - Arrangement algorithm | 2,233,926 ms | |
| | &nbsp;&nbsp;\* Construction | 1,708,959 ms | |
| | &nbsp;&nbsp;\* Black-box calls | 367,421 ms | |
| **LMF, proposed** (maximal sets, exact predicates) | **643,923 ms** (30.7 ms per instance) | | **66,067,601** (3,146.1 per instance) |
| | - Preprocessing | 82,581 ms | |
| | - Black-box calls (Lipschitz) | 282,761 ms | |
| | - Arrangement estimation | 162,130 ms | |
| | - Arrangement algorithm | 77,596 ms | |
| | &nbsp;&nbsp;\* Construction | 42,777 ms | |
| | &nbsp;&nbsp;\* Black-box calls | 33,978 ms | |
| *LMF as reported in [BKN20], Table 4 (authors' machine)* | *2,938,512 ms (140.0 ms per instance)* | | *260,128,449 (12,387.1 per instance)* |

Speed-up 4.41× (sum of times; geomean of per-instance ratios 4.07 [4.04, 4.10], faster on 20,904/21,000);
black-box calls 3.9×; Construction 40×; arrangement black-box calls 10.8×; arrangement-algorithm
share 78.6 % → 12.0 %; the three unchanged stages are 81.9 % of the proposed time. All 21,000 values
agree with the baseline to within 8.9e-9.

```latex
\begin{table}[t]
\centering
\caption{Value computation on the 21{,}000 \texttt{characters\_full} instances
of~\cite{BKN20}, in the format of their Table~4: LMF with the original arrangement
construction (baseline) and LMF with the proposed maximal-set enumeration (proposed),
measured back-to-back on the same machine, one measurement per instance. In the proposed
arm the Construction row times the enumeration of maximal sets and witnesses, and the
black-box calls below it the predicate evaluations at the witnesses. \cite{BKN20} report
2{,}938{,}512\,ms (140.0\,ms per instance) and 260{,}128{,}449 calls on their machine.}
\label{tab:lmf-profile}
\begin{tabular}{llrr}
\toprule
\textbf{Algorithm} & \multicolumn{2}{c}{\textbf{Time}} & \textbf{Black-Box Calls} \\
\midrule
LMF, baseline & \multicolumn{2}{c}{2{,}842{,}300 ms} & 257{,}162{,}361 \\
              & \multicolumn{2}{c}{(135.3 ms/inst.)} & (12{,}245.8/inst.) \\
\cmidrule(r){2-3}
& -- Preprocessing                & 87{,}237 ms \\
\cmidrule(r){2-3}
& -- Black-box calls (Lipschitz)  & 289{,}267 ms \\
\cmidrule(r){2-3}
& -- Arrangement estimation       & 190{,}210 ms \\
\cmidrule(r){2-3}
& -- Arrangement algorithm        & 2{,}233{,}926 ms \\
& \hphantom{bla} * Construction    & 1{,}708{,}959 ms \\
& \hphantom{bla} * Black-box calls & 367{,}421 ms \\
\midrule
LMF, proposed & \multicolumn{2}{c}{643{,}923 ms} & 66{,}067{,}601 \\
              & \multicolumn{2}{c}{(30.7 ms/inst.)} & (3{,}146.1/inst.) \\
\cmidrule(r){2-3}
& -- Preprocessing                & 82{,}581 ms \\
\cmidrule(r){2-3}
& -- Black-box calls (Lipschitz)  & 282{,}761 ms \\
\cmidrule(r){2-3}
& -- Arrangement estimation       & 162{,}130 ms \\
\cmidrule(r){2-3}
& -- Arrangement algorithm        & 77{,}596 ms \\
& \hphantom{bla} * Construction    & 42{,}777 ms \\
& \hphantom{bla} * Black-box calls & 33{,}978 ms \\
\bottomrule
\end{tabular}
\end{table}
```

## Table D - Sigspatial value computation, the authors' 1,000 decider pairs, in the format of the paper's Table 4 (user PC, WSL2, r3)

Not in the paper (its Table 4 is Characters only). The authors' Sigspatial decider pairs on the full 20,199-curve set; value
computation (`calcDistance2`), one measurement per instance; `run_wsl_paired.sh`: steady_clock, the arms of each pair back-to-back
on one vCPU, one process per pair and arm; proposed = candidate5 with MAXREGION_EXACT=1 MAXREGION_SLACK=0. 998 pairs measured on
both arms. On the other 2 the baseline is OOM-killed even under a 12 GB limit (at 312 s and 203 s in an
earlier run with the old clock, `sigspatial_lmf_original_oom_12gb.txt` in the archive; an approximate lower bound on its time),
and the proposed arm finishes them in 0.52 s and 1.85 s. The rows below cover the 998 pairs.

| Algorithm | Time | Black-Box Calls |
|---|--:|--:|
| **LMF, baseline** | **2,456,421 ms** (2,461.34 ms per instance) | **12,554,186** (12,579.34 per instance) |
| - Preprocessing | 18,379 ms | |
| - Black-box calls (Lipschitz) | 17,777 ms | |
| - Arrangement estimation | 29,700 ms | |
| - Arrangement algorithm | 2,387,716 ms | |
| &nbsp;&nbsp;\* Construction | 2,365,749 ms | |
| &nbsp;&nbsp;\* Black-box calls | 15,590 ms | |
| **LMF, proposed** | **65,571 ms** (65.70 ms per instance) | **3,561,796** (3,568.93 per instance) |
| - Preprocessing | 18,463 ms | |
| - Black-box calls (Lipschitz) | 17,415 ms | |
| - Arrangement estimation | 23,130 ms | |
| - Arrangement algorithm | 3,892 ms | |
| &nbsp;&nbsp;\* Maximal-set enumeration | 2,371 ms | |
| &nbsp;&nbsp;\* Black-box calls | 1,435 ms | |

Per-instance speed-up: geometric mean 2.47 [95 % bootstrap interval 2.36, 2.59], median 2.42, faster on 908/998;
values agree to within 8.6e-09; the slowest proposed instance, over all 1,000 pairs, takes 2.04 s.
Total-time ratio 37.5x, but it rests on a few instances measured once: the slowest baseline instance is
34.5 % of the baseline total and the 4 slowest are 86 %; without the slowest the ratio is 25.0x, and without
the 10 slowest the per-instance means are 150.0 vs 55.2 ms (2.72x). The same 10 slow instances took 0.75-1.27x
their r3 baseline time in the invalidated first run, whose total ratio was 32.9x. Read the total ratio as indicative only;
the per-instance statistics are stable (geometric mean 2.37 in the first run, 2.47 here).
candidate5_noslack (same predicates without the exact filter): 50.1 ms per instance, 2,841.1 calls.
Peak RSS over the 998 pairs: baseline 31 MB, proposed 29 MB.

## Table E - Decision problem in the format of the paper's Table 2: same-characters, all-characters, Sigspatial (user PC, WSL2, r3)

The authors' 23 x 1,000 decider instances per benchmark, each measured once; `run_wsl_paired.sh`: steady_clock, both arms on the
same vCPU, alternating per query file. Stage timers are the authors' `updateProfileDec` ones (`FUT_PREPROCESSING1`, `FUT_BLACKBOX1`,
`FUT_DISCSELECTION1`, `FUT_ARRANGEMENT1` with sub-timers `FUT_N6_ARR` and `FUT_N6_FRECHET`; the sub-rows omit the untimed rest of the
stage). The 4^l sets follow the paper's text (its instance files
are not shipped, so the pair sample may differ from the paper's); the 2^l sets are the authors' shipped files (their own outputs for
those are in `experiments/`, see `REPRO_check.md`). Paper rows are the authors' machine and are shown for the stage shares only.
The proposed arm is candidate5 with MAXREGION_EXACT=1 MAXREGION_SLACK=0, as in Tables C and D. Table A (container) ran the
default candidate5 (band filter, caller slack); its call counts differ from these on 3 of 92,000 Characters instances, so Tables A
and E differ in both machine and proposed configuration.

### 4^l (the paper's protocol)

#### same-characters (23 x 1,000 instances; both arms 0 wrong answers)

| Algorithm | Time | Black-Box Calls |
|---|--:|--:|
| *LMF as reported in [BKN20], Table 2 (authors' machine)* | *429,623 ms* (18.7 ms per instance) | *26,661,524* (1,159.20 per instance) |
| *- Preprocessing / Black-box calls (Lipschitz) / Arrangement estimation* | *5 / 44,312 / 157,780 ms* | |
| *- Arrangement algorithm (Construction, Black-box calls)* | *226,469 ms (148,898, 60,156)* | |
| **LMF, baseline** | **266,604 ms** (11.59 ms per instance) | **22,931,233** (997.01 per instance) |
| - Preprocessing | 15 ms | |
| - Black-box calls (Lipschitz) | 29,214 ms | |
| - Arrangement estimation | 99,242 ms | |
| - Arrangement algorithm | 137,350 ms | |
| &nbsp;&nbsp;\* Construction | 104,070 ms | |
| &nbsp;&nbsp;\* Black-box calls | 24,316 ms | |
| **LMF, proposed** | **135,597 ms** (5.90 ms per instance) | **5,676,400** (246.80 per instance) |
| - Preprocessing | 15 ms | |
| - Black-box calls (Lipschitz) | 28,542 ms | |
| - Arrangement estimation | 100,605 ms | |
| - Arrangement algorithm | 5,863 ms | |
| &nbsp;&nbsp;\* Maximal-set enumeration | 3,171 ms | |
| &nbsp;&nbsp;\* Black-box calls | 2,599 ms | |

#### all-characters (23 x 1,000 instances; both arms 0 wrong answers)

| Algorithm | Time | Black-Box Calls |
|---|--:|--:|
| *LMF as reported in [BKN20], Table 2 (authors' machine)* | *628,043 ms* (27.3 ms per instance) | *42,781,931* (1,860.08 per instance) |
| *- Preprocessing / Black-box calls (Lipschitz) / Arrangement estimation* | *5 / 50,462 / 191,177 ms* | |
| *- Arrangement algorithm (Construction, Black-box calls)* | *385,145 ms (237,043, 120,149)* | |
| **LMF, baseline** | **446,469 ms** (19.41 ms per instance) | **37,175,347** (1,616.32 per instance) |
| - Preprocessing | 20 ms | |
| - Black-box calls (Lipschitz) | 38,223 ms | |
| - Arrangement estimation | 148,961 ms | |
| - Arrangement algorithm | 258,060 ms | |
| &nbsp;&nbsp;\* Construction | 191,560 ms | |
| &nbsp;&nbsp;\* Black-box calls | 49,595 ms | |
| **LMF, proposed** | **200,385 ms** (8.71 ms per instance) | **8,875,271** (385.88 per instance) |
| - Preprocessing | 23 ms | |
| - Black-box calls (Lipschitz) | 37,603 ms | |
| - Arrangement estimation | 151,624 ms | |
| - Arrangement algorithm | 10,078 ms | |
| &nbsp;&nbsp;\* Maximal-set enumeration | 5,629 ms | |
| &nbsp;&nbsp;\* Black-box calls | 4,291 ms | |

#### sigspatial (23 x 1,000 instances; both arms 0 wrong answers)

| Algorithm | Time | Black-Box Calls |
|---|--:|--:|
| *LMF as reported in [BKN20], Table 2 (authors' machine)* | *1,207,560 ms* (52.5 ms per instance) | *31,420,517* (1,366.11 per instance) |
| *- Preprocessing / Black-box calls (Lipschitz) / Arrangement estimation* | *5 / 43,861 / 913,266 ms* | |
| *- Arrangement algorithm (Construction, Black-box calls)* | *249,268 ms (155,332, 73,934)* | |
| **LMF, baseline** | **890,144 ms** (38.70 ms per instance) | **26,366,095** (1,146.35 per instance) |
| - Preprocessing | 19 ms | |
| - Black-box calls (Lipschitz) | 34,786 ms | |
| - Arrangement estimation | 685,321 ms | |
| - Arrangement algorithm | 168,714 ms | |
| &nbsp;&nbsp;\* Construction | 124,904 ms | |
| &nbsp;&nbsp;\* Black-box calls | 31,550 ms | |
| **LMF, proposed** | **705,065 ms** (30.65 ms per instance) | **6,748,661** (293.42 per instance) |
| - Preprocessing | 19 ms | |
| - Black-box calls (Lipschitz) | 34,216 ms | |
| - Arrangement estimation | 662,544 ms | |
| - Arrangement algorithm | 7,162 ms | |
| &nbsp;&nbsp;\* Maximal-set enumeration | 4,027 ms | |
| &nbsp;&nbsp;\* Black-box calls | 2,954 ms | |

### 2^l (the authors' shipped query files)

#### same-characters (23 x 1,000 instances; both arms 0 wrong answers)

| Algorithm | Time | Black-Box Calls |
|---|--:|--:|
| **LMF, baseline** | **19,713 ms** (0.86 ms per instance) | **1,590,691** (69.16 per instance) |
| - Preprocessing | 15 ms | |
| - Black-box calls (Lipschitz) | 8,307 ms | |
| - Arrangement estimation | 8,091 ms | |
| - Arrangement algorithm | 3,135 ms | |
| &nbsp;&nbsp;\* Construction | 2,372 ms | |
| &nbsp;&nbsp;\* Black-box calls | 567 ms | |
| **LMF, proposed** | **16,447 ms** (0.72 ms per instance) | **1,229,988** (53.48 per instance) |
| - Preprocessing | 15 ms | |
| - Black-box calls (Lipschitz) | 8,128 ms | |
| - Arrangement estimation | 8,008 ms | |
| - Arrangement algorithm | 138 ms | |
| &nbsp;&nbsp;\* Maximal-set enumeration | 73 ms | |
| &nbsp;&nbsp;\* Black-box calls | 62 ms | |

#### all-characters (23 x 1,000 instances; both arms 0 wrong answers)

| Algorithm | Time | Black-Box Calls |
|---|--:|--:|
| **LMF, baseline** | **12,619 ms** (0.55 ms per instance) | **1,412,302** (61.40 per instance) |
| - Preprocessing | 18 ms | |
| - Black-box calls (Lipschitz) | 6,661 ms | |
| - Arrangement estimation | 5,009 ms | |
| - Arrangement algorithm | 761 ms | |
| &nbsp;&nbsp;\* Construction | 566 ms | |
| &nbsp;&nbsp;\* Black-box calls | 149 ms | |
| **LMF, proposed** | **11,926 ms** (0.52 ms per instance) | **1,332,718** (57.94 per instance) |
| - Preprocessing | 18 ms | |
| - Black-box calls (Lipschitz) | 6,673 ms | |
| - Arrangement estimation | 5,032 ms | |
| - Arrangement algorithm | 35 ms | |
| &nbsp;&nbsp;\* Maximal-set enumeration | 18 ms | |
| &nbsp;&nbsp;\* Black-box calls | 16 ms | |

#### sigspatial (23 x 1,000 instances; both arms 0 wrong answers)

| Algorithm | Time | Black-Box Calls |
|---|--:|--:|
| **LMF, baseline** | **6,327 ms** (0.28 ms per instance) | **557,540** (24.24 per instance) |
| - Preprocessing | 11 ms | |
| - Black-box calls (Lipschitz) | 2,311 ms | |
| - Arrangement estimation | 3,723 ms | |
| - Arrangement algorithm | 210 ms | |
| &nbsp;&nbsp;\* Construction | 181 ms | |
| &nbsp;&nbsp;\* Black-box calls | 11 ms | |
| **LMF, proposed** | **6,120 ms** (0.27 ms per instance) | **523,550** (22.76 per instance) |
| - Preprocessing | 12 ms | |
| - Black-box calls (Lipschitz) | 2,306 ms | |
| - Arrangement estimation | 3,723 ms | |
| - Arrangement algorithm | 7 ms | |
| &nbsp;&nbsp;\* Maximal-set enumeration | 6 ms | |
| &nbsp;&nbsp;\* Black-box calls | 1 ms | |

### Summary

| set | benchmark | ms/instance baseline -> proposed | speed-up | calls/instance | arrangement algorithm (ms) | its share of the baseline |
|---|---|---|--:|---|---|--:|
| 4^l | same-characters | 11.59 -> 5.90 | 1.97x | 997 -> 247 | 137,350 -> 5,863 | 51.5 % |
| 4^l | all-characters | 19.41 -> 8.71 | 2.23x | 1,616 -> 386 | 258,060 -> 10,078 | 57.8 % |
| 4^l | sigspatial | 38.70 -> 30.65 | 1.26x | 1,146 -> 293 | 168,714 -> 7,162 | 19.0 % |
| 2^l | same-characters | 0.86 -> 0.72 | 1.20x | 69 -> 53 | 3,135 -> 138 | 15.9 % |
| 2^l | all-characters | 0.55 -> 0.52 | 1.06x | 61 -> 58 | 761 -> 35 | 6.0 % |
| 2^l | sigspatial | 0.28 -> 0.27 | 1.03x | 24 -> 23 | 210 -> 7 | 3.3 % |

