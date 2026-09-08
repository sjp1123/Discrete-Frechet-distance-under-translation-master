# Candidate 5 — `original` + Čech maximal-region enumeration, Epeck fallback

Candidate 5 is **`original/` verbatim** with the candidate-region emitter swapped
for candidate 4's Čech pipeline.  Nothing else changes: the same Epeck kernel,
the same CGAL DCEL arrangement code, the same divide-and-conquer driver, kd-tree,
sub-Fréchet decider and `CUT_LIMIT`/`DEPTH_LIMIT` knobs.  Components too large
for the pipeline's DP are handed back to the **Epeck arrangement**, which is why
the arm is exact by construction wherever the pipeline declines to answer.

This is the arm the earlier work never built.  Candidate 4 put the pipeline on
top of candidate 2's **double** rewrite, so it could only ever measure the
pipeline's value *after* the arrangement had already been made cheap.  Candidate
5 measures it against the arithmetic the paper actually uses.

---

## 1. What changed relative to `original/`

| file | change |
|---|---|
| `lib/cgal_disk_arrangements/maximal_regions.h` | **copied verbatim from candidate4** — pipeline API |
| `lib/cgal_disk_arrangements/maximal_regions.cpp` | **copied verbatim from candidate4** — Stages 0–5 |
| `lib/cgal_disk_arrangements/disc_arrangement_traversal.cpp` | dispatcher + Epeck fallback; the original vertex enumeration kept verbatim on every pre-existing constructor |
| `lib/cgal_disk_arrangements/disc_arrangement_traversal.h` | one added constructor (slack-aware) |
| `lib/cgal_disk_arrangements/CMakeLists.txt` | build `maximal_regions.cpp` |
| `src/fut_n6_algorithm.cpp` | **one line**: pass `epsilon_slack` to the traversal |

`maximal_regions.{h,cpp}` are byte-identical to candidate 4's.  They depend only
on `Point` / `Disc` / `Discs` from `disc_arrangement_traversal.h`, which
`original/` already provides, so no adaptation was needed.

The idea, the Helly-3 / equal-radii predicate system and the stage breakdown are
documented in `../candidate4/README.candidate4.md` §2 and are not repeated here.

## 2. Runtime knobs

| env | default | meaning |
|---|---|---|
| `MAXREGION` | `cech` | `cech` \| `legacy` (= `original` verbatim, the control arm) |
| `MAXREGION_BAND` | `1e-12` | relative leniency on `r²` |
| `MAXREGION_DP_LIMIT` | `20` | components larger than this go to the Epeck fallback |
| `MAXREGION_SLACK` | *(from caller)* | override the predicate slack; `0` disables slack alignment |

`MAXREGION=legacy` restores the original code path inside the same binary; §4
below shows it reproduces `original`'s answers bit-for-bit at no measurable cost,
which is what makes the contrast a clean one.

## 3. The Epeck fallback must run on the **un-inflated** radii

`N6Alg` builds discs of radius `distance` but asks the decider about
`distance + epsilon_slack`, so the pipeline enumerates at `distance + slack`
(candidate 4 §5).  The fallback must **not** inherit that inflation.

An arrangement vertex sits exactly *on* a circle.  At the inflated radius the
`sqrt` construction error there (~1.5e-8·r) exceeds the slack (~9e-9), so the
decider can miss the very pair the vertex was built for — this is candidate 4's
`C4S` pathology (§6, "Caveat on `C4S`"), and it is silent.

Measured on `test_cases/bench_small`, with the fallback forced to fire
(`MAXREGION_DP_LIMIT=4`, 758 fallback components over 15 pairs):

| fallback radius | answer vs `original` |
|---|---|
| inflated (`radius + slack`) | off by up to **2.7e-4** — 11/15 pairs breach ε = 1e-7 |
| original radius | **bit-identical**, 15/15 |

`build_candidates_cech` therefore slices the fallback's sub-vectors out of
`discs`, never out of the inflated copy.  The pipeline keeps the inflated set;
the two are deliberately not the same object.

## 4. Measured runtime

`_c5_bench.sh` + `_c5_analyze.py`, `test_cases/bench_100` (100 synthetic pairs,
n ∈ [100, 1000]), `fut_lmf`, **100 pairs × 4 arms × 3 reps**, arm order reshuffled
every rep, min per (pair, arm), 120 s censoring (0 censored).  Per-pair
log-ratios → geomean with a 10 000-resample bootstrap 95% CI, the same protocol
as `_c4_bench.sh` / `_factor_perpair.sh`.

Arms: `A0` = `original`; `C5L` = candidate5 `MAXREGION=legacy`; `C5N` =
`MAXREGION=cech MAXREGION_SLACK=0`; `C5` = `MAXREGION=cech` (default).

