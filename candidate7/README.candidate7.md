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
