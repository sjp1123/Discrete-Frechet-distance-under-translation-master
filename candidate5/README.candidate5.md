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

## 4. Measured runtime — `geolife_100` (real data)

`_c5_bench_geo.sh` + `_c5_analyze.py`, the **same `geolife_100` manifest and the
same curve files candidate 4 was measured on**, `fut_lmf`, **100 pairs × 4 arms ×
3 reps**, arm order reshuffled every rep, min per (pair, arm), 60 s censoring
(0 censored).  Per-pair log-ratios → geomean with a 10 000-resample bootstrap
95% CI, the same protocol as `_c4_bench.sh` / `_factor_perpair.sh`.

Arms: `A0` = `original`; `C5L` = candidate5 `MAXREGION=legacy`; `C5N` =
`MAXREGION=cech MAXREGION_SLACK=0`; `C5` = `MAXREGION=cech` (default).

> The curve files are not committed.  They were regenerated from the official
> Geolife 1.3 zip with the tree's own `geolife_converter.py`.  Identity with
> candidate 4's data is established by `original`'s answers being
> **bit-identical on all 100 pairs** to the `A0` column of `_c4_bench.csv`
> (exact Epeck output — one differing coordinate would move the 20th digit),
> all 200 manifest files present, and the manifest's `n` column matching
> max(curve length).

| contrast | WALL | ARRANGEMENT | INTERNAL (arr+fre) | decider calls |
|---|---|---|---|---|
| **`A0` → `C5`** | **1.69× [1.52, 1.90]** | 48.2× [36.8, 63.4] | 36.9× [28.4, 47.9] | 4.49× [3.72, 5.44] |
| pipeline alone (`A0/C5N`) | 1.24× [1.14, 1.34] | 22.4× | 17.0× | 2.70× |
| slack given pipeline (`C5N/C5`) | 1.36× [1.22, 1.53] | 2.16× | 2.17× | 1.66× |
| control (`A0/C5L`) | 1.00× [0.97, 1.01] | 0.98× | 0.98× | 1.000× |

Faster on **86/100** pairs; median 1.50×, p90 3.88×.  Absolute over all 100
pairs: wall **68.0 s → 39.7 s**, arrangement **12.37 s → 0.61 s**, internal
**21.07 s → 1.11 s**, decider calls **4 000 135 → 1 201 618**.

Answers vs `original`: max |Δ| **1.11e-8**, **0/100** pairs over ε = 1e-7;
`C5L` bit-identical on 100/100.

### Against candidate 4, on the same pairs

Both runs share `A0` = `original` as the baseline, so the `A0`-relative ratios
are the comparable quantity.  Recomputed from `_c4_bench.csv` with
`_c5_analyze.py`'s statistics (reproduces candidate 4 §6 exactly):

| arm | WALL vs `original` | ARRANGEMENT | INTERNAL |
|---|---|---|---|
| candidate 2 | 1.38× [1.32, 1.44] | 12.7× | 5.6× |
| candidate 4 | 1.98× [1.77, 2.24] | 42.3× | 33.8× |
| **candidate 5** | **1.69× [1.52, 1.90]** | **48.2×** | **36.9×** |

The **inner-loop** reduction of candidate 5 matches or exceeds candidate 4's —
with the exact Epeck kernel throughout and none of candidate 2's rewrite.  The
end-to-end column is *not* strictly comparable: the two runs are on different
hosts, and the code neither arm touches costs 26.6 s on candidate 4's host
against 46.9 s here (`wall − (arr+fre)` for `A0`), i.e. this host is ~1.8×
slower on exactly the term that sets the Amdahl ceiling.  Read it as "candidate
5 recovers most of candidate 4's end-to-end win without the float rewrite", not
as a ranking.

### Slack alignment matters here, unlike on synthetic data

The two factors multiply as 1.24× (pipeline) × 1.36× (slack) = 1.69×.  On the
synthetic `bench_100` of §5 slack alignment is not significant at all.  The
mechanism is candidate 4 §5: real trajectories contain repeated and
near-repeated points, so an un-aligned pipeline emits far more regions than the
decider needs — on pair `13962/4238`, 2 606 regions without slack against **65**
with it, and 606 ms against 207 ms.  Synthetic random-walk curves carry no such
degeneracy, so there is nothing for the alignment to collapse.

## 5. Measured runtime — synthetic `bench_100`

Same protocol, `test_cases/bench_100` (100 synthetic pairs, n ∈ [100, 1000]),
120 s censoring (0 censored).  Committed as `_c5_bench.csv` / `_c5_analysis.txt`;
the geolife run is `_c5_bench_geo.csv` / `_c5_analysis_geo.txt`.

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

### Why the pipeline alone carries this run

