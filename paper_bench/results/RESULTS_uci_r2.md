# Characters on the original UCI file, the authors' instances (one measurement each; LMF repetition r2)

## Value computation (LMF, `calcDistance2`), the authors' `characters_full_*` pairs

Paper Table 4 (all 21,000 instances, authors' machine): 140.0 ms, 12,387 black-box calls per instance, construction 52.3 % of time.

### all pairs (210 letter pairs): 21000 pairs measured on every arm

| arm | mean ms/instance | total s | bb calls/instance | construction % | arr. bb calls % | max abs diff vs original | pairs over 1e-7 (cand > orig) | vs original: sum-ratio, geomean [95% CI], median, faster |
|---|--:|--:|--:|--:|--:|--:|--:|---|
| original | 135.35 | 2842.3 | 12,246 | 60.1 | 12.9 | — | — | — |
| candidate5_noslack | 30.75 | 645.8 | 3,144 | 6.5 | 5.3 | 8.732e-09 | 0 (0) | 4.40, 4.07 [4.04, 4.10], 4.60, 20853/21000 |
| candidate5_exact | 30.66 | 643.9 | 3,146 | 6.6 | 5.3 | 8.844e-09 | 0 (0) | 4.41, 4.07 [4.04, 4.10], 4.61, 20904/21000 |

### same-letter pairs (20 files): 2000 pairs measured on every arm

| arm | mean ms/instance | total s | bb calls/instance | construction % | arr. bb calls % | max abs diff vs original | pairs over 1e-7 (cand > orig) | vs original: sum-ratio, geomean [95% CI], median, faster |
|---|--:|--:|--:|--:|--:|--:|--:|---|
| original | 104.50 | 209.0 | 8,728 | 60.9 | 13.0 | — | — | — |
| candidate5_noslack | 23.62 | 47.2 | 2,020 | 6.8 | 7.1 | 8.252e-09 | 0 (0) | 4.42, 4.24 [4.16, 4.33], 4.49, 1994/2000 |
| candidate5_exact | 23.62 | 47.2 | 2,020 | 6.9 | 7.1 | 8.252e-09 | 0 (0) | 4.42, 4.24 [4.16, 4.33], 4.47, 1997/2000 |

## Decision problem, all-characters, the authors' query files (factors 1 ± 2^l)

Paper Table 2 (all-characters, authors' machine): 27.3 ms and 1,860 black-box calls per instance.

| set | expected | n | original ms/instance | candidate5 ms/instance | ratio | bb calls orig | bb calls c5 | wrong (orig / c5) | arms disagree |
|---|---|--:|--:|--:|--:|--:|--:|--:|--:|
| l=-10 plus | yes | 1000 | 0.8536 | 0.8813 | 0.969 | 130.9 | 130.6 | 0 / 0 | 0 |
| l=-9 plus | yes | 1000 | 0.7537 | 0.6812 | 1.106 | 99.4 | 99.4 | 0 / 0 | 0 |
| l=-8 plus | yes | 1000 | 0.3920 | 0.4165 | 0.941 | 69.2 | 69.2 | 0 / 0 | 0 |
| l=-7 plus | yes | 1000 | 0.2801 | 0.2417 | 1.159 | 45.9 | 45.9 | 0 / 0 | 0 |
| l=-6 plus | yes | 1000 | 0.2008 | 0.1510 | 1.330 | 31.2 | 31.2 | 0 / 0 | 0 |
| l=-5 plus | yes | 1000 | 0.1144 | 0.0837 | 1.366 | 17.4 | 17.4 | 0 / 0 | 0 |
| l=-4 plus | yes | 1000 | 0.0563 | 0.0435 | 1.294 | 9.6 | 9.6 | 0 / 0 | 0 |
| l=-3 plus | yes | 1000 | 0.0281 | 0.0221 | 1.276 | 5.0 | 5.0 | 0 / 0 | 0 |
| l=-2 plus | yes | 1000 | 0.0153 | 0.0116 | 1.322 | 3.0 | 3.0 | 0 / 0 | 0 |
| l=-1 plus | yes | 1000 | 0.0084 | 0.0063 | 1.344 | 2.2 | 2.2 | 0 / 0 | 0 |
| l=0 plus | yes | 1000 | 0.0047 | 0.0045 | 1.045 | 2.0 | 2.0 | 0 / 0 | 0 |
| l=1 plus | yes | 1000 | 0.0027 | 0.0024 | 1.099 | 2.0 | 2.0 | 0 / 0 | 0 |
| l=2 plus | yes | 1000 | 0.0016 | 0.0012 | 1.307 | 2.0 | 2.0 | 0 / 0 | 0 |
| l=-10 minus | no | 1000 | 4.4883 | 3.7944 | 1.183 | 420.1 | 350.1 | 0 / 0 | 0 |
| l=-9 minus | no | 1000 | 2.1094 | 2.1862 | 0.965 | 239.4 | 230.6 | 0 / 0 | 0 |
| l=-8 minus | no | 1000 | 1.1167 | 1.2025 | 0.929 | 147.4 | 146.9 | 0 / 0 | 0 |
| l=-7 minus | no | 1000 | 0.6848 | 0.6515 | 1.051 | 89.8 | 89.8 | 0 / 0 | 0 |
| l=-6 minus | no | 1000 | 0.3530 | 0.3550 | 0.994 | 52.4 | 52.4 | 0 / 0 | 0 |
| l=-5 minus | no | 1000 | 0.1700 | 0.1577 | 1.078 | 27.0 | 27.0 | 0 / 0 | 0 |
| l=-4 minus | no | 1000 | 0.0739 | 0.0756 | 0.977 | 12.0 | 12.0 | 0 / 0 | 0 |
| l=-3 minus | no | 1000 | 0.0222 | 0.0252 | 0.879 | 3.9 | 3.9 | 0 / 0 | 0 |
| l=-2 minus | no | 1000 | 0.0041 | 0.0033 | 1.255 | 0.6 | 0.6 | 0 / 0 | 0 |
| l=-1 minus | no | 1000 | 0.0006 | 0.0005 | 1.110 | 0.1 | 0.1 | 0 / 0 | 0 |
| **all sets** | | 23000 | **0.5102** | **0.4782** | **1.067** | 61.4 | 57.9 | **0 / 0** | **0** |

Totals over 23000 instances: original 11.73 s, candidate5 11.00 s.

## Decision problem, same-characters, the authors' query files (factors 1 ± 2^l)

Paper Table 2 (same-characters, authors' machine): 18.7 ms and 1,159 black-box calls per instance.

| set | expected | n | original ms/instance | candidate5 ms/instance | ratio | bb calls orig | bb calls c5 | wrong (orig / c5) | arms disagree |
|---|---|--:|--:|--:|--:|--:|--:|--:|--:|
| l=-10 plus | yes | 1000 | 2.3852 | 2.1711 | 1.099 | 150.9 | 145.3 | 0 / 0 | 0 |
| l=-9 plus | yes | 1000 | 1.5376 | 1.2898 | 1.192 | 109.6 | 107.5 | 0 / 0 | 0 |
| l=-8 plus | yes | 1000 | 0.9545 | 0.7711 | 1.238 | 76.9 | 75.7 | 0 / 0 | 0 |
| l=-7 plus | yes | 1000 | 0.5941 | 0.4479 | 1.327 | 52.9 | 52.9 | 0 / 0 | 0 |
| l=-6 plus | yes | 1000 | 0.3713 | 0.2802 | 1.325 | 35.2 | 35.2 | 0 / 0 | 0 |
| l=-5 plus | yes | 1000 | 0.2297 | 0.1733 | 1.325 | 22.2 | 22.2 | 0 / 0 | 0 |
| l=-4 plus | yes | 1000 | 0.1238 | 0.1029 | 1.203 | 12.5 | 12.5 | 0 / 0 | 0 |
| l=-3 plus | yes | 1000 | 0.0686 | 0.0649 | 1.057 | 6.9 | 6.9 | 0 / 0 | 0 |
| l=-2 plus | yes | 1000 | 0.0383 | 0.0348 | 1.099 | 3.7 | 3.7 | 0 / 0 | 0 |
| l=-1 plus | yes | 1000 | 0.0225 | 0.0200 | 1.122 | 2.3 | 2.3 | 0 / 0 | 0 |
| l=0 plus | yes | 1000 | 0.0118 | 0.0136 | 0.868 | 2.0 | 2.0 | 0 / 0 | 0 |
| l=1 plus | yes | 1000 | 0.0078 | 0.0090 | 0.868 | 2.0 | 2.0 | 0 / 0 | 0 |
| l=2 plus | yes | 1000 | 0.0039 | 0.0044 | 0.881 | 2.0 | 2.0 | 0 / 0 | 0 |
| l=-10 minus | no | 1000 | 8.5757 | 4.9241 | 1.742 | 569.4 | 278.0 | 0 / 0 | 0 |
| l=-9 minus | no | 1000 | 3.4594 | 3.1483 | 1.099 | 242.4 | 188.6 | 0 / 0 | 0 |
| l=-8 minus | no | 1000 | 1.4268 | 1.4134 | 1.010 | 130.8 | 124.4 | 0 / 0 | 0 |
| l=-7 minus | no | 1000 | 0.6982 | 0.6796 | 1.027 | 79.6 | 79.4 | 0 / 0 | 0 |
| l=-6 minus | no | 1000 | 0.4308 | 0.3568 | 1.207 | 46.7 | 46.7 | 0 / 0 | 0 |
| l=-5 minus | no | 1000 | 0.2530 | 0.1854 | 1.365 | 25.9 | 25.9 | 0 / 0 | 0 |
| l=-4 minus | no | 1000 | 0.1101 | 0.1046 | 1.053 | 11.9 | 11.9 | 0 / 0 | 0 |
| l=-3 minus | no | 1000 | 0.0321 | 0.0301 | 1.069 | 4.1 | 4.1 | 0 / 0 | 0 |
| l=-2 minus | no | 1000 | 0.0046 | 0.0040 | 1.139 | 0.6 | 0.6 | 0 / 0 | 0 |
| l=-1 minus | no | 1000 | 0.0008 | 0.0007 | 1.149 | 0.1 | 0.1 | 0 / 0 | 0 |
| **all sets** | | 23000 | **0.9279** | **0.7056** | **1.315** | 69.2 | 53.5 | **0 / 0** | **0** |

Totals over 23000 instances: original 21.34 s, candidate5 16.23 s.

## Decision problem, all-characters, same pairs, factors (1 ± 4^l) as in the paper text

Paper Table 2 (all-characters, authors' machine): 27.3 ms and 1,860 black-box calls per instance.

| set | expected | n | original ms/instance | candidate5 ms/instance | ratio | bb calls orig | bb calls c5 | wrong (orig / c5) | arms disagree |
|---|---|--:|--:|--:|--:|--:|--:|--:|--:|
| l=-10 plus | yes | 1000 | 9.6630 | 11.3958 | 0.848 | 709.4 | 628.2 | 0 / 0 | 0 |
| l=-9 plus | yes | 1000 | 9.9816 | 9.1064 | 1.096 | 682.7 | 616.5 | 0 / 0 | 0 |
| l=-8 plus | yes | 1000 | 8.8918 | 7.3702 | 1.206 | 594.4 | 549.3 | 0 / 0 | 0 |
| l=-7 plus | yes | 1000 | 4.5290 | 4.0946 | 1.106 | 423.1 | 400.5 | 0 / 0 | 0 |
| l=-6 plus | yes | 1000 | 2.4633 | 1.9625 | 1.255 | 251.5 | 244.3 | 0 / 0 | 0 |
| l=-5 plus | yes | 1000 | 1.0011 | 1.0354 | 0.967 | 130.9 | 130.6 | 0 / 0 | 0 |
| l=-4 plus | yes | 1000 | 0.4714 | 0.3745 | 1.259 | 69.2 | 69.2 | 0 / 0 | 0 |
| l=-3 plus | yes | 1000 | 0.1968 | 0.1612 | 1.221 | 31.2 | 31.2 | 0 / 0 | 0 |
| l=-2 plus | yes | 1000 | 0.0560 | 0.0435 | 1.288 | 9.6 | 9.6 | 0 / 0 | 0 |
| l=-1 plus | yes | 1000 | 0.0150 | 0.0121 | 1.239 | 3.0 | 3.0 | 0 / 0 | 0 |
| l=0 plus | yes | 1000 | 0.0046 | 0.0037 | 1.258 | 2.0 | 2.0 | 0 / 0 | 0 |
| l=1 plus | yes | 1000 | 0.0017 | 0.0013 | 1.303 | 2.0 | 2.0 | 0 / 0 | 0 |
| l=2 plus | yes | 1000 | 0.0011 | 0.0010 | 1.143 | 2.0 | 2.0 | 0 / 0 | 0 |
| l=-10 minus | no | 1000 | 85.7162 | 31.3781 | 2.732 | 8465.2 | 1279.0 | 0 / 0 | 0 |
| l=-9 minus | no | 1000 | 83.6624 | 32.4494 | 2.578 | 8360.1 | 1270.2 | 0 / 0 | 0 |
| l=-8 minus | no | 1000 | 82.1630 | 27.4529 | 2.993 | 7898.9 | 1230.2 | 0 / 0 | 0 |
| l=-7 minus | no | 1000 | 63.5492 | 23.9244 | 2.656 | 6311.6 | 1099.7 | 0 / 0 | 0 |
| l=-6 minus | no | 1000 | 28.4589 | 15.0662 | 1.889 | 2596.1 | 746.0 | 0 / 0 | 0 |
| l=-5 minus | no | 1000 | 3.6722 | 4.0873 | 0.898 | 420.1 | 350.1 | 0 / 0 | 0 |
| l=-4 minus | no | 1000 | 0.9068 | 1.1977 | 0.757 | 147.4 | 146.9 | 0 / 0 | 0 |
| l=-3 minus | no | 1000 | 0.3013 | 0.3599 | 0.837 | 52.4 | 52.4 | 0 / 0 | 0 |
| l=-2 minus | no | 1000 | 0.0618 | 0.0759 | 0.814 | 12.0 | 12.0 | 0 / 0 | 0 |
| l=-1 minus | no | 1000 | 0.0041 | 0.0041 | 1.004 | 0.6 | 0.6 | 0 / 0 | 0 |
| **all sets** | | 23000 | **16.7727** | **7.4590** | **2.249** | 1616.3 | 385.9 | **0 / 0** | **0** |

Totals over 23000 instances: original 385.77 s, candidate5 171.56 s.

## Decision problem, same-characters, same pairs, factors (1 ± 4^l) as in the paper text

Paper Table 2 (same-characters, authors' machine): 18.7 ms and 1,159 black-box calls per instance.

| set | expected | n | original ms/instance | candidate5 ms/instance | ratio | bb calls orig | bb calls c5 | wrong (orig / c5) | arms disagree |
|---|---|--:|--:|--:|--:|--:|--:|--:|--:|
| l=-10 plus | yes | 1000 | 10.8239 | 9.1334 | 1.185 | 447.7 | 369.3 | 0 / 0 | 0 |
| l=-9 plus | yes | 1000 | 10.0735 | 9.3309 | 1.080 | 444.8 | 367.7 | 0 / 0 | 0 |
| l=-8 plus | yes | 1000 | 10.5214 | 7.9804 | 1.318 | 430.1 | 360.3 | 0 / 0 | 0 |
| l=-7 plus | yes | 1000 | 8.0576 | 7.5256 | 1.071 | 371.4 | 323.2 | 0 / 0 | 0 |
| l=-6 plus | yes | 1000 | 4.9266 | 3.7606 | 1.310 | 267.1 | 241.5 | 0 / 0 | 0 |
| l=-5 plus | yes | 1000 | 2.3548 | 2.1421 | 1.099 | 150.9 | 145.3 | 0 / 0 | 0 |
| l=-4 plus | yes | 1000 | 0.9299 | 0.9259 | 1.004 | 76.9 | 75.7 | 0 / 0 | 0 |
| l=-3 plus | yes | 1000 | 0.3776 | 0.3702 | 1.020 | 35.2 | 35.2 | 0 / 0 | 0 |
| l=-2 plus | yes | 1000 | 0.1244 | 0.0954 | 1.304 | 12.5 | 12.5 | 0 / 0 | 0 |
| l=-1 plus | yes | 1000 | 0.0395 | 0.0290 | 1.364 | 3.7 | 3.7 | 0 / 0 | 0 |
| l=0 plus | yes | 1000 | 0.0125 | 0.0118 | 1.061 | 2.0 | 2.0 | 0 / 0 | 0 |
| l=1 plus | yes | 1000 | 0.0046 | 0.0039 | 1.174 | 2.0 | 2.0 | 0 / 0 | 0 |
| l=2 plus | yes | 1000 | 0.0012 | 0.0009 | 1.309 | 2.0 | 2.0 | 0 / 0 | 0 |
| l=-10 minus | no | 1000 | 48.8266 | 21.2903 | 2.293 | 4574.9 | 708.5 | 0 / 0 | 0 |
| l=-9 minus | no | 1000 | 49.0214 | 21.3963 | 2.291 | 4528.0 | 704.7 | 0 / 0 | 0 |
| l=-8 minus | no | 1000 | 45.5002 | 20.4456 | 2.225 | 4392.0 | 692.0 | 0 / 0 | 0 |
| l=-7 minus | no | 1000 | 40.1986 | 19.5027 | 2.061 | 3938.0 | 651.6 | 0 / 0 | 0 |
| l=-6 minus | no | 1000 | 27.7083 | 14.6205 | 1.895 | 2492.5 | 517.6 | 0 / 0 | 0 |
| l=-5 minus | no | 1000 | 8.2548 | 5.8251 | 1.417 | 569.4 | 278.0 | 0 / 0 | 0 |
| l=-4 minus | no | 1000 | 1.7156 | 1.3014 | 1.318 | 130.8 | 124.4 | 0 / 0 | 0 |
| l=-3 minus | no | 1000 | 0.4774 | 0.3562 | 1.341 | 46.7 | 46.7 | 0 / 0 | 0 |
| l=-2 minus | no | 1000 | 0.1078 | 0.0815 | 1.323 | 11.9 | 11.9 | 0 / 0 | 0 |
| l=-1 minus | no | 1000 | 0.0054 | 0.0041 | 1.320 | 0.6 | 0.6 | 0 / 0 | 0 |
| **all sets** | | 23000 | **11.7419** | **6.3536** | **1.848** | 997.0 | 246.8 | **0 / 0** | **0** |

Totals over 23000 instances: original 270.06 s, candidate5 146.13 s.

