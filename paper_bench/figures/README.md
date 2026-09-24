# Figures

## `plot_scatter.py` — per-instance running times (the paper's Figure 6 format)

```
python3 paper_bench/figures/plot_scatter.py
```

Outputs `fig_scatter_characters.{pdf,png}` and `fig_scatter_sigspatial.{pdf,png}` (single column,
3.3 in wide after the tight bounding box) and `fig_scatter.{pdf,png}` (both panels, 6.9 in). PDF
scatter layers are rasterised at 300 dpi.

Figure 6 of Bringmann, Kuennemann, Nusser (ESA 2020) plots, for each of the 21,000 Characters
instances, the running time of LMF (y) against binary search (x) on log-log axes with the diagonal
y = x. Here the x axis is LMF with the original arrangement construction (baseline) and the y axis
LMF with the maximal-set enumeration (proposed, `MAXREGION_EXACT=1 MAXREGION_SLACK=0`); a dot below
the dashed diagonal is an instance on which the proposed method is faster. Within a panel both axes
share one range so the diagonal is at 45 degrees; dotted lines mark 10x and 100x faster, labelled in
the right margin. The two panels have different ranges (4.4 and 7.1 decades), so equal speed-ups sit
at different distances from the diagonal in the two panels.

| panel | data | machine | instances |
|---|---|---|--:|
| Characters | `results/raw_characters_uci_lmf_r2.tar.gz` (the 21,000 pairs of the paper's Table 4) | server container (Xeon) | 21,000 |
| Sigspatial | `results/raw_sigspatial_lmf_r3.tar.gz` (the authors' 1,000 decider pairs, full set; r3) | user PC, WSL2 (Core Ultra 5 125H) | 998 + 2 |

Absolute times are not comparable across the two panels. On two Sigspatial pairs the baseline is
OOM-killed even under a 12 GB limit; they are drawn as orange triangles at the time of the kill
(311.7 s, 203.3 s), read from `sigspatial_lmf_original_oom_12gb.txt` in the same archive. Those kill
times come from an earlier run with the old clock (`experiment_log` 6.9), not from r3, and are an
approximate lower bound on the baseline's time. The summary text in each panel (instances faster,
geometric mean and median of the per-instance ratios, total-time ratio) covers the instances measured
on both arms: all 21,000 for Characters, the 998 dots for Sigspatial (the two triangles excluded).
The Sigspatial total-time ratio is marked "tail-driven" because its four slowest baseline instances
make up 86 % of the baseline total (`TABLES_uci.md` Table D).

Suggested caption: *Running time of LMF with the original arrangement construction (x) and with the
proposed maximal-set enumeration (y) on every instance; dashed: equal time, dotted: 10x and 100x faster.
Left: the 21,000 Characters instances of [2, Table 4] (server). Right: the 1,000 Sigspatial decider pairs
of [2] on the full 20,199-curve set (laptop; absolute times are not comparable with the left panel);
triangles: the original implementation exceeded 12 GB of memory and was stopped, placed at the time of
the stop. The two panels use different axis ranges.*

Colours are the reference palette's categorical slots 1-2 (`#2a78d6`, `#eb6834`), validated all-pairs
for colour-vision deficiency; the two series also differ in marker shape.
