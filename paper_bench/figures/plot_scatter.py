#!/usr/bin/env python3
"""Per-instance running times, baseline vs proposed, in the format of Figure 6 of
Bringmann, Kuennemann, Nusser (ESA 2020): a log-log scatter with one dot per instance and
the dashed diagonal y = x.  Their x axis is binary search and their y axis LMF; here the x
axis is LMF with the original arrangement construction (baseline) and the y axis LMF with
the maximal-set enumeration (proposed, MAXREGION_EXACT=1 MAXREGION_SLACK=0).  Dots below
the diagonal are instances on which the proposed method is faster.  Within a panel both axes
share one range so that the diagonal is at 45 degrees; dotted lines mark 10x and 100x,
labelled in the right margin.  The two panels have different ranges.

Data (value computation, `calcDistance2`, one measurement per instance):
  Characters  results/raw_characters_uci_lmf_r2.tar.gz  the authors' 21,000 characters_full
              pairs (the instances of the paper's Table 4), both arms back-to-back (container)
  Sigspatial  results/raw_sigspatial_lmf_r3.tar.gz  the authors' 1,000 Sigspatial decider pairs
              (full 20,199-curve set, user PC WSL2, r3: steady_clock, the arms of each pair
              back-to-back on one vCPU; run_wsl_paired.sh).  The baseline is OOM-killed on 2 pairs
              even under a 12 GB limit; they are drawn as right-pointing triangles at the time of the
              kill, read from sigspatial_lmf_original_oom_12gb.txt in the same archive (an earlier run
              with the old clock; an approximate lower bound on the baseline's time).  The summary text
              covers the pairs measured on both arms (998), not the triangles.

    python3 paper_bench/figures/plot_scatter.py
      -> fig_scatter_characters.{pdf,png}, fig_scatter_sigspatial.{pdf,png} (single column, 3.35 in wide)
         fig_scatter.{pdf,png} (both panels side by side, full width)
"""
import csv
import io
import math
import os
import statistics
import tarfile

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.lines import Line2D

HERE = os.path.dirname(os.path.abspath(__file__))
RES = os.path.normpath(os.path.join(HERE, "..", "results"))

# Reference palette (dataviz skill, light mode): categorical slots 1-2, validated all-pairs.
SERIES_1 = "#2a78d6"      # instances
SERIES_2 = "#eb6834"      # baseline out of memory
SURFACE = "#ffffff"
INK = "#0b0b0b"           # primary text
INK_2 = "#52514e"         # secondary text
REF = "#8a8984"           # reference lines
GRID = "#e6e5e1"          # hairline grid
SINGLE_WIDTH_IN = 3.35    # target width of the single-column figures after bbox_inches="tight"

plt.rcParams.update({
    "font.size": 8, "axes.labelsize": 8, "xtick.labelsize": 7, "ytick.labelsize": 7,
    "legend.fontsize": 7, "axes.edgecolor": INK_2, "axes.labelcolor": INK,
    "xtick.color": INK_2, "ytick.color": INK_2, "axes.linewidth": 0.8,
    "savefig.facecolor": SURFACE, "figure.facecolor": SURFACE, "pdf.fonttype": 42,
})


def read_member(tar_name, member):
    with tarfile.open(os.path.join(RES, tar_name)) as tf:
        return list(csv.DictReader(io.TextIOWrapper(tf.extractfile(member), encoding="utf-8")))


def oom_kills_ms():
    """{(file1, file2): kill time in ms} of the baseline's 12 GB run on the two OOM pairs."""
    with tarfile.open(os.path.join(RES, "raw_sigspatial_lmf_r3.tar.gz")) as tf:
        text = tf.extractfile("sigspatial_lmf_original_oom_12gb.txt").read().decode()
    rows = [l.split() for l in text.splitlines() if l and not l.startswith("#")]
    return {(r[1], r[2]): float(r[4]) * 1000.0 for r in rows}