Candidate 4 found the pipeline alone to be a wash (`D1/C4N` = 0.98×) with the
whole 1.44× coming from slack alignment.  On synthetic curves it is the other
way round: the pipeline alone is 1.47× and slack alignment is not significant.

Both readings are consistent, and the reason is what the pipeline replaces.  On
candidate 2's double list the arrangement stage was already down to ~115 ms over
100 pairs (X1 §3), leaving nothing for a faster emitter to win — only the search
trajectory could still move.  On `original` the Epeck arrangement *is* the
dominant cost of the inner loop, so deleting it pays directly.  What the two
data sets then decide is how much is *left* for slack alignment: on geolife the
degeneracy of real trajectories gives it a further 1.36× (§4), on synthetic
random walks nothing.

The end-to-end figure stays far below the arrangement win for the ordinary
Amdahl reason recorded in R1: the N6 loop is a minority share of the wall.

## 6. Correctness

Against `original` (exact Epeck), over the 100 pairs of each run:

| arm | max \|Δ\| geolife | max \|Δ\| synthetic | pairs over ε = 1e-7 | `C5L` bit-identical |
|---|---|---|---|---|
| `C5L` | 0.00e+00 | 0.00e+00 | 0/100 | **100/100** |
| `C5N` | 1.09e-08 | 7.13e-09 | 0/100 | — |
| `C5`  | 1.11e-08 | 1.15e-08 | 0/100 | — |

`_c5_check.sh <case_dir> [N]` runs the gates: default vs `original`, the forced
fallback of §3, and the `C5L` bit-identity check.

## 7. Operating limits

* **The fallback is dormant on the `fut_lmf` path.**  With `CUT_LIMIT = 12` and
  `MAXREGION_DP_LIMIT = 20`, `overflow = 0` on every build (checked over 20
  benchmark pairs).  The §4/§5 numbers are therefore the pipeline alone; Epeck is
  carried as a safety net, not as part of the measured path.
* **The global `n6` entry point is parity.**  There the disc set is the full
  n₁·n₂ product, every component overflows, and all the work goes to the Epeck
  fallback.  Measured over `_nsweep` n = 6…20: answers **bit-identical** to
  `original`, wall **0.84×–1.07×, median ≈ 1.00×**.  Same conclusion as
  candidate 4 §6, and the same remedy — implement §5 of the design note
  (Bron–Kerbosch + per-clique DP) before using `n6` as anything but a reference.
* `CUT_LIMIT` above `MAXREGION_DP_LIMIT` disables the pipeline entirely and
  leaves only the pre-pass overhead; see candidate 4 §7, which applies verbatim.

## 8. Reproducing

```
bash _c5_build.sh                                   # -> ~/b_candidate5
bash _build_one.sh original                         # -> ~/b_original  (baseline)
bash _c5_check.sh  test_cases/bench_small 15        # correctness gates

# geolife_100 (real data).  The curve files are NOT committed: run
# original/test_data/benchmark/fetch_and_convert_data.py, then _cp_manifest_data.sh
# to stage the manifest's files in a native-FS ~/geodata.
bash _c5_bench_geo.sh 100 3 ~/_c5/bench_geo.csv
python3 _c5_analyze.py ~/_c5/bench_geo.csv          # -> _c5_analysis_geo.txt

# synthetic bench_100 (committed data, no download needed)
bash _c5_bench.sh  test_cases/bench_100 100 3 ~/_c5/bench.csv
python3 _c5_analyze.py ~/_c5/bench.csv              # -> _c5_analysis.txt
```

Committed outputs: `_c5_bench_geo.csv` / `_c5_analysis_geo.txt` (geolife) and
`_c5_bench.csv` / `_c5_analysis.txt` (synthetic), all per-pair raw.

**Environment used for the §4 numbers** — WSL2 Ubuntu, g++ 15.2, CGAL **6.1.1**,
CMake 4.2.3, `RelWithDebInfo`.  This is *not* the g++ 13 / CGAL 5.6 toolchain of
`_x1_results/ENVIRONMENT.md`.  CGAL ≥ 6 requires C++17 while the tree requests
C++14, so `-std=c++14` was raised to `-std=c++17` — applied identically to both
arms, so the contrast is unaffected; the committed `CMakeLists.txt` is left at
C++14 to match the other arms.  Absolute times are not comparable across the two
toolchains; the ratios are the result.

**Data.**  §4 is on `geolife_100`, the same manifest and the same curve files
as every prior arm, so it is directly comparable to candidate 2 and candidate 4
through the shared `A0` baseline.  §5 is on the committed synthetic
`bench_100`; the repository's own `bench_100.out` (different machine) recorded
`original` → candidate 2 at 1.24× there.