| contrast | WALL | ARRANGEMENT | INTERNAL (arr+fre) | decider calls |
|---|---|---|---|---|
| **`A0` → `C5`** | **1.50× [1.42, 1.58]** | 32.8× [28.9, 37.5] | 8.52× [7.62, 9.60] | 3.65× [3.36, 3.99] |
| pipeline alone (`A0/C5N`) | 1.47× [1.40, 1.55] | 30.2× | 7.87× | 3.40× |
| slack given pipeline (`C5N/C5`) | 1.02× [0.98, 1.06] | 1.09× | 1.08× | 1.07× |
| control (`A0/C5L`) | 1.01× [0.99, 1.03] | 0.99× | 1.00× | 1.000× |

Faster on **95/100** pairs; median 1.48×, p90 1.99×.  Absolute over all 100
pairs: wall **59.4 s → 39.8 s**, arrangement stage **5.03 s → 0.17 s**, internal
**20.5 s → 2.34 s**, decider calls **1 055 717 → 287 713**.

Run twice end to end: 1.508× and 1.498×, i.e. reproducible inside the ~4% rep
noise characterised in `_x1_results/ENVIRONMENT.md` §2.

### This inverts candidate 4's factor decomposition

Candidate 4 found the pipeline alone to be a wash (`D1/C4N` = 0.98×) with the
whole 1.44× coming from slack alignment.  Here it is the other way round: the
pipeline alone is 1.47× and slack alignment is **not significant** (CI spans 1).

Both readings are consistent, and the reason is what the pipeline is replacing.
On candidate 2's double list the arrangement stage was already down to ~115 ms
over 100 pairs (X1 §3), leaving nothing for a faster emitter to win — only the
search trajectory could still move.  On `original` the Epeck arrangement *is* the
dominant cost of the inner loop, so deleting it pays directly.

The end-to-end figure stays far below the 32.8× arrangement win for the ordinary
Amdahl reason recorded in R1: the N6 loop is a minority share of the wall.  The
remaining ~40 s is preprocessing and the driver's own decider calls.

## 5. Correctness

Against `original` (exact Epeck), over the 100 benchmark pairs:

| arm | max \|Δ\| | pairs over ε = 1e-7 | bit-identical |
|---|---|---|---|
| `C5L` | 0.00e+00 | 0/100 | **100/100** |
| `C5N` | 7.13e-09 | 0/100 | 15/100 |
| `C5`  | 1.15e-08 | 0/100 | 0/100 |

`_c5_check.sh <case_dir> [N]` runs the gates: default vs `original`, the forced
fallback of §3, and the `C5L` bit-identity check.

## 6. Operating limits

* **The fallback is dormant on the `fut_lmf` path.**  With `CUT_LIMIT = 12` and
  `MAXREGION_DP_LIMIT = 20`, `overflow = 0` on every build (checked over 20
  benchmark pairs).  The §4 numbers are therefore the pipeline alone; Epeck is
  carried as a safety net, not as part of the measured path.
* **The global `n6` entry point is parity.**  There the disc set is the full
  n₁·n₂ product, every component overflows, and all the work goes to the Epeck
  fallback.  Measured over `_nsweep` n = 6…20: answers **bit-identical** to
  `original`, wall **0.84×–1.07×, median ≈ 1.00×**.  Same conclusion as
  candidate 4 §6, and the same remedy — implement §5 of the design note
  (Bron–Kerbosch + per-clique DP) before using `n6` as anything but a reference.
* `CUT_LIMIT` above `MAXREGION_DP_LIMIT` disables the pipeline entirely and
  leaves only the pre-pass overhead; see candidate 4 §7, which applies verbatim.

## 7. Reproducing

```
bash _c5_build.sh                                   # -> ~/b_candidate5
bash _build_one.sh original                         # -> ~/b_original  (baseline)
bash _c5_check.sh  test_cases/bench_small 15        # correctness gates
bash _c5_bench.sh  test_cases/bench_100 100 3 ~/_c5/bench.csv
python3 _c5_analyze.py ~/_c5/bench.csv              # -> _c5_analysis.txt
```

Committed outputs: `_c5_bench.csv` (per-pair raw) and `_c5_analysis.txt`.

**Environment used for the §4 numbers** — WSL2 Ubuntu, g++ 15.2, CGAL **6.1.1**,
CMake 4.2.3, `RelWithDebInfo`.  This is *not* the g++ 13 / CGAL 5.6 toolchain of
`_x1_results/ENVIRONMENT.md`.  CGAL ≥ 6 requires C++17 while the tree requests
C++14, so `-std=c++14` was raised to `-std=c++17` — applied identically to both
arms, so the contrast is unaffected; the committed `CMakeLists.txt` is left at
C++14 to match the other arms.  Absolute times are not comparable across the two
toolchains; the ratios are the result.

**Data caveat.**  `geolife_100` needs the 2.2 GB Geolife dataset, which is not
committed and was not available on the measuring host, so §4 uses the committed
synthetic `bench_100`.  The prior arms' headline numbers are on `geolife_100`
and are therefore **not** directly comparable.  The one same-dataset anchor is
the repository's own `bench_100.out` (different machine), which recorded
`original` → candidate 2 at 1.24×.
