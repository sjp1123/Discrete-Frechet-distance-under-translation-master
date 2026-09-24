# paper_bench — `original` vs `candidate5` on the paper's data sets

In-process driver (`paper_bench.cpp`) compiled **once per arm against that arm's own
sources** (`CMakeLists.txt` pulls the arm in as a subdirectory and links its `trans`
objects + `disc_arrangement_traversal`, i.e. the same code path and flags as the arm's
`fut_paper_experiments`).  Timing follows the paper harness: a clock around
`FrechetUnderTranslation` construction + call, default parameters (ε = 1e-7, depth 40,
cut limit 12), `MEASUREMENT` counters/timers read per query.

**Clock.** The harness and both arms' `lib/measurement_tool/measurement_tool.h` use
`std::chrono::steady_clock` (monotonic). The authors' code used `high_resolution_clock`, which
libstdc++ defines as `system_clock`; under WSL2 that clock is stepped by several hundred ms up to
~1 s at Hyper-V time syncs, which corrupted the first WSL runs (negative instance times; `experiment_log` 6.12).
Only the measurement changed; the algorithms are untouched. The container runs predate the change
and show no clock anomaly in 331,600 rows: `raw_characters_uci_lmf.tar.gz`, `raw_characters_uci_lmf_r2.tar.gz`,
`raw_characters_uci_decider.tar.gz`, `raw_characters_uci_fpsens.tar.gz`. (`raw_characters_uci_decider_r2.tar.gz` is a
WSL run with invalid timings; `raw_characters_uci_decider_r3.tar.gz` is its valid WSL re-measurement.)

```
bash paper_bench/build.sh              # -> ~/b_pb_original/paper_bench, ~/b_pb_candidate5/paper_bench
bash paper_bench/run_uci.sh lmf        # the authors' 21,000 characters_full pairs, one measurement each
bash paper_bench/run_uci.sh decider    # the authors' 23 x 1,000 decider instances (2^l files and 4^l sets)
python3 paper_bench/analyze_uci.py     # -> results/RESULTS_uci.md; tables in results/TABLES_uci.md

# on a WSL2 / hybrid-core host: both arms back-to-back on one vCPU (r3); binaries from build.sh (or set ORIG=, C5=)
bash paper_bench/run_wsl_paired.sh lmf_sig <stream> <nstreams> <cpu>          # Sigspatial value computation, per pair
bash paper_bench/run_wsl_paired.sh decider <prefix> <data_dir> <cpu>          # 23 x 1,000 decider instances, per file
python3 paper_bench/merge_wsl_r3.py <work_dir>                               # -> results/raw_*_r3.tar.gz (re-runnable; stops rather than drop rows)
DATASET=sigspatial LMF_REP=r3 DEC_REP=r3 python3 paper_bench/analyze_uci.py  # -> results/RESULTS_sig.md
python3 paper_bench/make_tables_wsl.py     # -> TABLES_uci.md Table D (Sigspatial LMF) and Table E (decider, 3 benchmarks)
python3 paper_bench/make_repro_check.py    # -> results/REPRO_check.md (original vs the authors' outputs and the paper)
python3 paper_bench/figures/plot_scatter.py  # -> figures/fig_scatter*.{pdf,png} (the paper's Figure 6 format)
```

`run_sig.sh` is the first (r1) Sigspatial runner; its timings are superseded by r3.

This branch holds only the runs the abstract and the paper draft cite: the authors' own Characters
instances (`paper_data/characters_uci`, `queries/characters_uci_*`, `results/*_uci_*`) and the full
Sigspatial set (`paper_data/sigspatial`, `queries/sigspatial_*`, `results/*sigspatial*`).  The
mirror-order data sets and the random-pair runs live in the full branch.

## What is measured

* **Value computation (LMF)** — `calcDistance2`, the paper's main algorithm, per pair:
  time, black-box calls, the n6 arrangement/decider timers, the phase-2 timers.
* **Decision problem** — `lessThan(δ)`, the paper's decider benchmark: 23 query sets per
  benchmark, δ = (δ*−ε)(1−b^l), l=−10…−1 (answer must be *no*) and δ = (δ*+ε)(1+b^l),
  l=−10…2 (answer must be *yes*), with b = 2 in the authors' shipped files (`paperq`) and
  b = 4 in the paper's text (`paperq4`); time, black-box calls, the phase-1 stage timers.

## Data sets (see `../paper_data/README.md`)

| set | curves | pairs | note |
|---|--:|--:|---|
| `characters_uci` (LMF) | 2 858 | 21 000 | the authors' `characters_full_<s1>_<s2>.txt`, 210 files x 100 (paper Table 4) |
| `characters_uci_all` / `_same` (decider) | 2 858 | 1 000 each | the authors' decider pair files, 23 sets each |
| `sigspatial` (decider, LMF) | 20 199 | 1 000 | the authors' `sigspatial_fut_decider_*` files (full GIS Cup set, `paper_data/sigspatial`) |

## Correctness gates

* LMF: |value(original) − value(candidate5)| ≤ ε = 1e-7 on every pair (the tree's
  convention; both are ε-approximations of the same quantity, bit-identity is not expected).
* Decider: every answer equals the expected one for **both** arms, and the arms agree.

## Files

* `queries/<set>_{paperq,paperq4}_<l>_<plus|minus>.txt` — decider sets; `*_computed_distances.check` — δ* per pair.
* `results/raw_*.tar.gz` — per-instance rows per arm (`<set>_..._<arm>_<rep>.csv`). Container: Characters LMF r1 and r2
  (r2 = back-to-back re-measurement, used in Table C and the abstract), Characters decider r1. WSL (user PC): Sigspatial
  r1 and the decider r2 have invalid timings (clock steps); r3 is the paired re-measurement and the only valid WSL timing.
  `raw_sigspatial_lmf_r3.tar.gz` also holds `sigspatial_lmf_rss_r3.txt` (peak RSS) and
  `sigspatial_lmf_original_oom_12gb.txt` (the baseline's kill times on the two pairs above 12 GB).
* `results/TABLES_uci.md` — paper-format tables (A-C container, D-E WSL r3); `RESULTS_uci.md`, `RESULTS_uci_r2.md`,
  `RESULTS_sig.md` — per-set summaries; `REPRO_check.md`, `VERIFY_uci.md` — reproduction and data-identity checks.
