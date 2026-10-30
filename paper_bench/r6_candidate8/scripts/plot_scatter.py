#!/usr/bin/env python3
"""r6 scatter plots in the style of r5 (plain [BKN20] Figure 6 style): log-log, one dot per instance, dashed y = x,
both axes of a panel share one range.  Source: raw/r6_timing_3arm.tar.xz (run 1, the three arms back-to-back on one core).

  fig_scatter_characters.{pdf,png}   x = authors' LMF, y = candidate8, the 21,000 Characters pairs (single column, 3.35 in)
  fig_scatter_sigspatial.{pdf,png}   the same for all 1,000 Sigspatial decider pairs
  fig_scatter.{pdf,png}              both panels (full width)
  fig_scatter_c7_c8.{pdf,png}        x = candidate7, y = candidate8, both panels

    python3 paper_bench/r6_candidate8/scripts/plot_scatter.py
"""
import csv, io, os, tarfile
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

HERE = os.path.dirname(os.path.abspath(__file__)); FOLDER = os.path.dirname(HERE); RAW = os.path.join(FOLDER, "raw")
DOT, DIAGONAL, INK, INK_2 = "#2a78d6", "#7f7f7f", "#0b0b0b", "#52514e"
plt.rcParams.update({"font.size": 8, "axes.labelsize": 8, "xtick.labelsize": 7, "ytick.labelsize": 7,
                     "axes.edgecolor": INK_2, "axes.labelcolor": INK, "xtick.color": INK_2, "ytick.color": INK_2,
                     "axes.linewidth": 0.8, "savefig.facecolor": "white", "figure.facecolor": "white", "pdf.fonttype": 42})
LABEL = {"gitlab": "Authors' LMF (ms)", "candidate7": "candidate7 LMF (ms)", "candidate8": "Proposed LMF (ms)"}


def load_all():
    t = {}
    with tarfile.open(os.path.join(RAW, "r6_timing_3arm.tar.xz")) as tf:
        for m in sorted(tf.getmembers(), key=lambda m: m.name):
            parts = m.name.split("/")
            if not m.isfile() or len(parts) != 3 or not parts[2].endswith(".csv") or parts[0] not in ("chars", "sig"):
                continue
            for i, r in enumerate(csv.DictReader(io.TextIOWrapper(tf.extractfile(m), encoding="utf-8"))):
                t.setdefault(parts[0], {}).setdefault(parts[1], {})[(parts[2][:-4], i)] = float(r["time_ms"])
    return t


def xy(t, bench, xa, ya):
    keys = sorted(set(t[bench][xa]) & set(t[bench][ya]))
    return [t[bench][xa][k] for k in keys], [t[bench][ya][k] for k in keys]


def draw(ax, x, y, size, alpha, xa, ya):
    lo, hi = min(x + y) / 1.6, max(x + y) * 1.6
    ax.set_xscale("log"); ax.set_yscale("log"); ax.set_xlim(lo, hi); ax.set_ylim(lo, hi); ax.set_aspect("equal", adjustable="box")
    ax.plot([lo, hi], [lo, hi], linestyle=(0, (5, 3)), color=DIAGONAL, linewidth=1.2, zorder=1)
    ax.scatter(x, y, s=size, c=DOT, alpha=alpha, linewidths=0, rasterized=True, zorder=2)
    ax.set_xlabel(LABEL[xa]); ax.set_ylabel(LABEL[ya])


def save(fig, stem):
    for ext in ("pdf", "png"):
        fig.savefig(os.path.join(FOLDER, f"{stem}.{ext}"), dpi=300, bbox_inches="tight")
    plt.close(fig)


def main():
    t = load_all()
    for xa, ya, single in (("gitlab", "candidate8", True), ("candidate7", "candidate8", False)):
        ch, sg = xy(t, "chars", xa, ya), xy(t, "sig", xa, ya)
        print(f"{xa} vs {ya}: Characters {len(ch[0]):,}, Sigspatial {len(sg[0]):,} instances")
        panels = [("Characters", ch, 5.0, 0.35, "fig_scatter_characters"), ("Sigspatial", sg, 7.0, 0.45, "fig_scatter_sigspatial")]
        if single:
            for _, (x, y), s, a, stem in panels:
                fig, ax = plt.subplots(figsize=(3.35, 3.35)); draw(ax, x, y, s, a, xa, ya); save(fig, stem)
        fig, axes = plt.subplots(1, 2, figsize=(7.0, 3.5))
        for ax, (name, (x, y), s, a, _) in zip(axes, panels):
            draw(ax, x, y, s, a, xa, ya); ax.set_title(name, fontsize=8, color=INK, loc="left", pad=4)
        fig.tight_layout(w_pad=3.0); save(fig, "fig_scatter" if single else "fig_scatter_c7_c8")


if __name__ == "__main__":
    main()
