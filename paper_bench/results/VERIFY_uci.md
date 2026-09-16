# Identity check: paper_data/characters_uci vs the authors' instance files

`original` (precision 1e-7, `paper_bench gen`) recomputed δ* for every pair of the
authors' two check files and the values were compared with the authors' own:

| authors' file (test_data/fut_decider_benchmark_queries/) | pairs | max \|Δδ*\| | pairs over 1e-7 |
|---|--:|--:|--:|
| characters_fut_decider_computed_distances.check | 1000 | 1.207e-08 | 0 |
| characters_fut_decider_samechar_computed_distances.check | 1000 | 1.329e-08 | 0 |

Recomputed values: `characters_uci_{all,same}_recomputed_distances.check`.
Together with the per-letter index lists (`test_data/character_classification_data`,
2858/2858 agree with `consts.charlabels`) this shows the curves and their numbering
are the authors'. The differences are the binary-search tolerance of the LMF value
(≤ 2·10⁻⁸), not data differences.