def load_characters():
    o = read_member("raw_characters_uci_lmf_r2.tar.gz", "characters_uci_lmf_original_r2.csv")
    p = read_member("raw_characters_uci_lmf_r2.tar.gz", "characters_uci_lmf_candidate5_exact_r2.csv")
    assert len(o) == len(p) == 21000, (len(o), len(p))
    # rows are in the same (harness) order; the list contains repeated pairs, so match by index
    assert all((a["file1"], a["file2"]) == (b["file1"], b["file2"]) for a, b in zip(o, p))
    return [float(r["time_ms"]) for r in o], [float(r["time_ms"]) for r in p], [], []


def load_sigspatial():
    o = read_member("raw_sigspatial_lmf_r3.tar.gz", "sigspatial_lmf_original_r3.csv")
    p = read_member("raw_sigspatial_lmf_r3.tar.gz", "sigspatial_lmf_candidate5_exact_r3.csv")
    O = {(r["file1"], r["file2"]): float(r["time_ms"]) for r in o}
    P = {(r["file1"], r["file2"]): float(r["time_ms"]) for r in p}
    assert len(P) == 1000 and len(O) == 998 and set(O) <= set(P), (len(P), len(O))
    assert min(O.values()) > 0 and min(P.values()) > 0   # r1 (system_clock) had negative times
    kills = oom_kills_ms()
    missing = sorted(set(P) - set(O))
    assert set(missing) == set(kills), missing
    keys = [k for k in P if k in O]
    return ([O[k] for k in keys], [P[k] for k in keys],
            [kills[k] for k in missing], [P[k] for k in missing])


def stats(x, y):
    ratios = [a / b for a, b in zip(x, y)]
    top4 = sum(sorted(x, reverse=True)[:4]) / sum(x)
    return dict(n=len(x), faster=sum(r > 1 for r in ratios), sum_ratio=sum(x) / sum(y),
                geomean=math.exp(sum(math.log(r) for r in ratios) / len(ratios)),
                median=statistics.median(ratios), top4_share=top4)


def limits(values):
    # equal limits on both axes (the diagonal stays at 45 degrees), a factor 1.6 beyond the data
    return min(values) / 1.6, max(values) * 1.6


