#!/usr/bin/env python3
"""Per-instance running times, original (x) vs candidate7 (y), r4 — same plain style as
paper_bench/figures/plot_scatter.py (Figure 6 of Bringmann, Kuennemann, Nusser, ESA 2020): log-log,
one dot per instance, the dashed diagonal y = x, nothing else.  A dot below the diagonal is an instance
on which candidate7 is faster.  Within a panel both axes share one range (diagonal at 45 degrees).

Data (value computation, calcDistance2, one measurement per instance, both panels on the same
server container, both arms back-to-back on one core):
  Characters  raw/raw_characters_lmf_r4.tar.gz  the 21,000 characters_full pairs of [BKN20] Table 4
  Sigspatial  raw/raw_sigspatial_lmf_r4.tar.gz  the 998 of the authors' 1,000 decider pairs measured on
              both arms; on the other 2 the baseline needs > 12 GB and was not run (state it in the caption)

    python3 paper_bench/r4_original_vs_candidate7/scripts/plot_scatter.py
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
FOLDER = os.path.dirname(HERE)
RAW = os.path.join(FOLDER, "raw")
DOT = "#2a78d6"          # reference palette, categorical slot 1 (as in paper_bench/figures)
DIAGONAL = "#7f7f7f"
INK, INK_2 = "#0b0b0b", "#52514e"

plt.rcParams.update({
    "font.size": 8, "axes.labelsize": 8, "xtick.labelsize": 7, "ytick.labelsize": 7,
    "axes.edgecolor": INK_2, "axes.labelcolor": INK, "xtick.color": INK_2, "ytick.color": INK_2,
    "axes.linewidth": 0.8, "savefig.facecolor": "white", "figure.facecolor": "white", "pdf.fonttype": 42,
})


def load(tar_name):
    """{(job, row): time_ms} for each arm; the Characters list repeats pairs, so key by position."""
    t = {"original": {}, "candidate7": {}}
    with tarfile.open(os.path.join(RAW, tar_name)) as tf:
        for m in sorted(tf.getmembers(), key=lambda m: m.name):
            arm = m.name.split("/")[0]
            if not m.isfile() or arm not in t or not m.name.endswith(".csv"):
                continue
            job = os.path.basename(m.name)[:-4]
            for i, r in enumerate(csv.DictReader(io.TextIOWrapper(tf.extractfile(m), encoding="utf-8"))):
                t[arm][(job, i)] = float(r["time_ms"])
    keys = sorted(set(t["original"]) & set(t["candidate7"]))
    x = [t["original"][k] for k in keys]
    y = [t["candidate7"][k] for k in keys]
    assert min(x) > 0 and min(y) > 0
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
        fig.savefig(os.path.join(FOLDER, f"{stem}.{ext}"), dpi=300, bbox_inches="tight")
    plt.close(fig)


def main():
    ch = load("raw_characters_lmf_r4.tar.gz")
    sg = load("raw_sigspatial_lmf_r4.tar.gz")
    assert len(ch[0]) == 21000, len(ch[0])
    print(f"Characters {len(ch[0]):,} instances, Sigspatial {len(sg[0]):,} instances")
    panels = [("Characters", ch, 3.0, 0.10, "fig_scatter_characters"),
              ("Sigspatial", sg, 7.0, 0.45, "fig_scatter_sigspatial")]
    for _, (x, y), size, alpha, stem in panels:
        fig, ax = plt.subplots(figsize=(3.35, 3.35))
        draw(ax, x, y, size, alpha)
        save(fig, stem)
    fig, axes = plt.subplots(1, 2, figsize=(7.0, 3.5))
    for ax, (name, (x, y), size, alpha, _) in zip(axes, panels):
        draw(ax, x, y, size, alpha)
        ax.set_title(name, fontsize=8, color=INK, loc="left", pad=4)
    fig.tight_layout(w_pad=3.0)
    save(fig, "fig_scatter")


if __name__ == "__main__":
    main()
