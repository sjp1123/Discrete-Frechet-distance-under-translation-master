# paper_data — the data sets of "When Lipschitz Walks Your Dog" (BKN, ESA 2020)

Curve files in the tree's own format (`x y` per line, `parser.cpp` drops consecutive
duplicate rows), for the `original` vs `candidate5` comparison in `paper_bench/`.
The paper (arXiv:2008.07510, §5) benchmarks on **Characters** (UCI Character
Trajectories, 2 858 curves, 120.9 vertices on average), **Sigspatial** (ACM GIS Cup
2017 sample, 20 199 curves, 247.8 vertices on average) and Geolife.

The authors' fetch/convert scripts live in their repository
(`gitlab.com/anusser/frechet_distance_under_translation`, `test_data/benchmark/`):
`fetch_and_convert_data.py` downloads `mixoutALL_shifted.mat` from UCI and
`shortest-sf.tgz` from martinwerner.de and converts them.  The session that produced
this directory had **no network access to archive.ics.uci.edu, martinwerner.de,
sigspatial.org, the MPI/TUM mirrors, archive.org, HuggingFace, Zenodo or Kaggle**
(egress policy), so the data was assembled from public GitHub/GitLab mirrors as
documented below.  Everything here is reproducible from `convert_characters.py`
and the recorded source commits / checksums.

## characters/ — complete (2 858 curves) — NOT INCLUDED in this branch (mirror-order copy; see the full branch)

| item | value |
|---|---|
| source | `github.com/ShuningZhao/Character-Trajectories`, files `trajectories_train.mat` (1 429 curves + labels) and `trajectories_xtest.mat` (1 429 curves, no labels); SHA-256 in `characters/SOURCE_SHA256.txt` |
| identity with UCI `mixoutALL_shifted.mat` | 2 858 samples, 3 × T cells (x-vel, y-vel, pen force), T ∈ [109, 205] — the ranges UCI documents; 20 letter classes with the same key order (a b c d e g h l m n o p q r s u v w y z) |
| conversion | `convert_characters.py`, a line-by-line port of the authors' `character_converter.m`: drop the force row, `B = tril(ones(l,l),-1)·A` (exclusive prefix sum of the velocities, first vertex at the origin), one `x y` row per sample |
| result | mean **120.99 vertices** per curve after duplicate removal (min 61, max 183) — the paper's 120.9 |
| numbering | `data/1.txt … 1429.txt` = mirror's train set (letters in `labels_train.txt`), `1430.txt … 2858.txt` = mirror's test set.  The mirror's order is a **shuffle** of the UCI order, so the authors' index-based query files (`test_data/fut_*_benchmark_queries/characters_*`) cannot be replayed on it; `paper_bench/gen_pairs.py` draws fresh seeded pairs instead.  The per-letter counts of the labelled half are 45–60 % of the authors' per-letter totals (`character_classification_data/`), consistent with a random split of the same set. |

## characters_uci/ — the ORIGINAL UCI file, authors' numbering (use this one)

| item | value |
|---|---|
| source | `mixoutALL_shifted.mat` from the UCI repository (MATLAB 5.0 file created 2008-03-14), supplied by the user; SHA-256 in `characters_uci/SOURCE_SHA256.txt` |
| conversion | `convert_characters_uci.py`, the authors' `character_converter.m` line by line; `data/<i>.txt`, i = 1..2858 in the file's order |
| identity check | the authors' per-letter index lists (`test_data/character_classification_data/*_dataset.txt`, 2 858 entries) agree with `consts.charlabels` on every index; the authors' `characters_fut_decider_computed_distances.check` (δ* of their 1 000 all-characters pairs) is reproduced by `original` to within 1.2e-8 (see `paper_bench/results/RESULTS_uci.md`) |
| result | 2 858 curves, mean 120.99 vertices after duplicate removal (min 61, max 183) |
| consequence | the authors' instance files apply unchanged: `fut_val_computation_benchmark_queries/characters_full_*` (21 000 LMF pairs) and `fut_decider_benchmark_queries/characters_fut_decider{,_samechar}_*` (23 × 1 000 decider instances each). `paper_bench/run_uci.sh` runs exactly those. `characters/` (mirror order) is kept only for the earlier results. |

## sigspatial_subset/ — **101 of the 20 199 curves** (subset!) — NOT INCLUDED in this branch

| item | value |
|---|---|
| source | `github.com/MemoryMmy/OCJ_srajectory_similarity` (a GIS Cup 2017 entry) ships the competition's 101-file test sample `data/files/file-000000.dat … file-000100.dat`; commit in `SOURCE_COMMIT.txt` |
| identity | same files, same format (EPSG:3857 metres, header `x y k tid`, columns 3–4 ignored by the parser) as the paper's `sigspatial/file-NNNNNN.dat`; the authors' query files reference e.g. `file-000015.dat`, which is one of these 101 |
| conversion | header line removed, exactly what the authors' `fetch_and_convert_data.py` does (`tail -n +2`) |
| result | 101 curves, mean **201.0 vertices** (min 18, max 659) vs 247.8 over the full 20 199 |
| caveat | the full `shortest-sf.tgz` (20 199 trajectories, ≈5 M points) could not be fetched from any reachable host.  Numbers on this subset are on genuine GIS Cup curves, but from 0.5 % of the set and slightly shorter on average.  To run the full set: download `https://www.martinwerner.de/files/shortest-sf.tgz`, strip the header of every `file-*.dat` into `paper_data/sigspatial/data/`, write `dataset.txt`, and point `paper_bench/gen_pairs.py` / `run_bench.sh` at it. |

## Not included

Geolife (2.2 GB, `**/Geolife Trajectories 1.3/` is git-ignored) — the tree's earlier
`geolife_100` runs are in `_c5_bench_geo.csv` / `candidate5/README.candidate5.md` §4.
