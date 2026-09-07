# Candidate 4 — Čech maximal-region enumeration

Candidate 4 is candidate 2 with the *maximal-region* stage replaced.  Everything
else (divide-and-conquer driver, kd-tree, sub-Fréchet decider, benchmarks,
`CUT_LIMIT`/`DEPTH_LIMIT` knobs) is untouched.

Design reference: `maximal_region_pipeline.md` (outside the repo).

---

## 1. What changed

| file | change |
|---|---|
| `lib/cgal_disk_arrangements/maximal_regions.h` | **new** — pipeline API |
| `lib/cgal_disk_arrangements/maximal_regions.cpp` | **new** — Stages 0–5 |
| `lib/cgal_disk_arrangements/test_maximal_regions.cpp` | **new** — §9 regression suite |
| `lib/cgal_disk_arrangements/disc_arrangement_traversal.cpp` | dispatcher, `verify` cross-check; the old path kept verbatim as `build_candidates_legacy` |
| `lib/cgal_disk_arrangements/disc_arrangement_traversal.h` | slack-aware constructor |
| `lib/cgal_disk_arrangements/CMakeLists.txt` | build `maximal_regions.cpp` |
| `CMakeLists.txt` | `test_maximal_regions` target + `add_test` |
| `src/fut_n6_algorithm.cpp` | pass `epsilon_slack` to the traversal (§5) |

## 2. The idea

The decider only ever sees a translation `t` through the index set
`S(t) = { k : |t − c_k| ≤ r }`, and it is **monotone** in `S`.  So the only
translations worth testing are those realising an inclusion-maximal `S`, and by
the nerve lemma those are exactly the inclusion-maximal simplices of the Čech
complex of the discs.  No arrangement is needed to find them:

* **Helly** — discs are convex in the plane, so `∩_{k∈S} D_k ≠ ∅` iff every
  3-subset of `S` has a common point.
* **Equal radii** — `∩_{k∈S} D_k ≠ ∅` iff `MEC(centres of S)` has radius `≤ r`,
  which for `|S| ≤ 3` is a **sqrt-free, division-free polynomial inequality**.

```
Stage 0  merge bitwise-identical centres · proximity graph · components
Stage 1  P2 (deg 2) and P3 (deg 6) predicates, no sqrt, no division
Stage 2  downward-closed DP over 2^m subsets of each component (Helly-3)
Stage 3  maximality = "no single-element extension is valid"
Stage 4  alias expansion back to global disc indices
Stage 5  witness translation = MEC centre (Welzl, incremental)
```

Consequences relative to the arrangement path:

1. **One witness per region** instead of every boundary vertex of that region.
   Measured 8.6× fewer decider calls on geolife (561 → 65 points for the same
   65 maximal families).
2. **No sqrt.**  `h = √(r² − d²/4)` is gone, so the `√ε` error amplification
   that forced `band = 1e-7` is gone; the band is `1e-12` and one-sided.
3. **No coordinate-tolerance dedup** (`tol = 1e-9`).  Regions are identified by
   their index set — exact combinatorial data.

## 3. Runtime knobs

