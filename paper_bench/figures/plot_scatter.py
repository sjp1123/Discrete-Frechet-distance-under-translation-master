#!/usr/bin/env python3
"""Per-instance running times, baseline vs proposed, in the plain style of Figure 6 of
Bringmann, Kuennemann, Nusser (ESA 2020): a log-log scatter with one dot per instance and the
dashed diagonal y = x, nothing else.  Their x axis is binary search and their y axis LMF; here
the x axis is LMF with the original arrangement construction (baseline) and the y axis LMF with
the maximal-set enumeration (proposed, MAXREGION_EXACT=1 MAXREGION_SLACK=0).  A dot below the
diagonal is an instance on which the proposed method is faster.  Within a panel both axes share
one range, so the diagonal is at 45 degrees.

Data (value computation, `calcDistance2`, one measurement per instance):
  Characters  results/raw_characters_uci_lmf_r2.tar.gz  the authors' 21,000 characters_full pairs
              (the instances of the paper's Table 4), both arms back-to-back (container)
  Sigspatial  results/raw_sigspatial_lmf_r3.tar.gz  the 998 of the authors' 1,000 Sigspatial decider
              pairs measured on both arms (user PC WSL2, r3).  The other 2 pairs, on which the baseline
              exceeds 12 GB of memory, have no baseline time and are not drawn (state it in the caption).

    python3 paper_bench/figures/plot_scatter.py
      -> fig_scatter_characters.{pdf,png}, fig_scatter_sigspatial.{pdf,png} (single column, 3.35 in)
         fig_scatter.{pdf,png} (both panels side by side, full width)
"""
import csv
import io
import os
import tarfile

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

HERE = os.path.dirname(os.path.abspath(__file__))
RES = os.path.normpath(os.path.join(HERE, "..", "results"))
DOT = "#2a78d6"          # reference palette, categorical slot 1
DIAGONAL = "#7f7f7f"
INK, INK_2 = "#0b0b0b", "#52514e"

plt.rcParams.update({
    "font.size": 8, "axes.labelsize": 8, "xtick.labelsize": 7, "ytick.labelsize": 7,
    "axes.edgecolor": INK_2, "axes.labelcolor": INK, "xtick.color": INK_2, "ytick.color": INK_2,
    "axes.linewidth": 0.8, "savefig.facecolor": "white", "figure.facecolor": "white", "pdf.fonttype": 42,
})


def read_member(tar_name, member):
    with tarfile.open(os.path.join(RES, tar_name)) as tf:
        return list(csv.DictReader(io.TextIOWrapper(tf.extractfile(member), encoding="utf-8")))


def load_characters():
    o = read_member("raw_characters_uci_lmf_r2.tar.gz", "characters_uci_lmf_original_r2.csv")
    p = read_member("raw_characters_uci_lmf_r2.tar.gz", "characters_uci_lmf_candidate5_exact_r2.csv")
    assert len(o) == len(p) == 21000, (len(o), len(p))
    # rows are in the same (harness) order; the list contains repeated pairs, so match by index
    assert all((a["file1"], a["file2"]) == (b["file1"], b["file2"]) for a, b in zip(o, p))
    return [float(r["time_ms"]) for r in o], [float(r["time_ms"]) for r in p]


def load_sigspatial():
    o = read_member("raw_sigspatial_lmf_r3.tar.gz", "sigspatial_lmf_original_r3.csv")
    p = read_member("raw_sigspatial_lmf_r3.tar.gz", "sigspatial_lmf_candidate5_exact_r3.csv")
    P = {(r["file1"], r["file2"]): float(r["time_ms"]) for r in p}
    assert len(P) == 1000 and len(o) == 998
    x = [float(r["time_ms"]) for r in o]; y = [P[(r["file1"], r["file2"])] for r in o]
    assert min(x) > 0 and min(y) > 0   # r1 (system_clock) had negative times
    return x, y


def draw(ax, x, y, dot_size, alpha):
    lo, hi = min(x + y) / 1.6, max(x + y) * 1.6
    ax.set_xscale("log"); ax.set_yscale("log")
    ax.set_xlim(lo, hi); ax.set_ylim(lo, hi); ax.set_aspect("equal", adjustable="box")
    ax.plot([lo, hi], [lo, hi], linestyle=(0, (5, 3)), color=DIAGONAL, linewidth=1.2, zorder=1)
    ax.scatter(x, y, s=dot_size, c=DOT, alpha=alpha, linewidths=0, rasterized=True, zorder=2)
    ax.set_xlabel("Baseline LMF (ms)")
    ax.set_ylabel("Proposed LMF (ms)")


def save(fig, stem):
    for ext in ("pdf", "png"):
        fig.savefig(os.path.join(HERE, f"{stem}.{ext}"), dpi=300, bbox_inches="tight")
    plt.close(fig)


def main():
    panels = [("Characters", load_characters(), 3.0, 0.10, "fig_scatter_characters"),
              ("Sigspatial", load_sigspatial(), 7.0, 0.45, "fig_scatter_sigspatial")]
    for _, (x, y), size, alpha, stem in panels:
        fig, ax = plt.subplots(figsize=(3.35, 3.35))
        draw(ax, x, y, size, alpha)
        save(fig, stem)
    # two panels: each carries only its data set's name, like the benchmark labels of the paper's Figure 4
    fig, axes = plt.subplots(1, 2, figsize=(7.0, 3.5))
    for ax, (name, (x, y), size, alpha, _) in zip(axes, panels):
        draw(ax, x, y, size, alpha)
        ax.set_title(name, fontsize=8, color=INK, loc="left", pad=4)
    fig.tight_layout(w_pad=3.0)
    save(fig, "fig_scatter")


if __name__ == "__main__":
    main()
