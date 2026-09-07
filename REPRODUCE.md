# Reproducing the maximal-cell-reduction experiments

All results in `_x1_results/` come from the environment recorded in
`_x1_results/ENVIRONMENT.md`. This file is the single entry point to rebuild the
binaries and regenerate the headline numbers. Seeds are fixed throughout, so the
combinatorial counts are bit-reproducible; wall-clock ratios reproduce within the
noise characterized in ENVIRONMENT.md §2 (median rep CV ≈ 4%).

## 0. Prerequisites

- WSL2 Ubuntu 24.04 (or any Linux), g++ 13, CMake 3.28, CGAL 5.6, GMP/MPFR,
  Boost 1.83, Python 3 (stdlib only — no numpy/scipy needed).
- The Geolife Trajectories 1.3 dataset under
  `original/test_data/benchmark/Geolife Trajectories 1.3/data/` (NOT committed —
  2.2 GB; obtain separately). Pair manifests are in `test_cases/geolife_100/`.
- Copy the manifest's files to a native-FS dir `~/geodata` (avoids the /mnt/c
  per-exec overhead): `_cp_manifest_data.sh`.

## 1. Build every arm

```
bash _build_one.sh original        # A0  (Epeck, DCEL, no maximal)  -> ~/b_original
bash _build_one.sh candidate2      # D0/D1 (double list; MAXIMAL_MODE=off/on)
bash _c4_build.sh                  # C4  (candidate4, + ctest)      -> ~/b_candidate4
# X2 kernel arm (B0 = original with Epick):
bash _x2_build_epick.sh            # -> ~/b_original_epick
```

Arms are selected at RUNTIME (same binary, one knob), so the factor contrasts
compare identical code paths:

| knob | arm |
|---|---|
| `MAXIMAL_MODE=off` (candidate2) | D0 |
| `MAXIMAL_MODE=on`  (candidate2, default) | D1 |
| `MAXIMAL_MODE=exact` | X3 exact enumeration (verification) |
| `MAXREGION=cech` (candidate4, default) | C4 |
| `MAXREGION=legacy` | candidate2-equivalent inside candidate4 |
| `MAXREGION=verify` | run both, cross-check families |
| `CUT_LIMIT=<n>` | arrangement_cut_limit sweep (X3-b) |

## 2. Correctness gates (run these first — fast)

```
( cd ~/b_candidate4 && ctest -R maximal_regions --output-on-failure )  # unit suite
bash _oracle_check.sh        # A0/D1/C4 vs code-independent brute-force oracle
bash _c4_check.sh 30         # answers within eps; MAXREGION=legacy==c2; verify families
```

## 3. Headline numbers

```
bash _factor_perpair.sh 100   && python3 _factor_analyze.py   # X1/X2 factor table (A0,B0,D0,D1)
bash _c4_bench.sh             && python3 _c4_analyze.py        # candidate4 (A0,D1,C4S,C4N,C4)
bash _noise.sh 10 8                                            # timing-noise characterization
```

## 4. Deeper analyses (opt-in, slower)

```
python3 _x3_gen.py "8,12,16,20" 6 ; bash _x3a_run.sh "8 12 16 20" 6 240  # exact scaling (gamma)
python3 _x3_analyze.py ; python3 _r56_analyze.py                          # X3-a / R5 / R6
bash _x3q_run.sh 12 ; python3 _r4_analyze.py                              # C_q~ply^alpha
python3 _reanalyze.py                                                     # R1 ceiling / R2 / R3
```

## 5. One command (rebuild + gates + headline)

```
bash reproduce.sh
```

## Seeds

- `_gen_wall.py`: `random.seed(7)` · `_gen_nsweep.py`: `random.seed(2026)` ·
  `_x3_gen.py`: `random.Random(1000+idx)` per pair · candidate4 MEC shuffle:
  `xorshift(0x9E3779B9 ^ n)` (fixed).

## Known limits (see ENVIRONMENT.md)

- Single host; WSL frequency governor uncontrolled → wall ratios carry ≈4% rep
  noise. Multi-machine / bare-metal re-run is the outstanding external step.
- Headline speedups are for geolife_100 / `fut_lmf` / `cut_limit=12`. The global
  `n6` path and larger `cut_limit` are parity / unimplemented (candidate4 §7).
