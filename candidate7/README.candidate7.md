# Candidate 7 — candidate6 with the audit's defects fixed

candidate7 is candidate6 (the box-restricted maximal-set base case, `README` of commit fb69a13)
with every defect fixed that the 2026-09-26/27 audit of candidate6 confirmed. Each defect was
reproduced by two independent verifiers. The algorithm is unchanged. The fixes are local, and each
is marked `FIX (candidate7)` in the source.

## 1. Fixes

| id | defect (audit finding) | where | fix |
|---|---|---|---|
| A | The box-family witness was accepted up to r + 9e-9, which is the decider's own threshold, so the decider's raw-coordinate rounding (~1e-9 at 1.4e7) could reject it. Constructed Sigspatial-scale input gave a wrong NO at 1.01·δ* and an LMF value 2.7% high (F02, G1-1). candidate5 has the same decider failure. | `lib/cgal_disk_arrangements/maximal_regions.cpp` `enumerate_box` | A double witness is accepted only if certified within r. Otherwise the exact optimum (≤ r, since the set is feasible) is rounded. The decider keeps the whole slack for its own rounding. |
| B | The decider's depth-limit base case got a cut-disc list truncated at 13, because the brute-force `computeCutCenters` ignored `threshold=false`. This gave a wrong NO for δ < δ*(1+1.35e-6), up to δ*+2e-7 absolute on a translated GPS curve (F01, G4-2). Inherited from `original`. | `src/frechet_under_translation.cpp`, both brute-force loops | Stop early only when `threshold` is set, as the kd-tree branch already does. |
| C | The same branch looped forever on a box without cut discs (`continue` without `step()`) (F03, G4-3). Inherited. | `lessThanImpl` | `search_boxes.step()` before `continue`. |
| D | Bisections never terminated once max and min were neighbouring doubles with a spacing at or above ε/2. That happens for LMF values from about 2^22 (original) or 2^25 (candidate6), and in the coarse lower-bound evaluation from about 1e7 (F04, G3-1). Inherited. | 6 loops: `fut_n6_algorithm.cpp` ×3, `discrete_frechet_queries.h`, `discrete_frechet_light.cpp`, `frechet_under_translation.cpp` | Stop when the midpoint equals an end of the bracket. |
| E | The box-family overflow fallback crashed (SIGSEGV) on a box of zero width or height, which occurs once a side is 1 ulp wide (F09). New in candidate6. | `disc_arrangement_traversal.cpp` `build_candidates_box` | Insert only non-degenerate box sides: one segment, or none for a point box. A point box is its own witness. |
| F | The fixed-translation decider's free/non-free heuristics compare against rounded prefix lengths. At an exact tie (δ equal to an endpoint distance) this produced a false YES through propagation2, and calcDistance2 aborted on an empty initial box (F08). Inherited. | `discrete_frechet_light.{h,cpp}` | Every heuristic step that spans several points (maxdist > 0) gets a margin of 8u·((n+2)·L + max\|x\| + δ), a bound on the prefix-sum, coordinate and squaring rounding. A one-point step is the direct test itself and stays unchanged; a first version that put the margin there too was wrong, see `C7_report.md` §4. |
| H | `intersection_interval` could return begin < 0 at \|t\| ≈ 1.4e7 when a box edge was a few ulps long, which aborted on the forced assert (G4-1). Inherited. | `src/geometry_basics.cpp` | Clamp the fast-path entry/exit parameters to [0, 1]. The existing binary search then decides. |
| G | The default configuration (no env) enumerated at the decider's own radius with zero witness tolerance and lenient double predicates. It gave wrong NO at 0.5–2 % margin and LMF values up to 2.1 % high, including 3 Sigspatial pairs (F05, G2-1). | `disc_arrangement_traversal.cpp` | The paper configuration is now the default: `MAXREGION_EXACT` defaults to 1 and `MAXREGION_SLACK` to 0. `MAXREGION_SLACK=caller` gives the old behaviour. |
| knobs | An unknown `MAXREGION_FIX` value silently selected the unsound candidate5 path, and `N6_RANGE` was switched off only by a leading `0` (F10). | `src/fut_n6_algorithm.h`, `disc_arrangement_traversal.cpp` | Unknown values print a warning and keep the default. `N6_RANGE` accepts 0/1, off/on, false/true, no/yes. |

Not changed: the Lipschitz search, disc selection, the kd-tree, the enumeration and predicates of
candidate6 (their exactness was confirmed: 117,600 family comparisons and about 4 million predicate
cases, 0 mismatches), the `CUT_LIMIT`/`DEPTH_LIMIT` constructor parameters and `-std=c++14`. CGAL ≥ 6
needs `-std=c++17`, as for every arm. The Vim swap file `src/.swp` of the authors' tree is not copied.

## 2. Configuration