| env | default | meaning |
|---|---|---|
| `MAXREGION` | `cech` | `cech` \| `legacy` (candidate2 verbatim) \| `verify` (run both, compare families, return the pipeline's points) |
| `MAXREGION_BAND` | `1e-12` | relative leniency on `r²`; predicates run at radius `r√(1+band)` |
| `MAXREGION_DP_LIMIT` | `20` | components larger than this go to the legacy path (hard-capped at 24, see §7) |
| `MAXREGION_SLACK` | *(from caller)* | override the predicate slack of §5; `0` disables it |
| `MAXREGION_LEGACY_SLACK` | unset | control arm: apply the §5 slack to the *arrangement* emitter too |
| `MAXREGION_VERIFY_ABORT` | unset | `abort()` on the first verify failure |

`MAXIMAL_MODE ∈ {off, mask-only, exact}` and `X3Q_DUMP` are defined in terms of
the arrangement path, so they force `legacy` and behave exactly as in
candidate 2.  `MAXREGION=legacy` reproduces candidate 2 bit-for-bit (verified:
identical answers, `m`, `K`, `builds`, `queries` on 15 geolife pairs).

New stats line at exit:

```
[maxregion-stats] calls= discs= groups= comps= max_comp= dp_masks= regions=
                  singleton= overflow= mec_fail= p3_acute= p3_sliver=
                  pre_ms= dp_ms= mec_ms=
```

In `cech` mode `[sweep-stats]` reports `before == after == #emitted points`
(nothing is filtered at the point level); the stage timings live in
`[maxregion-stats]`.

## 4. Correctness

`ctest -R maximal_regions` runs the §9 regression table:

* **Rips ≠ Čech** — equilateral triangle of side exactly `2r`: all three pairs
  touch, but the circumradius is `2r/√3 > r`, so the triple has no common point.
  An implementation that skips the Helly/P3 check emits `{0,1,2}`; the correct
  answer is the three pairs.  This is the decisive test.
* P3 boundary at `s = r√3 · (1 ∓ 0.001)`.
* **P3 slivers** — two centres nearly coincident, third far away, radius set
  1e-11 above the exact circumradius; see the apex discussion below.
* coincident centres → merge + alias expansion.
* exact tangency `d = 2r` under the closed (`≤`) convention.
* isolated discs (the `|S| = 1` case).
* DP-overflow hand-off.
* **grid oracle** — 260×260 samples over random 8-disc sets: every observed
  `S(p)` must be contained in some emitted region, the emitted family must be an
  antichain, and every witness must lie in every disc of its own region.

Cross-check against the arrangement path (`MAXREGION=verify`, 486 builds on
geolife pair 13962/4238): **`cech_only = 0`, `legacy_only = 0`** — the two
emitters induce exactly the same family of maximal index sets.

> Note on comparing the two: the raw survivor sets are *not* comparable.  Every
> arrangement vertex lies exactly on two circles, so `mask_hi ≠ mask_lo` at ~90%
> of them, and `filter_maximal`'s conservative hi/lo domination rule leaves a
> large non-maximal tail (21114 points spanning 2606 distinct maximal families in
> the run above).  `verify` therefore puts both sides through the same plain
> closed-disc mask and the same plain subset filter.

### P3's apex must be the largest angle

`|cross| = |u|·|v|·sin θ` is a 2×2 determinant of two terms of size `|u|·|v|`, so
its relative error is ≈ `ε/sin θ`, and `R² ∝ 1/cross²` doubles that.  An earlier
revision evaluated `cross` at a fixed argument position and justified it with
"an acute triangle has every angle < 90°, hence sin ≥ √3/2" — **which is false**:
that bound holds for the *largest* angle, and an acute triangle's smallest angle
is unbounded ((89°, 89°, 2°) is acute).  Measured against an exact rational
oracle, relative error of the implied `4R²` over random slivers:

| sin θ at the apex | fixed argument | largest angle |
|---|---|---|
| 1e-3 | 2.8e-13 | 7.5e-16 |
| 1e-4 | 2.5e-12 | 7.4e-16 |
| 1e-5 | 2.9e-11 | 6.6e-16 |

So the fixed-argument form escapes `band = 1e-12` below sin θ ≈ 1e-4, and the
error direction is the harmful one: rejecting a valid triple deletes every
superset of it through the Helly recursion, so a genuine maximal region is
**lost**, not merely duplicated.  Worse, the bad shape — two centres nearly
coincident with the third far away — is precisely what `curve1[i] − curve2[j]`
produces from near-repeated curve points, which the *bitwise* coincidence merge
does not collapse.  `pred3` now takes the apex opposite the longest side, where
the acute branch guarantees sin θ ≥ √3/2; `[P3 sliver]` in the regression suite
carries two exact-oracle witnesses (one at geolife's working scale) that fail on
the old form and pass on the new.

Exposure in practice was nevertheless **zero**: `p3_sliver = 0` out of 150 652
acute evaluations over 38 geolife pairs, and 0 on the full-precision synthetic
sets.  Geolife coordinates are quantised at ~1e-5 while the radii are ~5e-2, so
sin θ is bounded below by ~2e-4 by the data itself.  Answers, builds and decider
queries are bit-identical before and after the fix on all 15 spot-check pairs —
**the measurements in §6 are unaffected.**  The fix is prophylactic, and the
`p3_sliver` counter exists so the assumption stays checkable on new data.

### One-sided band, deliberately

The first revision used two tables (strict/lenient) mirroring the arrangement
path's hi/lo guard.  At an exact tangency `d = 2r` the pair is lenient-valid but
not strict-valid, so the strict extension test failed to prune the singletons and
`{0}`, `{1}`, `{0,1}` were all emitted — redundant output and a broken antichain.
One lenient table is both simpler and safe: a genuinely feasible set is
lenient-valid, hence contained in some emitted set, whose witness misses that
set's discs by at most `r·band/2 ≈ 5e-13·r`.

## 5. The predicate slack (why `fut_n6_algorithm.cpp` changed)

`N6Alg::lessThan` builds discs of radius `distance`, then asks the sub-Fréchet
decider about `distance + epsilon_slack`.  The pair set the decider actually sees
is therefore the one at the **larger** radius.

The arrangement path hides this by accident: it probes many boundary vertices per
region, and a vertex sitting exactly on two circles of radius `distance` can see,
at `distance + epsilon_slack`, pairs the region's interior cannot.  One witness
per region removes the accident and makes candidate 4 marginally *stricter*, which
measured as a systematic `+5e-9` on the answer (≈ `epsilon_slack`, well inside the
`ε = 1e-7` contract) — but it also pushed the divide-and-conquer search down more
boxes, inflating the build count 1.6–2.9× and eating the per-build win:

| pair | c2 wall | c4, no slack | c4, slack-aligned |
|---|---|---|---|
| 3110/11704   | 325 ms (238 builds) | 406 ms (398) | **66 ms (29)** |
| 15818/12700  | 181 ms (177 builds) | 385 ms (508) | **43 ms (26)** |
| 10725/2829   | 179 ms (144 builds) | 224 ms (282) | **47 ms (26)** |
| 14551/3143   | 379 ms (155 builds) | 413 ms (288) | **219 ms (26)** |
| 13962/4238   | 504 ms (486 builds) | 503 ms (486) | **131 ms (25)** |

`epsilon_slack` is now passed to the traversal and the pipeline enumerates at
`distance + epsilon_slack`.  The arrangement path ignores the parameter, which is
what keeps `MAXREGION=legacy` bit-identical to candidate 2, and
`MAXREGION_LEGACY_SLACK` exists as the control arm that isolates the slack factor.

§6 quantifies both factors over 100 pairs.  Briefly: slack alignment cuts the
*number* of arrangement builds (the search converges sooner), the pipeline cuts
the *cost per build*, and neither alone is reliably a win.  `_c4_factor.sh`
reproduces the per-pair view above on demand.

## 6. Measured runtime

`_c4_bench.sh` + `_c4_analyze.py`, geolife_100, **100 pairs × 5 arms × 3 reps**,
arm order reshuffled every rep, min per (pair, arm), 60 s censoring (0 censored).
Statistics are per-pair log-ratios → geomean with a 10 000-resample bootstrap
95% CI, the same protocol as `_factor_perpair.sh`/`_factor_analyze.py`.

| contrast | WALL | ARRANGEMENT | INTERNAL (arr+fre) |
|---|---|---|---|
| **candidate2 → candidate4** | **1.44× [1.30, 1.60]** | 3.33× [2.62, 4.25] | 6.03× [4.66, 7.85] |
| original → candidate4 | 1.98× [1.77, 2.24] | 42.3× | 33.8× |
| *(prior work)* original → candidate2 | 1.38× [1.32, 1.44] | 12.7× | 5.61× |

Faster on **93/100** pairs; median 1.11×, p90 3.71×.  Absolute wall over all 100
pairs: original 39.5 s → candidate2 28.4 s → **candidate4 21.8 s**.  Decider
calls **689 354 → 54 666 (12.6×)**; arrangement builds 26 488 → 18 515.

Answers vs candidate 2: max |Δ| **1.1e-8**, **0/100** pairs exceed ε = 1e-7.
(Candidate 2 itself agrees with the exact-arithmetic original to 4.4e-9.)

### Factor decomposition (WALL)

| factor | geomean |
|---|---|
| slack alignment alone (`D1/C4S`) | 1.18× [1.08, 1.29] |
| **pipeline alone** (`D1/C4N`) | **0.98× [0.91, 1.03]** — a wash |
| pipeline given slack (`C4S/C4`) | 1.22× [1.15, 1.30] |
| product | 1.18 × 1.22 = **1.44** ✓ |

This *reproduces* the earlier X1 result (maximal reduction alone is an
end-to-end wash, `M_D` ≈ 0.98×) and the R1 Amdahl ceiling — the inner loop is
too small a share of the wall for a 2.9× internal win to show.  What unlocks it
is the slack alignment, which moves the search trajectory itself; R1 held that
trajectory fixed, which is why it read 1.15× as a hard ceiling.

> **Caveat on `C4S`.**  The slack-only arm exceeds ε on 5/100 pairs (max 3.3e-6),
> always *upward*.  Running the arrangement emitter at the inflated radius puts
> its vertices exactly on the inflated circles, where the `sqrt` error (~1.5e-8·r)
> is larger than the slack (9e-9), so the decider can miss the very pair the
> vertex was built for.  `C4S` is diagnostic-only (off by default) and its 1.18×
> should be read as an upper bound.  `C4` does not have this problem — an MEC
> centre sits away from the boundary by construction.

### Global N6 path (`_c4_nsweep.sh`)

On the standalone `n6` entry point the disc set is the full `n₁·n₂` product, so
every component overflows the `2^m` DP (`max_comp` up to 196, `overflow` = one
per build, `regions` = 0) and all the work goes to the legacy fall-back.
Result: **parity, 0.92×–1.07×, median ≈0.98×** over n = 6…28 — no gain, ~2–3%
residual overhead from the Stage-0 pre-pass.  This is the price of not
implementing §5 (below), and it is the regime where implementing it would pay.

> An earlier revision measured 0.85×–1.00× here.  The gap was a fall-back
> defect, not the pipeline: `ply` was being computed for every fall-back point
> with an `O(#points · n)` mask pass (plus a heap allocation per point) even
> though `ply` is only ever read under `X3Q_DUMP`, which forces the legacy path.
> Fixed, together with a needless disc-vector copy when the whole set is one
> oversized component.

## 7. Operating limits — `CUT_LIMIT` vs `MAXREGION_DP_LIMIT`

The DP is Θ(m·2^m) in the component size while the arrangement fall-back is
polynomial, so the driver's `CUT_LIMIT` (which caps the component size in the
divide-and-conquer path) and `MAXREGION_DP_LIMIT` have to be read together.
Measured on geolife 13962/4238, 25 builds:

| `CUT_LIMIT` | `max_comp` | `dp_masks` | `overflow` | `dp_ms` | effect |
|---|---|---|---|---|---|
| 12 (default) | 8 | 6 400 | 0 | ~0.0 | pipeline active, DP negligible |
| 24 | 20 | 26 214 400 | 0 | **107.8** | pipeline active, but the DP now dominates the stage |
| 48 | 48 | 0 | **143** | 0.0 | **pipeline entirely inactive** — everything falls back |

Two silent traps, both now instrumented:

* `CUT_LIMIT` **above** `MAXREGION_DP_LIMIT` disables candidate 4 completely and
  leaves only the pre-pass overhead.  The shipped X3-b sweep (`_x3b_run.sh`) uses
  `CUT_LIMIT` 12/24/48, so this is reachable with existing scripts.  A one-shot
  `[maxregion]` warning now goes to stderr on the first overflow, and
  `overflow=` / `regions=` in `[maxregion-stats]` show it per run.
* Raising `MAXREGION_DP_LIMIT` to match makes the DP the bottleneck instead —
  each step of `m` doubles it.  The knob is hard-capped at 24; beyond that the
  component is handed back, which is always exact, just not accelerated.

`MAXREGION=verify` additionally reports `vacuous=`: builds where every component
overflowed, so the "pipeline" output *is* the arrangement output and the
agreement check compares legacy against itself.  Trust an agreement figure only
when `vacuous = 0`.

## 8. Not implemented

* **§5 of the design note** (Bron–Kerbosch + per-clique DP + extremal-set filter)
  for components beyond the `2^m` DP.  Instead, oversized components are handed
  back and solved by the legacy arrangement path restricted to that component —
  exact, because components are more than `2r` apart and share no point.  With
  `CUT_LIMIT = 12` this branch never fires in the divide-and-conquer path
  (`overflow = 0` on every geolife build), so it costs the headline result
  nothing; on the standalone N6 entry point it fires on every build and the
  pipeline degenerates to parity (§6).  Implement §5 before raising `CUT_LIMIT`
  past ~24, or before using the `n6` algorithm as anything but a reference.
* **Filtered exact predicates** (§3.3): `P2`/`P3` are evaluated in plain double.
  They are polynomial and well-conditioned (only pairwise differences enter), and
  the grid oracle plus the `verify` runs show no disagreement, but a
  double-double fallback inside the band is the principled next step.
* **`epsilon_slack` for the arrangement path.**  The slack misalignment of §5 is
  candidate 2's too; it is simply invisible there.  Fixing it *inside* the
  arrangement emitter is not as simple as inflating the radius (see the `C4S`
  caveat in §6) — the vertices would have to be nudged inward off the circles.
* Raising `CUT_LIMIT` (§10.4) — X3-b already found no break-even up to 48 on the
  arrangement path; worth re-running now that the per-build cost profile changed.
