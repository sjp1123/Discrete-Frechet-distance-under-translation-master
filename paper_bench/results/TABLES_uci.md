# Paper tables — Characters, the authors' data and instances (UCI file)

Source: `RESULTS_uci.md` (raw: `raw_characters_uci_{lmf,decider}.tar.gz`). One measurement per
instance, single core. baseline = original (CGAL arrangement), proposed = candidate5_noslack
(LMF) / candidate5 (decider). Paper columns from Bringmann–Künnemann–Nusser, ESA 2020.

## Table A — Decision problem, (1 ± 4^ℓ)·δ instances, 1,000 pairs × 23 sets

| Data set | Time / instance (ms) baseline | proposed | speed-up | Black-box calls / instance baseline | proposed | Paper Table 2 (baseline) | wrong / disagree |
|---|--:|--:|--:|--:|--:|---|--:|
| all-characters | 16.77 | 7.46 | **2.25×** | 1,616 | 386 | 27.3 ms, 1,860 calls | 0 / 0 |
| same-characters | 11.74 | 6.35 | **1.85×** | 997 | 247 | 18.7 ms, 1,159 calls | 0 / 0 |

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
| Characters | 21,000 | 100.60 | 24.06 | **4.18× / 3.84×** [3.81, 3.87] | 12,246 | 3,144 | 59.5 % → 6.7 % | 140.0 ms, 12,387 calls, 52.3 % | 0 |

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
Characters      & 21{,}000 & 100.60 & 24.06 & 4.18$\times$ & 12{,}246 & 3{,}144 & 59.5\,\% & 6.7\,\% \\
\bottomrule
\end{tabular}
\end{table}
```

## Table C — LMF profile in the format of the paper's Table 4 (21,000 `characters_full` instances)

Rows are the same timers the authors' harness prints (`updateProfileValComp` in
`src/fut_paper_experiments.cpp`): Preprocessing = `FUT_PREPROCESSING2`, Black-box calls
(Lipschitz) = `FUT_BLACKBOX2`, Arrangement estimation = `FUT_DISCSELECTION2`, Arrangement
algorithm = `FUT_ARRANGEMENT2` with Construction = `FUT_N6_ARR` and Black-box calls =
`FUT_N6_FRECHET`; the same columns of `raw_characters_uci_lmf.tar.gz`. Sub-rows do not add up
to the total exactly (untimed overhead), as in the paper.

| Algorithm | Time | | Black-Box Calls |
|---|---|--:|--:|
| **LMF, baseline** (authors' code, this machine) | **2,112,604 ms** (100.6 ms per instance) | | **257,162,361** (12,245.8 per instance) |
| | - Preprocessing | 68,013 ms | |
| | - Black-box calls (Lipschitz) | 218,199 ms | |
| | - Arrangement estimation | 143,772 ms | |
| | - Arrangement algorithm | 1,651,253 ms | |
| | &nbsp;&nbsp;\* Construction | 1,257,343 ms | |
| | &nbsp;&nbsp;\* Black-box calls | 272,961 ms | |
| **LMF, proposed** (maximal Čech regions, no slack) | **505,247 ms** (24.1 ms per instance) | | **66,015,748** (3,143.6 per instance) |
| | - Preprocessing | 66,974 ms | |
| | - Black-box calls (Lipschitz) | 221,416 ms | |
| | - Arrangement estimation | 125,753 ms | |
| | - Arrangement algorithm | 60,854 ms | |
| | &nbsp;&nbsp;\* Construction | 33,968 ms | |
| | &nbsp;&nbsp;\* Black-box calls | 26,239 ms | |
| *LMF as reported in [BKN20], Table 4 (authors' machine)* | *2,938,512 ms (140.0 ms per instance)* | | *260,128,449 (12,387.1 per instance)* |
| | - Preprocessing | 71,728 ms | |
| | - Black-box calls (Lipschitz) | 400,189 ms | |
| | - Arrangement estimation | 166,479 ms | |
| | - Arrangement algorithm | 2,250,493 ms | |
| | &nbsp;&nbsp;\* Construction | 1,537,500 ms | |
| | &nbsp;&nbsp;\* Black-box calls | 545,442 ms | |

```latex
\begin{table}[t]
\centering
\caption{Value computation on the 21{,}000 \texttt{characters\_full} instances
of~\cite{BKN20}, in the format of their Table~4: LMF with the original arrangement
construction (baseline) and LMF with the proposed maximal-region construction, both
measured here on the same machine, one measurement per instance. The rows are the timers
of the authors' harness; sub-rows omit untimed overhead. For reference, \cite{BKN20}
report 2{,}938{,}512\,ms (140.0\,ms per instance) and 260{,}128{,}449 black-box calls
on their machine.}
\label{tab:lmf-profile}
\begin{tabular}{llrr}
\toprule
\textbf{Algorithm} & \multicolumn{2}{c}{\textbf{Time}} & \textbf{Black-Box Calls} \\
\midrule
LMF (baseline) & \multicolumn{2}{c}{2{,}112{,}604 ms} & 257{,}162{,}361 \\
               & \multicolumn{2}{c}{(100.6 ms per instance)} & (12{,}245.8 per instance) \\
\cmidrule(r){2-3}
& - Preprocessing                & 68{,}013 ms \\
\cmidrule(r){2-3}
& - Black-box calls (Lipschitz)  & 218{,}199 ms \\
\cmidrule(r){2-3}
& - Arrangement estimation       & 143{,}772 ms \\
\cmidrule(r){2-3}
& - Arrangement algorithm        & 1{,}651{,}253 ms \\
& \hphantom{bla} * Construction    & 1{,}257{,}343 ms \\
& \hphantom{bla} * Black-box calls & 272{,}961 ms \\
\midrule
LMF (proposed) & \multicolumn{2}{c}{505{,}247 ms} & 66{,}015{,}748 \\
               & \multicolumn{2}{c}{(24.1 ms per instance)} & (3{,}143.6 per instance) \\
\cmidrule(r){2-3}
& - Preprocessing                & 66{,}974 ms \\
\cmidrule(r){2-3}
& - Black-box calls (Lipschitz)  & 221{,}416 ms \\
\cmidrule(r){2-3}
& - Arrangement estimation       & 125{,}753 ms \\
\cmidrule(r){2-3}
& - Arrangement algorithm        & 60{,}854 ms \\
& \hphantom{bla} * Construction    & 33{,}968 ms \\
& \hphantom{bla} * Black-box calls & 26{,}239 ms \\
\bottomrule
\end{tabular}
\end{table}
```