No environment is needed: the defaults are the paper configuration (`MAXREGION=cech`,
`MAXREGION_EXACT=1`, `MAXREGION_SLACK=0`, `MAXREGION_FIX=union`, `N6_RANGE=1`). The old values
remain available for experiments.

## 3. Build

```
bash _c7_build.sh                                  # -> ~/b_candidate7
ARMS="original candidate7" bash paper_bench/build.sh   # paper_bench per arm
```

## 4. Evidence and timings

- `paper_bench/results/C7_report.md`: the reproducer of every audit finding, fuzzing against exact
  references, and real-data queries within ±1e-6 of δ*.
- `paper_bench/results/C7_timing.md`: the full Characters and Sigspatial runs, paired against
  `original` and `candidate5`. It covers every answer and value, plus the timings in the format of
  the paper's Tables 2 and 4. Raw data are in `raw_c7_timing.tar.gz`. Plan, runner and analysis:
  `paper_bench/plan_wsl_c7.py`, `run_wsl_c7.sh` and `analyze_c7.py`.

## 5. Follow-up after the merge

Two fixes on top of the merged candidate7, marked `FIX (candidate7 follow-up)`, and the regression
tests of the candidate6 audit fixes ported to `tests/`.

| id | issue | where | fix |
|---|---|---|---|
| A′ | Fix A kept a rounded exact witness if it cleared r + 9e-9, the whole slack, so nothing was left for the decider's own rounding. When even that check failed, the code only counted it (`mec_fail`) and emitted the witness. The double witness was accepted within r, however large the coordinates. | `maximal_regions.cpp` `enumerate_box`, `disc_arrangement_traversal.cpp`, `fut_n6_algorithm.{h,cpp}` | N6 passes a bound on the decider's rounding at a box point, 2u(max\|p\| + R) + 6uR (p from curve 1, R = δ + slack), and the witness tolerance becomes slack − bound, which can be negative. Both acceptance tests use it: the double witness within min(r, r + tol), the rounded exact one within r + tol. A region that fails keeps its witness, and its discs also go to the box-arrangement fallback, so the decider also tests the original's candidates for that set. |
| E′ | The E fix pushed the point of a point box but still built the arrangement of the component's circles (601 candidates on the test's 24 discs). | `disc_arrangement_traversal.cpp` | A point box yields only itself. The arrangement code moved into `append_box_arrangement_vertices`, shared by both fallback causes. |

The tolerance stays ≥ 0 while max|p| + R stays below about 4e7 (9e-9 / 2u). Then both tests are
the merged ones, bit for bit. Sigspatial has max|p| ≈ 1.4e7.

Evidence (commands in `tests/CMakeLists.txt` and the file headers):

- Paper subsets (Characters LMF 1,000 pairs, Sigspatial LMF 200, Characters decider 2,300 queries,
  Sigspatial decider 920): values, answers and `bbcalls` are identical to the merged candidate7,
  and the times agree within noise.
- `paper_bench/c7_verify/verify.sh` (QUICK=1): every job passes. The reproducers of A/B/C/H give
  the merged answers and values.
- `test_witness_margin` (20,000 three-disc cases per setting): the replayed decider rejects 0
  witnesses, for the merged build and the follow-up alike. The fallback fires only when both curves
  sit at 6e7 or 1.2e8 (20,000 of 20,000 with the optimum inside the box, 253 and 1,021 with it on an
  edge) and never at 1.36e7 or 3e7.
- `test_large_values` 1e8: identical. At 1e9 the fallback fires in 46 regions and 8 of 300 values
  move by 1 ulp; the largest relative error to the exact MEC radius stays 1.2e-15.
- `test_tangency_windows` at raw-coordinate scale 1, 3, 8 (far 0/1, 3,102 queries each): 0 NO.
- `test_degenerate_box`: point box 601 → 1 candidate; the other boxes are unchanged.
- `test_box_predicates`, `test_single_point_family` (decider and LMF), `test_clustered_family`,
  `test_depth_limit` (−8, −13, 8): identical output apart from timings.

No test found a witness the merged candidate7 emits and the decider rejects, so A′ is a guard for
coordinates beyond the paper data, not a fix of an observed failure.

| test | covers | run |
|---|---|---|
| `test_depth_limit` | B, C | `test_depth_limit 1 200 -13` (m < 0: cluster-major order; `TEST_ALARM` overrides the 5 s alarm per query) |
| `test_large_values` | D | `test_large_values 1 300 1e8` → `S seed value exact_MEC rel_err` |
| `test_degenerate_box` | E, E′ | no arguments |
| `test_tangency_windows` | A | `test_tangency_windows 1 150 [far] [scale]` |
| `test_witness_margin` | A′ | `test_witness_margin <count> [edge] [offset] [same_city]`; `_nofix` passes the whole slack |
