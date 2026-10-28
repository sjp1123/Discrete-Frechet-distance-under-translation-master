# Candidate 8 — candidate7 with the base case's enumeration radius budgeted

candidate8 is candidate7 with one change: the union base case (candidate5's witnesses, then the box
family) enumerates maximal sets at radius δ + s instead of δ, where s is the part of the decider's own
slack that rounding does not need. Everything else is candidate7 bit for bit; `MAXREGION_SLACK=0`
selects candidate7's radius and reproduces candidate7's answers, values and black-box calls exactly.
The change is marked `FIX (candidate8)` in three files:
`lib/cgal_disk_arrangements/disc_arrangement_traversal.{h,cpp}` and `src/fut_n6_algorithm.{h,cpp}`.

## 1. Why: candidate7 was slower than the authors' code on about 1 % of the instances

In r5 (`paper_bench/r5_authors_vs_candidate7/`) candidate7 was slower than the authors' code on 905 of
21,000 Characters pairs and 75 of 1,000 Sigspatial pairs. Two causes:

- 715 and 60 of them never reach a base case in either arm (same black-box calls, zero base-case
  time). The two arms run the same code there; re-measuring them five times gives a ratio of 0.997 and
  1.013 with half of them faster. This is timing noise on 4–10 ms instances.
- 190 and 15 of them are real: candidate7 processed about 5× the boxes of the authors' code.
  - The LMF search drops a box when `max − min_dist ≤ ε/2`.
  - On these pairs the start/end lower bound sits within 1e-8 of `δ* − ε/2`, so every box is on that
    edge.
  - The authors' base case tests arrangement vertices with the decider at `δ + epsilon_slack`
    (9e-9 at the default ε), so its bisection settles up to 9e-9 below the box minimum.
  - candidate7 enumerated maximal sets at exactly δ and settled 0.2–8e-9 higher. A final `max` 1e-9
    above the edge keeps every box alive.
  - With the enumeration radius raised by the slack, candidate7 processes the authors' number of boxes
    on these pairs.

## 2. The change

`enumeration_slack(slack, decider_err)` in `disc_arrangement_traversal.cpp`:

| `MAXREGION_SLACK` | enumeration slack s |
|---|---|
| unset or `budget` (default) | `max(0, slack − decider_err − slack/9)` when the caller passes its decider rounding bound `decider_err` (union's two stages and `MAXREGION_FIX=box`); 0 otherwise, as candidate7 |
| `0` | 0 — candidate7 |
| `caller` | `slack` |
| a number v | `min(v, slack)` — candidate7 did not clamp, see 3.3 |

`decider_err` is candidate7's `N6Alg::deciderRoundingBound`, 2u(max\|p\| + R) + 6uR. It is about
1e-13 on Characters and 3.1e-9 on Sigspatial (max\|p\| ≈ 1.4e7), so s ≈ 8.0e-9 and 4.9e-9. The
margin slack/9 (= N6Alg's `epsilon_sub` at the default ε) is left for rounding the exact witness, so
that `enumerate_box` rarely needs its fallback.

## 3. Correctness

Notation: the decider of the base case accepts a translation w when its floating-point evaluation of
DFD(π, σ + w) is at most δ + η, with η = `epsilon_slack` (9e-9). Its rounding error is at most
e = `decider_err`. The driver classifies the discs of radius δ against the box B into cut (C),
containing (A) and far (F) exactly as in candidate7. Let s ∈ [0, η] be the enumeration slack and
r = δ + s.

### 3.1 YES answers

Unchanged. Every YES, from either stage, is the decider's own verdict `lessThanFixedTranslation(δ + η)`
at a concrete translation. The enumeration radius only chooses which translations are tested. So
candidate8 returns YES only where candidate7 and the authors' code may: the decider accepted the
translation at δ + η. For δ ≥ δ* that is correct. Below δ* it can happen only inside the band the
decider's own slack allows, (δ* − η − e, δ*). The authors' arrangement path and candidate7 share that
band.

### 3.2 NO answers: per-box completeness

candidate7's proof (README of candidate6, §1.1; `paper_bench/results/C7_report.md`) goes through with
r in place of δ:

1. Let p ∈ B with DFD(p) ≤ δ and T(p) = {k ∈ C : |p − c_k| ≤ δ}. Since δ ≤ r, T(p) together with
   B has the common point p at radius r. By Helly on the family X ∪ {B} (predicates Q1, Q2, P3 at
   radius r), `enumerate_box` emits a maximal X ⊇ T(p).
2. The exact minimax point of X over B has max distance at most r to the centres of X.
   `enumerate_box` keeps a witness w ∈ B only if it certifies max\|w − c_k\| ≤ r + t, with
   t = (η − s) − e. With s ≤ η this bound is r + t = δ + η − e, whatever s is. Any other region keeps
   its witness and also sends its discs to the box-arrangement fallback on the original radius δ.
3. If w was kept, the decider sees every pair of X within δ + η − e, and every pair of A within δ,
   because w ∈ B. Its evaluation is therefore at most δ + η. The pairs that p uses lie in T(p) ∪ A,
   since the discs of F miss B. So the decider accepts w.
4. If the region went to the fallback, the box arrangement on radius δ is the authors' base case, and
   it is complete per box.

So the box family is complete per box for every s ∈ [0, η]. The choice of s changes which
translations are tested and how often the fallback runs, not the guarantee. union's first stage
(candidate5's uncertified witnesses) also runs at r. It only ends a base case early with a YES the
decider confirmed; its NO is never used.

### 3.3 The clamp

For s > η, step 2 breaks: `tol = max(0, η − s) − e` stops shrinking at −e, and a witness up to
r − e = δ + s − e > δ + η − e is accepted without the fallback, although the decider may reject it.
candidate7 allowed such a value through `MAXREGION_SLACK=<v>`. candidate8 clamps every setting to η.

### 3.4 LMF values

The value is the lower end of a bisection whose test is the base case above. Every YES still comes
from a translation the decider accepted at split + η. Completeness gives YES at every split above the
box minimum. Hence the value stays within [δ* − η − e − ε/2, δ* + ε/2]. That is inside the ε = 1e-7
contract for coordinates below about 4e7, the same bound as the authors' code. candidate8's values
are at most s below candidate7's.

## 4. Evidence

`paper_bench/r6_candidate8/` (verification kit on candidate8, real-data queries at ±5e-8 of the
near-threshold values, and the r5 protocol re-run with the authors' code, candidate7 and candidate8).