def draw(ax, name, x, y, ox, oy, dot_size, alpha):
    s = stats(x, y)
    lo, hi = limits(x + y + ox + oy)
    ax.set_xscale("log"); ax.set_yscale("log")
    ax.set_xlim(lo, hi); ax.set_ylim(lo, hi); ax.set_aspect("equal", adjustable="box")
    ax.grid(True, which="major", color=GRID, linewidth=0.6, zorder=0)
    ax.set_axisbelow(True)

    # reference lines: same time (dashed, as in the paper), 10x and 100x faster (dotted)
    ax.plot([lo, hi], [lo, hi], linestyle=(0, (5, 3)), color=REF, linewidth=1.1, zorder=1)
    for k in (10, 100):
        ax.plot([lo, hi], [lo / k, hi / k], linestyle=(0, (1, 2)), color=REF, linewidth=0.8, zorder=1)
    # direct labels in the right margin, where each line meets the right edge (y = hi / k):
    # outside the data, so they collide with no dot, triangle, legend or summary text
    span = math.log(hi) - math.log(lo)
    for k, text in ((1, "1×"), (10, "10×"), (100, "100×")):
        frac = (math.log(hi / k) - math.log(lo)) / span
        if frac > 0.04:
            ax.text(1.015, frac, text, transform=ax.transAxes, fontsize=6.5, color=INK_2, ha="left", va="center",
                    clip_on=False)

    ax.scatter(x, y, s=dot_size, c=SERIES_1, alpha=alpha, linewidths=0, rasterized=True, zorder=2)
    handles = []
    if ox:
        ax.scatter(ox, oy, s=42, marker=">", c=SERIES_2, edgecolors=SURFACE, linewidths=1.2, zorder=4)
        handles = [Line2D([], [], linestyle="", marker="o", markersize=4, markerfacecolor=SERIES_1, alpha=max(alpha, 0.6),
                          markeredgewidth=0, label=f"instance ({s['n']:,} measured on both arms)"),
                   Line2D([], [], linestyle="", marker=">", markersize=6, markerfacecolor=SERIES_2,
                          markeredgecolor=SURFACE, label="baseline out of memory\n(x = time at kill, a lower bound)")]
        # direct label to the left of the lower triangle, inside the axes, in the empty region under the 100x line
        k_low = oy.index(min(oy))
        # white backing so the dotted 100x line does not run through the letters
        ax.annotate("baseline killed\nat 12 GB", xy=(ox[k_low], oy[k_low]), xytext=(-4, -22),
                    textcoords="offset points", ha="right", va="top", fontsize=6.5, color=INK_2, zorder=6,
                    bbox=dict(boxstyle="square,pad=0.15", facecolor=SURFACE, edgecolor="none"),
                    arrowprops=dict(arrowstyle="-", color=INK_2, linewidth=0.6, shrinkA=1, shrinkB=4))

    ax.set_xlabel("Baseline LMF (ms)")
    ax.set_ylabel("Proposed LMF (ms)")
    ax.set_title(name, fontsize=8.5, color=INK, loc="left", pad=4)
    tail = "\n(tail-driven)" if s["top4_share"] > 0.5 else ""   # own line: keeps the text clear of the diagonal
    summary = (f"faster on {s['faster']:,} of {s['n']:,}\n"
               f"geometric mean {s['geomean']:.2f}×\n"
               f"median {s['median']:.2f}×\n"
               f"total-time ratio {s['sum_ratio']:.1f}×{tail}")
    # upper left: the region above the diagonal holds no data in either panel (below the legend if any);
    # kept narrow so that it stays clear of the diagonal
    ax.text(0.03, 0.78 if handles else 0.97, summary, transform=ax.transAxes, fontsize=6.5, color=INK_2, ha="left", va="top",
            linespacing=1.35, zorder=6)
    if handles:
        ax.legend(handles=handles, loc="upper left", frameon=False, handletextpad=0.3, borderaxespad=0.4,
                  labelcolor=INK_2)
    return s


def save(fig, stem):
    for ext in ("pdf", "png"):
        fig.savefig(os.path.join(HERE, f"{stem}.{ext}"), dpi=300, bbox_inches="tight")
    plt.close(fig)


def single(name, data, size, alpha, stem):
    # bbox_inches="tight" adds the right-margin labels and the y label to the width, so size the canvas
    # such that the tight box comes out at SINGLE_WIDTH_IN
    target = SINGLE_WIDTH_IN - 2 * plt.rcParams["savefig.pad_inches"]   # savefig pads the tight box on both sides
    w = SINGLE_WIDTH_IN
    for _ in range(5):
        fig, ax = plt.subplots(figsize=(w, w))
        s = draw(ax, name, *data, size, alpha)
        fig.canvas.draw()
        bb = fig.get_tightbbox(fig.canvas.get_renderer())
        if abs(bb.width - target) < 0.005: break
        plt.close(fig); w *= target / bb.width
    save(fig, stem)
    return s


def main():
    panels = [("Characters, 21,000 pairs", load_characters(), 3.0, 0.08, "fig_scatter_characters"),
              ("Sigspatial, 1,000 pairs", load_sigspatial(), 7.0, 0.45, "fig_scatter_sigspatial")]
    out = {name: single(name, data, size, alpha, stem) for name, data, size, alpha, stem in panels}
    fig, axes = plt.subplots(1, 2, figsize=(7.0, 3.55))
    for ax, (name, data, size, alpha, _) in zip(axes, panels):
        draw(ax, name, *data, size, alpha)
    fig.tight_layout(w_pad=2.0)
    save(fig, "fig_scatter")
    for name, s in out.items():
        print(f"{name}: n={s['n']}, faster={s['faster']}, sum={s['sum_ratio']:.3f}x, "
              f"geomean={s['geomean']:.3f}x, median={s['median']:.3f}x, top-4 share of the baseline total={s['top4_share']:.3f}")


if __name__ == "__main__":
    main()
