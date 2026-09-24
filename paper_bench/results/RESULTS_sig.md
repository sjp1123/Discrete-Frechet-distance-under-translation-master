# Sigspatial, the full 20,199-curve set, the authors' decider instances (one measurement each; LMF r3, decider r3)

## Value computation (LMF, `calcDistance2`) on the 1,000 pairs of the authors' Sigspatial decider set

### all 1,000 pairs: 998 pairs measured on every arm

| arm | mean ms/instance | total s | bb calls/instance | construction % | arr. bb calls % | max abs diff vs original | pairs over 1e-7 (cand > orig) | vs original: sum-ratio, geomean [95% CI], median, faster |
|---|--:|--:|--:|--:|--:|--:|--:|---|
| original | 2461.34 | 2456.4 | 12,579 | 96.3 | 0.6 | — | — | — |
| candidate5_noslack | 50.05 | 50.0 | 2,841 | 3.0 | 2.0 | 1.393e-08 | 0 (0) | 49.18, 3.21 [3.01, 3.41], 2.64, 912/998 |
| candidate5_exact | 65.70 | 65.6 | 3,569 | 3.6 | 2.2 | 8.587e-09 | 0 (0) | 37.46, 2.47 [2.36, 2.59], 2.42, 908/998 |

The sum ratio is tail-driven: the 4 slowest baseline instances are 86 % of the baseline total; read the geometric mean and median (see `TABLES_uci.md` Table D).

## Decision problem, Sigspatial, the authors' query files (factors 1 ± 2^l)

candidate5 = candidate5 with MAXREGION_EXACT=1 MAXREGION_SLACK=0 (run_wsl_paired.sh, r3); in RESULTS_uci.md it is the default configuration.

| set | expected | n | original ms/instance | candidate5 ms/instance | ratio | bb calls orig | bb calls c5 | wrong (orig / c5) | arms disagree |
|---|---|--:|--:|--:|--:|--:|--:|--:|--:|
| l=-10 plus | yes | 1000 | 0.7409 | 0.7232 | 1.024 | 68.1 | 67.6 | 0 / 0 | 0 |
| l=-9 plus | yes | 1000 | 0.4290 | 0.4371 | 0.981 | 48.3 | 48.1 | 0 / 0 | 0 |
| l=-8 plus | yes | 1000 | 0.2026 | 0.2013 | 1.006 | 29.8 | 29.8 | 0 / 0 | 0 |
| l=-7 plus | yes | 1000 | 0.1138 | 0.1132 | 1.005 | 18.5 | 18.5 | 0 / 0 | 0 |
| l=-6 plus | yes | 1000 | 0.0612 | 0.0622 | 0.984 | 10.7 | 10.7 | 0 / 0 | 0 |
| l=-5 plus | yes | 1000 | 0.0360 | 0.0348 | 1.036 | 6.2 | 6.2 | 0 / 0 | 0 |
| l=-4 plus | yes | 1000 | 0.0223 | 0.0224 | 0.997 | 3.9 | 3.9 | 0 / 0 | 0 |
| l=-3 plus | yes | 1000 | 0.0140 | 0.0131 | 1.065 | 2.7 | 2.7 | 0 / 0 | 0 |
| l=-2 plus | yes | 1000 | 0.0087 | 0.0114 | 0.762 | 2.2 | 2.2 | 0 / 0 | 0 |
| l=-1 plus | yes | 1000 | 0.0078 | 0.0082 | 0.952 | 2.0 | 2.0 | 0 / 0 | 0 |
| l=0 plus | yes | 1000 | 0.0047 | 0.0050 | 0.952 | 2.0 | 2.0 | 0 / 0 | 0 |
| l=1 plus | yes | 1000 | 0.0042 | 0.0043 | 0.976 | 2.0 | 2.0 | 0 / 0 | 0 |
| l=2 plus | yes | 1000 | 0.0034 | 0.0037 | 0.916 | 2.0 | 2.0 | 0 / 0 | 0 |
| l=-10 minus | no | 1000 | 2.8600 | 2.7021 | 1.058 | 169.1 | 142.8 | 0 / 0 | 0 |
| l=-9 minus | no | 1000 | 1.1182 | 1.0757 | 1.039 | 93.7 | 87.4 | 0 / 0 | 0 |
| l=-8 minus | no | 1000 | 0.4052 | 0.4041 | 1.003 | 50.0 | 49.3 | 0 / 0 | 0 |
| l=-7 minus | no | 1000 | 0.1646 | 0.1668 | 0.987 | 26.1 | 26.1 | 0 / 0 | 0 |
| l=-6 minus | no | 1000 | 0.0777 | 0.0774 | 1.004 | 12.6 | 12.6 | 0 / 0 | 0 |
| l=-5 minus | no | 1000 | 0.0331 | 0.0341 | 0.970 | 5.3 | 5.3 | 0 / 0 | 0 |
| l=-4 minus | no | 1000 | 0.0127 | 0.0131 | 0.967 | 1.9 | 1.9 | 0 / 0 | 0 |
| l=-3 minus | no | 1000 | 0.0044 | 0.0040 | 1.086 | 0.5 | 0.5 | 0 / 0 | 0 |
| l=-2 minus | no | 1000 | 0.0014 | 0.0018 | 0.770 | 0.0 | 0.0 | 0 / 0 | 0 |
| l=-1 minus | no | 1000 | 0.0007 | 0.0008 | 0.859 | 0.0 | 0.0 | 0 / 0 | 0 |
| **all sets** | | 23000 | **0.2751** | **0.2661** | **1.034** | 24.2 | 22.8 | **0 / 0** | **0** |

