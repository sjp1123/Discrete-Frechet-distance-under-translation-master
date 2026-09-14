# paper_bench — `original` vs `candidate5` on the paper's data sets

In-process driver (`paper_bench.cpp`) compiled **once per arm against that arm's own
sources** (`CMakeLists.txt` pulls the arm in as a subdirectory and links its `trans`
objects + `disc_arrangement_traversal`, i.e. the same code path and flags as the arm's
`fut_paper_experiments`).  Timing is the paper harness's: `high_resolution_clock`
around `FrechetUnderTranslation` construction + call, default parameters
(ε = 1e-7, depth 40, cut limit 12), `MEASUREMENT` counters/timers read per query.

```
bash paper_bench/build.sh              # -> ~/b_pb_original/paper_bench, ~/b_pb_candidate5/paper_bench
python3 paper_bench/gen_pairs.py       # seeded pair lists (already committed in queries/)
bash paper_bench/run_bench.sh 3        # gen decider sets (original arm) + 3 reps, alternating arm order
python3 paper_bench/analyze.py         # -> results/RESULTS.md, results/*_merged.csv
```

## What is measured

* **Value computation (LMF)** — `calcDistance2`, the paper's main algorithm, per pair:
  wall, black-box calls, the n6 arrangement/decider timers, the phase-2 timers.
* **Decision problem** — `lessThan(δ)`, the paper's decider benchmark (§5.2 protocol,
  `fut_create_benchmark_decider.cpp` formulas): for each pair δ* is computed by the
  **original** arm at precision 1e-7, then 23 query sets δ = (δ*−ε)(1−2^l), l=−10…−1
  (answer must be *no*) and δ = (δ*+ε)(1+2^l), l=−10…2 (answer must be *yes*).

## Data sets (see `../paper_data/README.md`)

| set | curves | pairs | note |
|---|--:|--:|---|
| `characters_all` | 2 858 | 2 000 | random pairs over all characters |
| `characters_same` | 1 429 (labelled half) | 1 000 | same-letter pairs |
| `sigspatial_subset` | **101** | 1 000 | the GIS Cup sample only — full 20 199-curve set was unreachable |

## Correctness gates

* LMF: |value(original) − value(candidate5)| ≤ ε = 1e-7 on every pair (the tree's
  convention; both are ε-approximations of the same quantity, bit-identity is not expected).
* Decider: every answer equals the expected one for **both** arms, and the arms agree.

## Files

* `queries/*_pairs.txt` — seeded pair lists (`gen_pairs.py`, seeds inside);
  `queries/*_decider_*` — generated decider sets + `*_computed_distances.check` (δ* per pair).
* `results/*_r<rep>.csv` — raw per-query rows per arm and rep (in `results/raw.tar.gz`);
  `results/*_merged.csv` — one row per query with both arms' answers and every rep's time;
  `results/RESULTS.md` — the tables.
