# candidate7 verification kit

These are the harnesses and inputs behind `paper_bench/results/C7_report.md`. The sources come from
the 2026-09-26/27 audit of candidate6 and are unchanged.

```
bash paper_bench/c7_verify/verify.sh [OUT=~/c7_verify_out]     # build if needed, run all, summarise
bash paper_bench/c7_verify/summarize.sh [OUT]                  # summary again
QUICK=1 bash paper_bench/c7_verify/verify.sh ~/c7_quick        # smoke test, counts / 10
```

It builds the harnesses in `~/b_c7v` and paper_bench in `~/b_pb_candidate7`, then runs 12 jobs at a time.
candidate7 runs in its default configuration, which is the paper configuration. The full run takes
about 20 minutes on the 18-thread test PC. It needs CGAL, Boost and GMP (gmpxx) and was tested with
WSL Ubuntu 22.04, g++ 11.4 and CGAL 5.4.

| file | what it checks | audit finding |
|---|---|---|
| `src/test_exact_ref.cpp` | value and decider against an exact brute force over all candidate translations (families 1–5) | F08, F03 seeds |
| `src/fuzz_oracle.cpp` | decider (δ*(1 ± u)) and value against an independent exact oracle (rational MEC over couplings); 11 generators; depth/cut variants; `MAXREGION_DP_LIMIT=2` | E2, F01 |
| `src/test_far.cpp` | a family where the first 13 cut discs are the wrong ones (depth-limit truncation) | F01 (B) |
| `src/test_one.cpp`, `src/test_diag.cpp`, `src/test_light.cpp` | the sub-decider's false YES at a tie, and the resulting abort | F08 (F) |
| `src/probe.cpp` | LMF termination on scaled instances and on the 2^24 16-gon | F04, G3-1 (D) |
| `src/e2e.cpp` | LMF on 180 concyclic lattice points at X = 2^50 (zero-width boxes) | F09 (E) |
| `cases/a/case_sig_*.txt`, `cases/a/HIT_*.txt` | Sigspatial-scale 2×3 pairs where a certified witness was rejected; decider and LMF | F02, G1-1 (A) |
| `cases/g4/` | GPS curves against translated copies: wrong NO at δ*+2e-7, a hang at δ*−2.5e-7, an assert at \|t\| ≈ 1.4e7 | G4-2, G4-3, G4-1 (B, C, H) |
| `cases/near_threshold/` | candidate6's own LMF value ± 1e-6 for every Characters and Sigspatial pair (truth: plus YES, minus NO) | E3 |
| `candidate7/tests/*` (built from the arm) | candidate6's shipped tests: box predicates, single-point and clustered families | README of candidate6 |

The case files hold `<delta>`, then `<n>` and n points, then `<m>` and m points. verify.sh turns
them into paper_bench curve files.