Totals over 23000 instances: original 6.33 s, candidate5 6.12 s.

## Decision problem, Sigspatial, same pairs, factors (1 ± 4^l) as in the paper text

candidate5 = candidate5 with MAXREGION_EXACT=1 MAXREGION_SLACK=0 (run_wsl_paired.sh, r3); in RESULTS_uci.md it is the default configuration.

| set | expected | n | original ms/instance | candidate5 ms/instance | ratio | bb calls orig | bb calls c5 | wrong (orig / c5) | arms disagree |
|---|---|--:|--:|--:|--:|--:|--:|--:|--:|
| l=-10 plus | yes | 1000 | 63.9027 | 65.1073 | 0.981 | 647.1 | 591.5 | 0 / 0 | 0 |
| l=-9 plus | yes | 1000 | 50.2139 | 45.8009 | 1.096 | 567.2 | 525.5 | 0 / 0 | 0 |
| l=-8 plus | yes | 1000 | 23.1865 | 23.4464 | 0.989 | 412.2 | 385.8 | 0 / 0 | 0 |
| l=-7 plus | yes | 1000 | 9.1355 | 9.0013 | 1.015 | 245.2 | 235.7 | 0 / 0 | 0 |
| l=-6 plus | yes | 1000 | 3.5447 | 3.4852 | 1.017 | 139.5 | 136.7 | 0 / 0 | 0 |
| l=-5 plus | yes | 1000 | 0.9367 | 0.8443 | 1.109 | 68.1 | 67.6 | 0 / 0 | 0 |
| l=-4 plus | yes | 1000 | 0.2411 | 0.2515 | 0.959 | 29.8 | 29.8 | 0 / 0 | 0 |
| l=-3 plus | yes | 1000 | 0.0691 | 0.0699 | 0.989 | 10.7 | 10.7 | 0 / 0 | 0 |
| l=-2 plus | yes | 1000 | 0.0265 | 0.0252 | 1.051 | 3.9 | 3.9 | 0 / 0 | 0 |
| l=-1 plus | yes | 1000 | 0.0112 | 0.0114 | 0.986 | 2.2 | 2.2 | 0 / 0 | 0 |
| l=0 plus | yes | 1000 | 0.0069 | 0.0069 | 0.989 | 2.0 | 2.0 | 0 / 0 | 0 |
| l=1 plus | yes | 1000 | 0.0053 | 0.0052 | 1.029 | 2.0 | 2.0 | 0 / 0 | 0 |
| l=2 plus | yes | 1000 | 0.0049 | 0.0049 | 0.985 | 2.0 | 2.0 | 0 / 0 | 0 |
| l=-10 minus | no | 1000 | 280.5552 | 193.5761 | 1.449 | 7885.4 | 1338.4 | 0 / 0 | 0 |
| l=-9 minus | no | 1000 | 212.5108 | 168.7214 | 1.260 | 7292.0 | 1194.2 | 0 / 0 | 0 |
| l=-8 minus | no | 1000 | 156.5436 | 120.8737 | 1.295 | 5613.1 | 1005.3 | 0 / 0 | 0 |
| l=-7 minus | no | 1000 | 68.8731 | 55.2570 | 1.246 | 2541.4 | 668.8 | 0 / 0 | 0 |
| l=-6 minus | no | 1000 | 16.8568 | 15.0686 | 1.119 | 668.7 | 340.0 | 0 / 0 | 0 |
| l=-5 minus | no | 1000 | 2.9848 | 2.9876 | 0.999 | 169.1 | 142.8 | 0 / 0 | 0 |
| l=-4 minus | no | 1000 | 0.4389 | 0.4219 | 1.040 | 50.0 | 49.3 | 0 / 0 | 0 |
| l=-3 minus | no | 1000 | 0.0812 | 0.0828 | 0.980 | 12.6 | 12.6 | 0 / 0 | 0 |
| l=-2 minus | no | 1000 | 0.0137 | 0.0141 | 0.972 | 1.9 | 1.9 | 0 / 0 | 0 |
| l=-1 minus | no | 1000 | 0.0011 | 0.0013 | 0.869 | 0.0 | 0.0 | 0 / 0 | 0 |
| **all sets** | | 23000 | **38.7019** | **30.6550** | **1.262** | 1146.4 | 293.4 | **0 / 0** | **0** |

Totals over 23000 instances: original 890.14 s, candidate5 705.06 s.

