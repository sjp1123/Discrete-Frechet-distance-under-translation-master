# paper_bench — `original` vs `candidate5` on the paper's data sets

In-process driver (`paper_bench.cpp`) compiled **once per arm against that arm's own
sources** (`CMakeLists.txt` pulls the arm in as a subdirectory and links its `trans`
objects + `disc_arrangement_traversal`, i.e. the same code path and flags as the arm's
`fut_paper_experiments`).  Timing is the paper harness's: `high_resolution_clock`
around `FrechetUnderTranslation` construction + call, default parameters
(ε = 1e-7, depth 40, cut limit 12), `MEASUREMENT` counters/timers read per query.

```
bash paper_bench/build.sh              # -> ~/b_pb_original/paper_bench, ~/b_pb_candidate5/paper_bench
bash paper_bench/run_uci.sh lmf        # the authors' 21,000 characters_full pairs, one measurement each
bash paper_bench/run_uci.sh decider    # the authors' 23 x 1,000 decider instances (2^l files and 4^l sets)
python3 paper_bench/analyze_uci.py     # -> results/RESULTS_uci.md; tables in results/TABLES_uci.md
```

This branch holds only the runs the abstract cites: the authors' own Characters instances
(`paper_data/characters_uci`, `queries/characters_uci_*`, `results/*_uci_*`).  The
mirror-order data sets, the random-pair runs (`run_bench.sh`, `analyze.py`,
`results/RESULTS.md`) and the figure scripts live in the full branch.

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
| `characters_uci` (LMF) | 2 858 | 21 000 | the authors' `characters_full_<s1>_<s2>.txt`, 210 files x 100 (paper Table 4) |
| `characters_uci_all` / `_same` (decider) | 2 858 | 1 000 each | the authors' decider pair files, 23 sets each |

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
