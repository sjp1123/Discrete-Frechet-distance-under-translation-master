# Candidate 6 — candidate5 made sound per box

Candidate 6 is **candidate5 with its base case repaired**.  candidate5's
maximal-region emitter ignores the discs that contain the whole search box; its
witnesses (MEC centres) may then fall outside those discs, and the base case can
answer NO for a box that contains a YES translation.  The authors' `original`
has a second, independent defect in the LMF base case.  Both show up as wrong
answers on constructed inputs (section 2); neither showed up on the paper's data.

Nothing outside the base case changes: the same divide-and-conquer driver, disc
selection, kd-tree, sub-Fréchet decider, Epeck fallback and `CUT_LIMIT` /
`DEPTH_LIMIT` knobs.  `MAXREGION_FIX=none N6_RANGE=0` restores candidate5's code
path inside the same binary.

---

## 1. What changed relative to candidate5

| file | change |
|---|---|
| `lib/cgal_disk_arrangements/maximal_regions.{h,cpp}` | `enumerate_box()`: maximal sets whose intersection meets the box; filtered-exact Q1/Q2; certified witness. `enumerate_block()` for the rejected `aware`/`lazy` arms. `enumerate()` untouched |
| `lib/cgal_disk_arrangements/disc_arrangement_traversal.{h,cpp}` | box / block constructors, `hadOverflow()`, `hull_vertices()` |
| `src/fut_n6_algorithm.{h,cpp}` | `MAXREGION_FIX` dispatch, `setBox()`, `setContainProvider()`, `calcDistanceRange()` |
| `src/frechet_under_translation.{h,cpp}` | passes the box (and a provider of box-containing discs) to the base case; LMF range fix; `collectContainCenters()` |
| `tests/` | the two counterexample harnesses and the predicate unit test |

### 1.1 The box family (soundness)

The base case now enumerates the inclusion-maximal subsets X of the cut discs
with **∩X ∩ B ≠ ∅**, B the search box.  Helly on X ∪ {B} (all convex) reduces
this to three predicates:

* P3(i,j,k): the three discs share a point (candidate5's predicate, unchanged);
* Q1(i): D_i ∩ B ≠ ∅;
* Q2(i,j): D_i ∩ D_j ∩ B ≠ ∅.

The witness is the point of B minimising max_{i∈X} |p − c_i| (the MEC centre
when it lies in B).  It lies in B, so it lies in every disc that contains B;
those discs never have to be found.

*Why this is enough.*  Let p ∈ B be a YES translation and T = S(p) ∩ C, C the
cut discs.  T ∪ {B} has the common point p, so T ⊆ X for some maximal X, whose
witness w satisfies S(w) ⊇ X ∪ {discs containing B} ⊇ S(p) (a disc that neither
crosses nor contains B cannot contain p).  The decider is monotone in S, so w is
YES.

### 1.2 Exactness

In exact mode (`MAXREGION_EXACT=1`) every combinatorial predicate is
filtered-exact (double evaluation with a rigorous error bound, `cpp_rational`
fallback, sqrt-free):

* Q1: clamp the centre to B (exact) and call `pred2_exact` with radius r/2
  (halving is exact).
* Q2: if the midpoint of c_i c_j lies in B (exact sign of x_i + x_j − 2x_B) it is
  P2.  Otherwise the lens meets B iff it meets an edge; on an edge, 1-D Helly on
  (chord_i, chord_j, edge): the two chord–edge tests are Q1-type, and chord
  overlap is |x_i − x_j| ≤ √h_i + √h_j, squared twice into a degree-4 polynomial.
* Witness: accepted only if |w − c_i| ≤ r + tol for every member, certified in
  double; otherwise the optimum is recomputed exactly (every candidate — pair
  midpoint, triple circumcentre, corner, clamped projection, clamped crossing on
  an edge — is rational) and rounded.  tol = the part of the decider's slack the
  pipeline radius does not already use (9e-9 with `MAXREGION_SLACK=0`).

candidate5 certified P2/P3 but not its MEC witness.

### 1.3 Order of witnesses (`union`, the default)

The box family alone slowed YES decider queries by 13–30%: candidate5's
out-of-box witnesses often found a YES elsewhere and ended the search early.
`union` tests candidate5's witnesses first and the box family only if they are
all NO.  Soundness comes from the box family; the candidate5 witnesses are a
free early exit (any YES witness is a real translation).

### 1.4 LMF range fix (`N6_RANGE`, default on)

In `original` and candidate5, the LMF base case calls
`N6Alg::calcDistance(curve1, curve2)`, which binary-searches [0, f(τ_start)]
(not the paper's [ℓ_B, δ̃]), and assigns the result to `max` unconditionally.
One wrong probe above δ̃ therefore raises the global upper bound, up to
f(τ_start) ≤ 2δ*.  `calcDistanceRange(ℓ_B, max)` searches [ℓ_B, max] only and
`max = min(max, result)`.  This alone makes candidate5's LMF 3.6% faster on
Characters (1,000 pairs).

## 2. Runtime knobs

| env | default | meaning |
|---|---|---|
| `MAXREGION_FIX` | `union` | `union` \| `box` \| `none` (= candidate5) \| `full` \| `aware` \| `lazy` |
| `N6_RANGE` | `1` | `0` restores the authors' LMF base-case search |
| `MAXREGION_EXACT`, `MAXREGION_SLACK`, `MAXREGION_BAND`, `MAXREGION_DP_LIMIT`, `MAXREGION` | as candidate5 | the paper configuration is `MAXREGION_EXACT=1 MAXREGION_SLACK=0` |

`full`: box-containing discs added to the emitter's input.  `aware`: those
discs' hull as a common Helly block.  `lazy`: candidate5 first, `aware` when a
NO cannot be certified.  All three are correct and were rejected on runtime
(section 4).

## 3. Correctness evidence (paper configuration)

| test | candidate5 | original | candidate6 |
|---|---|---|---|
| `tests/test_single_point_family 1 20000 6 6` — decider, δ = r*(1+u), u ∈ [1e-4, 1] | **78 NO** (truth YES) | 0 | 0 |
| same, `LMF=1` — value vs exact r* | 0 | **75 wrong (≤ 2×)** | 0 |
| `tests/test_clustered_family 1 20000` — decider at v(1+u)+1e-7, 8 u each | **1,215 NO on 893 pairs**, up to u = 3e-2 | values: **63 wrong (≤ 2×)** | 0 NO, values = original+`N6_RANGE` |
| random curves, n ≤ 10, 8,000 pairs × (value + 6 queries) | = original | — | = original |
| `tests/test_box_predicates` — Q1/Q2 vs independent rational reference, 300,000 cases (mostly ±3 ulp from tangency; offsets 1.36e7; near-vertical pairs) | — | — | 0 mismatches |
| Characters LMF 1,000 pairs, Sigspatial LMF 200 pairs | — | — | \|Δ\| ≤ 1e-7 vs candidate5 |
| Characters decider 2,300 queries, Sigspatial decider 920 queries (first 100 / 40 of each of the 23 paperq4 files) | all correct | all correct | all correct |

Rational fallbacks on real data: Q2 33 of 379,726 (Sigspatial LMF, 40 pairs),
0 of 465,220 (Sigspatial decider), 1 of 1,257,224 (Characters LMF, 200 pairs);
witness 0.  Max witness excess over r: 0 everywhere (the first, lenient-double
version of Q1/Q2 reached 1.15e-8 on Sigspatial, above the 9e-9 slack — the
reason the predicates are now exact).

## 4. Measured runtime

Container, 2 vCPU, one process at a time pinned to one core (an earlier
two-stream run was biased: its arm rotation always co-scheduled the box arm with
`original`), arms interleaved per chunk, g++ 13.3, CGAL 5.6, Boost 1.83, CMake
3.28, `RelWithDebInfo`, `MAXREGION_EXACT=1 MAXREGION_SLACK=0`.  Subsets: the
first rows of the committed query files as described in section 3.

| benchmark | candidate5 | `box` | **`union` (default)** |
|---|---|---|---|
| Characters LMF, 1,000 pairs (every 21st of `characters_uci_lmf_pairs.txt`) | 27.87 s | +5.5%, geomean +6.7% [4.8, 8.6] | **+6.4%, geomean +1.5% [−0.4, 3.4]** |
| Sigspatial LMF, 200 pairs (every 5th of `sigspatial_pairs.txt`) | 10.61 s | +3.1% | **−1.1%**, geomean −2.9% [−6.9, 0.8] |
| Characters decider, 2,300 queries | 15.47 s | +5.3% | **+2.2%** |
| Sigspatial decider, 920 queries | 64.22 s | +2.4% | **+1.4%** |

`original` against candidate5 on the same host and subsets: Characters LMF
4.07×, Characters decider 2.06×, Sigspatial decider 1.115×.  **These are
container subsets; re-measure on the paper host with the full query sets
(`paper_bench/run_wsl_paired.sh`) before quoting any ratio.**

Rejected alternatives, Characters LMF first 20 pairs (candidate5: 0.55 s):
`full` 65.0 s, `aware` 2.09 s, `lazy` 2.10 s.  The box-containing discs'
centre hull averaged 51 vertices, so the Helly block multiplied P3 calls
from 0.28 M to 21 M; `lazy`'s certificate failed in 77% of base cases.

## 5. Precision budget

The decider accepts radius + 9e-9 (absolute; `N6Alg` slack = 1e-8 − 1e-9).
Radii seen: Characters δ* 1.4–106 (base cases ≤ 98.5); Sigspatial (EPSG:3857
metres) δ* 664–48,550, median 13,180 (base cases ≤ 3.85e4).  The witness
guarantee is with respect to the centres c = π_i − σ_j as doubles.  The decider
itself works in raw coordinates; Sigspatial's are ≈ 1.36e7 (ulp 1.9e-9), a
fifth of the slack.  That term is shared by every arm, `original` included.

Not exercised by any run: the box family's overflow fallback (arrangement of
the component plus the box edges); `overflow = 0` throughout.

## 6. Reproducing

```
bash _c6_build.sh                                          # -> ~/b_candidate6
ARMS="original candidate5 candidate6" bash paper_bench/build.sh
cmake -S candidate6/tests -B ~/b_c6_tests -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DCMAKE_CXX_FLAGS="-include cstdint -include array -include cstddef"
cmake --build ~/b_c6_tests -j
cd ~/b_c6_tests && export MAXREGION_EXACT=1 MAXREGION_SLACK=0
./test_box_predicates                                      # mismatches=0
./test_single_point_family 1 20000 6 6                     # fails=0
MAXREGION_FIX=none N6_RANGE=0 ./test_single_point_family 1 20000 6 6   # fails=78 (candidate5)
./test_clustered_family 1 20000                            # no 0 in the answer columns
```
