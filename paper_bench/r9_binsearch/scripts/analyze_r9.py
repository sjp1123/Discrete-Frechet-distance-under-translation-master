#!/usr/bin/env python3
"""r9 summary: the authors' Binary Search (defects fixed) vs candidate7 LMF on the 21,000 Characters pairs.
Reads W (default ~/r9)/{binsearch,candidate7}/c*.csv, writes RESULTS.md next to this script's folder and
fig_scatter_binsearch.{pdf,png} (x = Binary search, y = LMF, proposed; the style of fig_scatter_characters).
    python3 paper_bench/r9_binsearch/scripts/analyze_r9.py [W]
"""
import csv, glob, math, os, random, statistics as st, sys
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

HERE = os.path.dirname(os.path.abspath(__file__)); FOLDER = os.path.dirname(HERE)
W = sys.argv[1] if len(sys.argv) > 1 else os.path.expanduser("~/r9")


def load(arm):
    out = {}
    for f in sorted(glob.glob(f"{W}/{arm}/c*.csv")):
        for i, r in enumerate(csv.DictReader(open(f))):
            out[(os.path.basename(f), i)] = r
    return out


B, C = load("binsearch"), load("candidate7")
keys = sorted(set(B) & set(C))
tb = [float(B[k]["time_ms"]) for k in keys]; tc = [float(C[k]["time_ms"]) for k in keys]
cb = [int(B[k]["bbcalls"]) for k in keys]; cc = [int(C[k]["bbcalls"]) for k in keys]
vb = [float(B[k]["value"]) for k in keys]; vc = [float(C[k]["value"]) for k in keys]
n = len(keys)
sp = [x / y for x, y in zip(tb, tc)]
lg = [math.log(s) for s in sp]
gm = math.exp(st.mean(lg))
rnd = random.Random(1); boots = []
for _ in range(2000):
    smp = [lg[rnd.randrange(n)] for _ in range(n)]
    boots.append(math.exp(sum(smp) / n))
boots.sort(); lo, hi = boots[49], boots[1949]
diff = [abs(a - b) for a, b in zip(vb, vc)]
rc = [l.split() for l in open(f"{W}/rc.txt")] if os.path.exists(f"{W}/rc.txt") else []
bad = sum(1 for r in rc if r[-1] != "rc=0")
log = open(f"{W}/log.txt").read().strip().splitlines() if os.path.exists(f"{W}/log.txt") else []

L = []
L.append("# r9: 이분 탐색(저자 코드, 결함 수정) 대 제안 LMF(candidate7) — Characters 값 계산\n")
L.append("기준은 [BKN20] Figure 6의 Binary Search다. 저자 GitLab 코드(3bbb305)의 `calcDistance`에서 이분 탐색 경로의 결함 세 곳만 고쳤다(`binsearch_fixes.patch`: P10 kd-tree 하한의 부호, C 깊이 한도 분기의 `step()`, D 이분 탐색 종료 조건). 기저 단계는 저자의 배열 방식 그대로다. 제안 방법은 candidate7의 LMF(`MAXREGION=cech MAXREGION_EXACT=1 MAXREGION_SLACK=0`)다. r5와 같은 방식으로 쟀다: CPU 1 고정, 다른 코어는 비움, 100쌍 묶음마다 두 방식을 별도 프로세스로 연달아, 순서를 번갈아. 이 파일은 `scripts/analyze_r9.py`가 만든다.\n")
for l in log: L.append(f"- `{l}`")
L.append(f"- 측정 {n:,} / 21,000쌍, 비정상 종료 {bad}개 / {len(rc)}개 프로세스.\n")
L.append("## 1. 합계\n")
L.append("| | 이분 탐색 (저자, 결함 수정) | LMF, proposed | 비 |\n|---|--:|--:|--:|")
L.append(f"| Time | {sum(tb):,.0f} ms ({sum(tb)/n:.1f} ms/inst.) | {sum(tc):,.0f} ms ({sum(tc)/n:.1f} ms/inst.) | **{sum(tb)/sum(tc):.2f}×** |")
L.append(f"| Black-Box Calls | {sum(cb):,} ({sum(cb)/n:,.1f}/inst.) | {sum(cc):,} ({sum(cc)/n:,.1f}/inst.) | {sum(cb)/sum(cc):.2f}× |\n")
L.append("참고: [BKN20] Table 4의 Binary Search는 10,555,630 ms (502.7 ms/inst.), 875,424,988회 (41,686.9/inst.)이고 LMF(저자)는 2,938,512 ms (140.0 ms/inst.)다.\n")
L.append("## 2. 인스턴스별 가속 (이분 탐색 시간 ÷ 제안 LMF 시간)\n")
L.append("| 인스턴스 | 기하평균 [95 % CI] | 중앙값 | 제안 LMF가 빠른 인스턴스 | 최소 | 최대 |\n|--:|--:|--:|--:|--:|--:|")
L.append(f"| {n:,} | **{gm:.2f}×** [{lo:.2f}, {hi:.2f}] | {st.median(sp):.2f}× | {sum(s > 1 for s in sp):,} ({100*sum(s > 1 for s in sp)/n:.1f} %) | {min(sp):.2f}× | {max(sp):.1f}× |\n")
L.append(f"- 이분 탐색이 더 빠른 인스턴스: {sum(s < 1 for s in sp):,}개. 그 인스턴스들의 이분 탐색 시간 중앙값 {st.median([b for b, s in zip(tb, sp) if s < 1] or [0]):.2f} ms.")
q = sorted(tb);
L.append(f"- 이분 탐색 시간 사분위: {q[n//4]:.1f} / {q[n//2]:.1f} / {q[3*n//4]:.1f} ms.\n")
L.append("## 3. 값\n")
L.append(f"- |이분 탐색 − 제안 LMF| 최대 {max(diff):.3g}, 10⁻⁷ 초과 {sum(d > 1e-7 for d in diff)}쌍 / {n:,}.\n")
open(os.path.join(FOLDER, "RESULTS.md"), "w").write("\n".join(L) + "\n")
print("\n".join(L))

DOT, DIAGONAL, INK, INK_2 = "#2a78d6", "#7f7f7f", "#0b0b0b", "#52514e"
plt.rcParams.update({"font.size": 8, "axes.labelsize": 8, "xtick.labelsize": 7, "ytick.labelsize": 7,
                     "axes.edgecolor": INK_2, "axes.labelcolor": INK, "xtick.color": INK_2, "ytick.color": INK_2,
                     "axes.linewidth": 0.8, "savefig.facecolor": "white", "figure.facecolor": "white", "pdf.fonttype": 42})
fig, ax = plt.subplots(figsize=(3.35, 3.35))
lo_, hi_ = min(tb + tc) / 1.6, max(tb + tc) * 1.6
ax.set_xscale("log"); ax.set_yscale("log"); ax.set_xlim(lo_, hi_); ax.set_ylim(lo_, hi_); ax.set_aspect("equal", adjustable="box")
ax.plot([lo_, hi_], [lo_, hi_], linestyle=(0, (5, 3)), color=DIAGONAL, linewidth=1.2, zorder=1)
ax.scatter(tb, tc, s=5.0, c=DOT, alpha=0.35, linewidths=0, rasterized=True, zorder=2)
ax.set_xlabel("Binary search (ms)"); ax.set_ylabel("LMF, proposed (ms)")
for ext in ("pdf", "png"):
    fig.savefig(os.path.join(FOLDER, f"fig_scatter_binsearch.{ext}"), dpi=300, bbox_inches="tight")
