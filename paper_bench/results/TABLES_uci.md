# Paper tables — Characters, the authors' data and instances (UCI file)

Source: `RESULTS_uci_r2.md` for LMF (raw: `raw_characters_uci_lmf_r2.tar.gz`; original, candidate5_noslack and
candidate5_exact measured back-to-back on one idle core of the same container instance) and
`RESULTS_uci.md` for the decider (raw: `raw_characters_uci_decider.tar.gz`, earlier container instance —
absolute times are not comparable across the two instances). One measurement per instance.
baseline = original (CGAL arrangement); proposed = candidate5 with MAXREGION_EXACT=1 and MAXREGION_SLACK=0
(filtered P2/P3 predicates with rational fallback) for LMF, candidate5 for the decider. Paper columns from Bringmann–Künnemann–Nusser, ESA 2020.

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

## Table D — Sigspatial, the full 20,199-curve set, the authors' 1,000 decider pairs (user PC, WSL2; see experiment_log §6.9)

Value computation (LMF); proposed = candidate5 with MAXREGION_EXACT=1 MAXREGION_SLACK=0; 998 pairs measured on both arms
(on 2 pairs the baseline was killed at 7.4 GB under the default 7.5 GB WSL limit and again at 11.8 GB under a 12 GB limit, i.e. it needs more than 12 GB; the proposed arm finished them in 0.6 s and 2.0 s with values matching the authors' delta*).

| Algorithm | Time | Black-Box Calls |
|---|--:|--:|
| **LMF, baseline** | **2,318,393 ms** (2,323.0 ms per instance) | **12,554,186** (12,579.3 per instance) |
| - Preprocessing | 20,176 ms | |
| - Black-box calls (Lipschitz) | 16,289 ms | |
| - Arrangement estimation | 27,722 ms | |
| - Arrangement algorithm | 2,251,388 ms | |
| &nbsp;&nbsp;\* Construction | 2,230,595 ms | |
| &nbsp;&nbsp;\* Black-box calls | 15,283 ms | |
| **LMF, proposed** | **70,549 ms** (70.7 ms per instance) | **3,561,796** (3,568.9 per instance) |
| - Preprocessing | 19,164 ms | |
| - Black-box calls (Lipschitz) | 18,273 ms | |
| - Arrangement estimation | 25,881 ms | |
| - Arrangement algorithm | 4,234 ms | |
| &nbsp;&nbsp;\* Maximal-set enumeration | 2,502 ms | |
| &nbsp;&nbsp;\* Black-box calls | 1,622 ms | |

Speed-up 32.9x (sum), geomean of per-instance ratios 2.37 [2.26, 2.49], median 2.27, faster on 893/998; all 998 values agree
to within 8.6e-9 and all arms reproduce the authors' delta* to within 1.33e-8. The sum is dominated by a few instances on which
the baseline's arrangement construction takes 12-795 s (5 instances above 60 s); excluding the 10 slowest baseline instances the
per-instance times are 146.4 vs 60.4 ms (2.42x). Decider (23 x 1,000 instances, both arms 0 wrong, 0 disagreements):
authors' 2^l files 0.369 vs 0.393 ms (0.94x), 4^l sets 41.74 vs 34.51 ms (1.21x), calls 1,146 -> 293.
