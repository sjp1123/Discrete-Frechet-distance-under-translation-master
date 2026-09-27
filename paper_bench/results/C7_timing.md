# candidate7: correctness on the paper's data and paired timings against original

Machine: this PC (Intel Core Ultra 5 125H, WSL2 Ubuntu 22.04, g++ 11.4, CGAL 5.4, Boost 1.74), paper_bench built from the current sources of each arm (`RelWithDebInfo`, same flags). Every job (a chunk of 100 Characters pairs, one Sigspatial pair, or one decider query file) ran the three arms back-to-back on the same pinned vCPU, in an order rotated per job; 8 such streams ran in parallel (vCPUs 1,3,…,15). Clock: `steady_clock`. candidate7 and candidate5 run with `MAXREGION=cech MAXREGION_EXACT=1 MAXREGION_SLACK=0` (candidate7's defaults); original has no knobs. Raw data: `results/raw_c7_timing.tar.gz`.

Process exits: 3,835 with rc=0; 0 non-zero; 2 skipped (the original on the 2 Sigspatial pairs that needed > 12 GB in r3).

## 1. Correctness

### Decider (paperq4 files: YES expected at (1 + 4^l)·δ*, NO at (1 − 4^l)·δ*)

| data set | queries per arm | wrong: original | wrong: candidate7 | wrong: candidate5 | candidate7 ≠ original |
|---|--:|--:|--:|--:|--:|
| all-characters | 23,000 | 0 | 0 | 0 | 0 |
| same-characters | 23,000 | 0 | 0 | 0 | 0 |
| Sigspatial | 23,000 | 0 | 0 | 0 | 0 |

### Value computation (LMF, `calcDistance2`)

| data set | pairs | max \|candidate7 − original\| | pairs over 1e-7 | max \|candidate7 − candidate5\| | pairs over 1e-7 |
|---|--:|--:|--:|--:|--:|
| Characters (21,000) | 21,000 (vs original 21,000) | 9.27e-09 | 0 | 4.78e-09 | 0 |
| Sigspatial (1,000) | 1,000 (vs original 998) | 8.46e-09 | 0 | 4.52e-09 | 0 |

## 2. Value computation (LMF), in the paper's Table 4 format

### Characters, the authors' 21,000 pairs

Times summed over the 21,000 pairs measured on all three arms.

| | original (baseline) | candidate7 (proposed) | candidate5 (abstract) |
|---|--:|--:|--:|
| Time | 3,618,349 ms (172.3 ms/inst.) | 975,613 ms (46.5 ms/inst.) | 887,289 ms (42.3 ms/inst.) |
| Black-box calls | 257,162,361 (12,245.8/inst.) | 68,690,225 (3,271.0/inst.) | 66,067,601 (3,146.1/inst.) |
| – Preprocessing | 123,022 ms | 127,124 ms | 127,451 ms |
| – Black-box calls (Lipschitz) | 377,642 ms | 400,178 ms | 376,568 ms |
| – Arrangement estimation | 245,753 ms | 220,930 ms | 220,636 ms |
| – Arrangement algorithm / base case | 2,818,084 ms | 172,260 ms | 110,407 ms |
|   * Construction / maximal-set enumeration | 2,149,403 ms | 105,880 ms | 63,780 ms |
|   * Black-box calls | 463,079 ms | 64,849 ms | 45,308 ms |

| comparison | pairs | baseline total | proposed total | total ratio | geometric mean [95% CI] | median | proposed faster |
|---|--:|--:|--:|--:|--:|--:|--:|
| original / candidate7 | 21,000 | 3,618.3 s | 975.6 s | **3.71×** | **3.56×** [3.53, 3.60] | 3.85× | 95.0 % |
| original / candidate5 | 21,000 | 3,618.3 s | 887.3 s | **4.08×** | **3.76×** [3.73, 3.78] | 4.22× | 95.5 % |
| candidate5 / candidate7 (cost of the fixes, <1 = slower) | 21,000 | 887.3 s | 975.6 s | **0.91×** | **0.95×** [0.94, 0.95] | 0.93× | 38.3 % |

### Sigspatial, the authors' 1,000 decider pairs

Times summed over the 998 pairs measured on all three arms (pairs measured on every arm: 998 of 1,000).

| | original (baseline) | candidate7 (proposed) | candidate5 (abstract) |
|---|--:|--:|--:|
| Time | 3,860,606 ms (3,868.3 ms/inst.) | 86,229 ms (86.4 ms/inst.) | 84,361 ms (84.5 ms/inst.) |
| Black-box calls | 12,554,186 (12,579.3/inst.) | 3,616,226 (3,623.5/inst.) | 3,561,796 (3,568.9/inst.) |
| – Preprocessing | 22,947 ms | 22,867 ms | 22,884 ms |
| – Black-box calls (Lipschitz) | 23,277 ms | 23,254 ms | 22,783 ms |
| – Arrangement estimation | 39,440 ms | 28,800 ms | 30,205 ms |
| – Arrangement algorithm / base case | 3,771,240 ms | 7,861 ms | 4,974 ms |
|   * Construction / maximal-set enumeration | 3,743,297 ms | 4,623 ms | 2,983 ms |
|   * Black-box calls | 19,641 ms | 3,105 ms | 1,870 ms |

| comparison | pairs | baseline total | proposed total | total ratio | geometric mean [95% CI] | median | proposed faster |
|---|--:|--:|--:|--:|--:|--:|--:|
| original / candidate7 | 998 | 3,860.6 s | 86.2 s | **44.77×** | **2.52×** [2.40, 2.66] | 2.34× | 91.8 % |
| original / candidate5 | 998 | 3,860.6 s | 84.4 s | **45.76×** | **2.47×** [2.36, 2.59] | 2.42× | 91.7 % |
| candidate5 / candidate7 (cost of the fixes, <1 = slower) | 998 | 84.4 s | 86.2 s | **0.98×** | **1.02×** [1.00, 1.04] | 0.96× | 39.4 % |

Pairs without an original time (skipped: > 12 GB in r3): file-003586.dat/file-002157.dat candidate7 0.66 s, value 5110.942581; file-002502.dat/file-016674.dat candidate7 2.67 s, value 3886.460162.

The total ratio is dominated by a few pairs: the 4 slowest original pairs are 88 % of the original total. Read the geometric mean and the median for the typical pair.

Peak RSS per Sigspatial LMF process (MB): original median 9, max 31; candidate7 median 8, max 29; candidate5 median 8, max 29.

## 3. Decision problem, in the paper's Table 2 format (23 paperq4 files × 1,000 queries per data set)

### same-characters

| | original (baseline) | candidate7 (proposed) | candidate5 (abstract) |
|---|--:|--:|--:|
| Time | 491,408 ms (21.37 ms/inst.) | 263,244 ms (11.45 ms/inst.) | 252,012 ms (10.96 ms/inst.) |
| Black-box calls | 22,931,233 (997.0/inst.) | 6,272,563 (272.7/inst.) | 5,676,400 (246.8/inst.) |
| – Preprocessing | 24 ms | 23 ms | 27 ms |
| – Black-box calls (Lipschitz) | 50,371 ms | 52,210 ms | 49,396 ms |
| – Arrangement estimation | 186,790 ms | 189,021 ms | 190,529 ms |
| – Arrangement algorithm / base case | 252,777 ms | 20,884 ms | 10,979 ms |
|   * Construction / maximal-set enumeration | 190,476 ms | 12,481 ms | 5,968 ms |
|   * Black-box calls | 43,715 ms | 8,166 ms | 4,801 ms |

| comparison | queries | baseline total | proposed total | total ratio | geometric mean [95% CI] | median | proposed faster |
|---|--:|--:|--:|--:|--:|--:|--:|
| original / candidate7 | 23,000 | 491.4 s | 263.2 s | **1.87×** | **1.17×** [1.16, 1.18] | 1.08× | 58.6 % |
| original / candidate5 | 23,000 | 491.4 s | 252.0 s | **1.95×** | **1.21×** [1.20, 1.22] | 1.10× | 60.8 % |
| candidate5 / candidate7 (cost of the fixes, <1 = slower) | 23,000 | 252.0 s | 263.2 s | **0.96×** | **0.97×** [0.97, 0.97] | 0.97× | 45.3 % |

### all-characters

| | original (baseline) | candidate7 (proposed) | candidate5 (abstract) |
|---|--:|--:|--:|
| Time | 669,240 ms (29.10 ms/inst.) | 321,160 ms (13.96 ms/inst.) | 300,728 ms (13.08 ms/inst.) |
| Black-box calls | 37,175,347 (1,616.3/inst.) | 9,734,920 (423.3/inst.) | 8,875,271 (385.9/inst.) |
| – Preprocessing | 25 ms | 23 ms | 26 ms |
| – Black-box calls (Lipschitz) | 53,758 ms | 56,906 ms | 52,869 ms |
| – Arrangement estimation | 226,937 ms | 232,130 ms | 230,812 ms |
| – Arrangement algorithm / base case | 386,781 ms | 30,555 ms | 15,505 ms |
|   * Construction / maximal-set enumeration | 286,985 ms | 18,691 ms | 8,827 ms |
|   * Black-box calls | 72,110 ms | 11,535 ms | 6,405 ms |

| comparison | queries | baseline total | proposed total | total ratio | geometric mean [95% CI] | median | proposed faster |
|---|--:|--:|--:|--:|--:|--:|--:|
| original / candidate7 | 23,000 | 669.2 s | 321.2 s | **2.08×** | **1.13×** [1.13, 1.14] | 1.06× | 56.0 % |
| original / candidate5 | 23,000 | 669.2 s | 300.7 s | **2.23×** | **1.16×** [1.16, 1.17] | 1.08× | 58.5 % |
| candidate5 / candidate7 (cost of the fixes, <1 = slower) | 23,000 | 300.7 s | 321.2 s | **0.94×** | **0.97×** [0.97, 0.98] | 0.97× | 45.4 % |

### Sigspatial

| | original (baseline) | candidate7 (proposed) | candidate5 (abstract) |
|---|--:|--:|--:|
| Time | 1,363,217 ms (59.27 ms/inst.) | 1,147,752 ms (49.90 ms/inst.) | 1,133,719 ms (49.29 ms/inst.) |
| Black-box calls | 26,366,095 (1,146.4/inst.) | 7,339,507 (319.1/inst.) | 6,748,661 (293.4/inst.) |
| – Preprocessing | 24 ms | 25 ms | 25 ms |
| – Black-box calls (Lipschitz) | 50,940 ms | 53,397 ms | 50,462 ms |
| – Arrangement estimation | 1,065,005 ms | 1,071,095 ms | 1,070,657 ms |
| – Arrangement algorithm / base case | 245,340 ms | 21,429 ms | 10,817 ms |
|   * Construction / maximal-set enumeration | 181,096 ms | 13,143 ms | 6,180 ms |
|   * Black-box calls | 45,243 ms | 7,954 ms | 4,346 ms |

| comparison | queries | baseline total | proposed total | total ratio | geometric mean [95% CI] | median | proposed faster |
|---|--:|--:|--:|--:|--:|--:|--:|
| original / candidate7 | 23,000 | 1,363.2 s | 1,147.8 s | **1.19×** | **1.01×** [1.01, 1.02] | 0.99× | 49.1 % |
| original / candidate5 | 23,000 | 1,363.2 s | 1,133.7 s | **1.20×** | **1.03×** [1.02, 1.04] | 1.01× | 51.0 % |
| candidate5 / candidate7 (cost of the fixes, <1 = slower) | 23,000 | 1,133.7 s | 1,147.8 s | **0.99×** | **0.98×** [0.98, 0.99] | 0.98× | 47.4 % |

## 4. Per-level decider means (ms/query), as in the paper's Figure 4

| level | same orig. | same c7 | all orig. | all c7 | Sigspatial orig. | Sigspatial c7 |
|---|--:|--:|--:|--:|--:|--:|
| 1 − 4^-1 | 0.01 | 0.01 | 0.01 | 0.01 | 0.00 | 0.00 |
| 1 − 4^-2 | 0.16 | 0.18 | 0.11 | 0.11 | 0.02 | 0.02 |
| 1 − 4^-3 | 0.72 | 0.74 | 0.38 | 0.44 | 0.09 | 0.11 |
| 1 − 4^-4 | 2.53 | 2.69 | 1.74 | 1.84 | 0.74 | 0.76 |
| 1 − 4^-5 | 14.21 | 10.09 | 7.81 | 6.84 | 5.19 | 4.84 |
| 1 − 4^-6 | 50.54 | 26.45 | 51.27 | 27.60 | 27.39 | 24.63 |
| 1 − 4^-7 | 76.16 | 33.43 | 108.56 | 46.85 | 116.96 | 95.04 |
| 1 − 4^-8 | 91.00 | 38.33 | 135.72 | 54.23 | 250.16 | 195.07 |
| 1 − 4^-9 | 94.33 | 39.82 | 147.86 | 57.90 | 335.87 | 271.29 |
| 1 − 4^-10 | 84.56 | 38.60 | 144.21 | 57.40 | 403.90 | 327.70 |
| 1 + 4^-10 | 17.69 | 16.36 | 20.60 | 19.61 | 94.19 | 99.70 |
| 1 + 4^-9 | 16.55 | 15.92 | 19.89 | 17.97 | 72.61 | 72.04 |
| 1 + 4^-8 | 16.58 | 15.62 | 15.66 | 15.28 | 35.70 | 35.84 |
| 1 + 4^-7 | 13.78 | 12.90 | 9.35 | 8.69 | 14.08 | 13.99 |
| 1 + 4^-6 | 8.03 | 7.68 | 3.52 | 3.66 | 4.82 | 5.26 |
| 1 + 4^-5 | 2.80 | 2.65 | 1.60 | 1.71 | 1.04 | 1.00 |
| 1 + 4^-4 | 1.01 | 0.98 | 0.58 | 0.59 | 0.34 | 0.33 |
| 1 + 4^-3 | 0.48 | 0.51 | 0.28 | 0.32 | 0.07 | 0.07 |
| 1 + 4^-2 | 0.16 | 0.15 | 0.08 | 0.08 | 0.02 | 0.03 |
| 1 + 4^-1 | 0.05 | 0.05 | 0.02 | 0.02 | 0.01 | 0.01 |
| 1 + 4^0 | 0.04 | 0.04 | 0.01 | 0.01 | 0.01 | 0.01 |
| 1 + 4^1 | 0.02 | 0.02 | 0.00 | 0.00 | 0.00 | 0.00 |
| 1 + 4^2 | 0.00 | 0.00 | 0.00 | 0.00 | 0.01 | 0.01 |

