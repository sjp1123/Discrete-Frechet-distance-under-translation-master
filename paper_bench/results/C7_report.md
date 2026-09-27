# candidate7: verification of the audit fixes

candidate7 = candidate6 with the fixes listed in `candidate7/README.candidate7.md` (A–H plus
defaults and knobs). This file records the checks run before the timing run (`C7_timing.md`).
All runs: WSL2 Ubuntu 22.04 on this PC, g++ 11.4, CGAL 5.4, `RelWithDebInfo`. candidate7 ran in
its default configuration, which is the paper configuration. The harnesses and inputs are the audit's
own, collected in `paper_bench/c7_verify/`. To reproduce everything below, run
`bash paper_bench/c7_verify/verify.sh`.

## 1. The audit's reproducers

| defect | reproducer | candidate6 (audit) | candidate7 |
|---|---|---|---|
| A witness slack | 3 constructed 2×3 pairs at Sigspatial scale, decider at δ and 0.999δ (δ = 1.01–1.05·δ*) | NO at δ | **YES on all 6** |
| A (LMF) | 2 tuned 2×3 pairs (G1), value | 45,000 and 30,000 (+2.7 %) | **43,807.656897467** and **29,205.104598322** (δ* 43,807.6568974677 and 29,205.1045983245); decider YES at 45,000 / 30,000 / 44,000 / 29,300 |
| B truncated cut set | GPS curve 000008 vs its translated copy, 3 points moved 2 m, decider at δ* + 2e-7 | NO | **YES** |
| B | `test_far` (constructed so that the first 13 cut discs are the wrong ones), 26,400 queries | wrong NO at δ*(1+1e-7…3e-7) | **0 wrong** |
| C depth-limit loop | GPS curve 000005 vs its copy, one point moved 5 m, decider at δ* − 2.5e-7 | never terminates | **NO in 0.41 s** |
| D bisection loops | the audit's 3×2 instance scaled by 6.72e7, 1e8, 1e9 (LMF) | never terminates from 6.72e7 | **terminates** (value/S = 0.50017310 each) |
| D (coarse loop) | 16-gon of radius 2^24 with one tuned vertex (G3) | never terminates | **16,777,215.999999996** |
| E zero-width box | 180 concyclic lattice points at X = 2^50 (LMF) | SIGSEGV | **5,524.99999998926** (R = 5,525; original and candidate5 give the same) |
| F decider tie | `test_diag`: sub-decider at δ = the end-pair distance e = 0.17677669529663689 (true DFD 0.3536) | lt(e) = 1 (false YES) | **lt(e) = 0** |
| F | `test_one 4697` (value computation) | abort (empty initial box) | **0.2253469477** (δ* 0.225347) |
| F | `test_light`: sub-decider vs brute-force DP at ties, 73,826 queries | — | **0 false YES, 0 false NO** |
| H assert | GPS curve 000005 shifted to the origin (\|t\| ≈ 1.4e7), decider at δ* − 1e-9 | SIGABRT | **terminates (NO)** |

## 2. Fuzzing against exact references

| harness | instances / queries | result |
|---|---|---|
| candidate6's own tests: box predicates; single-point family (seeds 1–30,000 at (6,6), 1–20,000 at (12,12), decider and LMF); clustered family 1–20,000 | 300,000 predicate cases; 70,000 decider + 20,000 LMF instances; 20,000 pairs | 0 mismatches, 0 fails, 0 zero answers |
| `test_exact_ref` (exact brute force over all candidate translations), families 1–5 incl. the seeds of the F08 abort and the F03 hangs | 22,000 instances, 257,570 decider queries | 0 value errors (max \|v − δ*\| = 1.34e-8), 0 wrong decisions, no abort, no hang |
| `fuzz_oracle` (independent exact oracle, rational MEC over couplings): 11 generators with the default constructor, plus grid/dup/uniform with (depth, cut) = (6,3), (3,2), (40,3) and with `MAXREGION_DP_LIMIT=2` (forces the overflow fallback) | 46,000 instances, about 320,000 decider queries | FN = 0, FP = 0, value errors 0 (max 2.9e-8), crashes 0, timeouts 0 |

## 3. Real data, near the threshold

The audit's query files use candidate6's own LMF value v ± 1e-6 for every pair. The truth is YES at
v + 1e-6 and NO at v − 1e-6, since v is within 1e-8 of δ*. On the Sigspatial files candidate6 gave 9
wrong NO answers (defect B) and 16 queries never finished (defect C).

| set | queries | wrong | not finished |
|---|--:|--:|--:|
| Characters, v ± 1e-6 | 42,000 | **0** | 0 |
| Sigspatial, v ± 1e-6 | 2,000 | **0** | 0 |

## 4. A regression found and fixed during this verification

The first version of fix F added its rounding margin to every heuristic comparison. That included
the one in `getLastReachablePoint`, where a step of one point (maxdist = 0) is the only test of that
point. The test became stricter than the definition, and the value came out R + margin on the 2^50
instance (5,526.25). The margin now applies only to multi-point steps (maxdist > 0); for a single
point the heuristic is the direct test itself. Every result above is from the corrected build.
