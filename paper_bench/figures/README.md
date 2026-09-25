# Figures

## `plot_scatter.py` — per-instance running times (the paper's Figure 6 format)

```
python3 paper_bench/figures/plot_scatter.py
```

Outputs `fig_scatter_characters.{pdf,png}` and `fig_scatter_sigspatial.{pdf,png}` (single column) and
`fig_scatter.{pdf,png}` (both panels, full width). PDF scatter layers are rasterised at 300 dpi.

Plain style, as in Figure 6 of Bringmann, Kuennemann, Nusser (ESA 2020): log-log axes, one dot per
instance, the dashed diagonal y = x, nothing else. Their axes are binary search (x) and LMF (y); here
x is LMF with the original arrangement construction (baseline) and y is LMF with the maximal-set
enumeration (proposed, `MAXREGION_EXACT=1 MAXREGION_SLACK=0`). A dot below the diagonal is an instance
on which the proposed method is faster. Within a panel both axes share one range, so the diagonal is at
45 degrees; the two panels have different ranges.

| panel | data | machine | instances |
|---|---|---|--:|
| Characters | `results/raw_characters_uci_lmf_r2.tar.gz` (the 21,000 pairs of the paper's Table 4) | server container (Xeon) | 21,000 |
| Sigspatial | `results/raw_sigspatial_lmf_r3.tar.gz` (the authors' 1,000 decider pairs, full set; r3) | user PC, WSL2 (Core Ultra 5 125H) | 998 |

Absolute times are not comparable across the two panels. The Sigspatial panel shows the 998 pairs
measured on both arms; on the other 2 the baseline exceeds 12 GB of memory and has no time (the
proposed method finishes them in 0.52 s and 1.85 s; `TABLES_uci.md` Table D). The per-instance
statistics (geometric mean, median, total-time ratio) are in Tables C and D, not in the figure.

Suggested caption: *Running time of LMF with the original arrangement construction (x) and with the
proposed maximal-set enumeration (y) on every instance; dashed: equal time. Left: the 21,000 Characters
instances of [2, Table 4]. Right: the Sigspatial decider pairs of [2] on the full 20,199-curve set,
without the 2 pairs on which the original implementation exceeded 12 GB of memory. The panels were
measured on different machines and use different axis ranges.*
