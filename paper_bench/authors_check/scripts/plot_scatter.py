#!/usr/bin/env python3
"""Figure 6 style scatter plots for the authors_check runs (same plain style as r4: log-log, one dot per
instance, dashed y = x, both axes share one range).

  fig_scatter_gitlab_vs_c7_{characters,sigspatial}.{pdf,png}, fig_scatter_gitlab_vs_c7.{pdf,png}
      x = LMF of the authors' code (GitLab 3bbb305), y = candidate7.  Characters: the 21,000 pairs of
      [BKN20] Table 4.  Sigspatial: all 1,000 authors' decider pairs (the authors' code finishes all of them).
  fig6_authors_shipped.{pdf,png}
      the paper's own Figure 6 comparison (LMF vs Binary Search) drawn from the per-instance times the
      authors shipped in experiments/characters_valcomp_full_scatter_{lmf,binsearch}.dat (their machine;
      this is the run of their characters_valcomp_full_total_table.tex, not necessarily the paper's run).

    python3 paper_bench/authors_check/scripts/plot_scatter.py [path to the GitLab tree, default <repo>/authors_gitlab]
"""
import csv, io, os, sys, tarfile
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

HERE = os.path.dirname(os.path.abspath(__file__)); FOLDER = os.path.dirname(HERE); RAW = os.path.join(FOLDER, "raw")
GITLAB = sys.argv[1] if len(sys.argv) > 1 else os.path.normpath(os.path.join(FOLDER, "..", "..", "authors_gitlab"))
DOT, DIAGONAL, INK, INK_2 = "#2a78d6", "#7f7f7f", "#0b0b0b", "#52514e"
plt.rcParams.update({"font.size": 8, "axes.labelsize": 8, "xtick.labelsize": 7, "ytick.labelsize": 7,
                     "axes.edgecolor": INK_2, "axes.labelcolor": INK, "xtick.color": INK_2, "ytick.color": INK_2,
                     "axes.linewidth": 0.8, "savefig.facecolor": "white", "figure.facecolor": "white", "pdf.fonttype": 42})


def load(tar_name, xa="gitlab", ya="candidate7"):
    t = {xa: {}, ya: {}}
    with tarfile.open(os.path.join(RAW, tar_name)) as tf:
        for m in sorted(tf.getmembers(), key=lambda m: m.name):
            arm = m.name.split("/")[0]
            if not m.isfile() or arm not in t or not m.name.endswith(".csv"):
                continue
            job = os.path.basename(m.name)[:-4]
            for i, r in enumerate(csv.DictReader(io.TextIOWrapper(tf.extractfile(m), encoding="utf-8"))):
                t[arm][(job, i)] = float(r["time_ms"])
    keys = sorted(set(t[xa]) & set(t[ya]))
    return [t[xa][k] for k in keys], [t[ya][k] for k in keys]


def draw(ax, x, y, size, alpha, xl, yl):
    lo, hi = min(x + y) / 1.6, max(x + y) * 1.6
    ax.set_xscale("log"); ax.set_yscale("log"); ax.set_xlim(lo, hi); ax.set_ylim(lo, hi); ax.set_aspect("equal", adjustable="box")
    ax.plot([lo, hi], [lo, hi], linestyle=(0, (5, 3)), color=DIAGONAL, linewidth=1.2, zorder=1)
    ax.scatter(x, y, s=size, c=DOT, alpha=alpha, linewidths=0, rasterized=True, zorder=2)
    ax.set_xlabel(xl); ax.set_ylabel(yl)


def save(fig, stem):
    for ext in ("pdf", "png"):
        fig.savefig(os.path.join(FOLDER, f"{stem}.{ext}"), dpi=300, bbox_inches="tight")
    plt.close(fig)


def main():
    ch = load("characters_lmf_3way.tar.gz"); sg = load("sigspatial_lmf_3way.tar.gz")
    print(f"Characters {len(ch[0]):,}, Sigspatial {len(sg[0]):,} instances")
    xl, yl = "Authors' LMF, GitLab code (ms)", "Proposed LMF, candidate7 (ms)"
    panels = [("Characters", ch, 3.0, 0.10, "fig_scatter_gitlab_vs_c7_characters"),
              ("Sigspatial", sg, 7.0, 0.45, "fig_scatter_gitlab_vs_c7_sigspatial")]
    for _, (x, y), s, a, stem in panels:
        fig, ax = plt.subplots(figsize=(3.35, 3.35)); draw(ax, x, y, s, a, xl, yl); save(fig, stem)
    fig, axes = plt.subplots(1, 2, figsize=(7.0, 3.5))
    for ax, (name, (x, y), s, a, _) in zip(axes, panels):
        draw(ax, x, y, s, a, xl, yl); ax.set_title(name, fontsize=8, color=INK, loc="left", pad=4)
    fig.tight_layout(w_pad=3.0); save(fig, "fig_scatter_gitlab_vs_c7")
    e = os.path.join(GITLAB, "experiments")
    lmf = [float(l) for l in open(os.path.join(e, "characters_valcomp_full_scatter_lmf.dat")) if l.strip()]
    bs = [float(l) for l in open(os.path.join(e, "characters_valcomp_full_scatter_binsearch.dat")) if l.strip()]
    fig, ax = plt.subplots(figsize=(3.35, 3.35))
    draw(ax, bs, lmf, 3.0, 0.10, "Binary Search (ms), authors' shipped times", "LMF (ms), authors' shipped times")
    save(fig, "fig6_authors_shipped")


if __name__ == "__main__":
    main()
