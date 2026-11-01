#!/usr/bin/env python3
"""r8 analysis: raw/ -> RESULTS.md, fig_ablation.{pdf,png}, fig_counts.{pdf,png}.

1. 2x2 ablation (timing run, raw/r8_timing.tar.xz): the authors' code (gitlab), the authors' code with
   candidate8's radius slack (gitlab_slack), candidate7 (maximal sets, s = 0), candidate8 (maximal sets,
   s = budget).  [BKN20] Table 4 format sums, per-instance speedups (geometric mean with a percentile
   bootstrap 95 % interval, 2,000 resamples, seed 1; median), job-level exact sign tests (Characters:
   100-pair chunks, Sigspatial: pairs), values against the authors' code.
2. Base-case counts (counting run, raw/r8_count.tar.xz): per base-case decision of candidate8,
   m = cut discs, V = vertices of the authors' arrangement for the same discs, radius and box,
   K = candidate8's box family (radius delta + s), K0 = the box family at radius delta,
   calls of both base cases.

    python3 paper_bench/r8_ablation/scripts/analyze_r8.py
"""
import collections, csv, io, math, os, tarfile
import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__)); FOLDER = os.path.dirname(HERE); RAW = os.path.join(FOLDER, "raw")
ARMS = ["gitlab", "gitlab_slack", "candidate7", "candidate8"]
NAME = {"gitlab": "저자 코드", "gitlab_slack": "저자 코드 + 여유", "candidate7": "candidate7 (극대, 여유 없음)",
        "candidate8": "candidate8 (극대 + 여유)"}
SHORT = {"gitlab": "A", "gitlab_slack": "A+s", "candidate7": "c7", "candidate8": "c8"}
STAGES = [("Preprocessing", "pre2_ms"), ("Black-box calls (Lipschitz)", "bb2_ms"), ("Arrangement estimation", "disc2_ms"),
          ("Arrangement algorithm", "arr2_ms"), ("∗ Construction / 극대 집합 열거", "n6_arr_ms"), ("∗ Black-box calls", "n6_fre_ms")]
BENCH = [("chars", "Characters", "100쌍 묶음"), ("sig", "Sigspatial", "쌍")]


def load_timing():
    """{bench: {arm: {(job, row): row-dict}}}, {bench: log text}"""
    t, logs = {}, {}
    with tarfile.open(os.path.join(RAW, "r8_timing.tar.xz")) as tf:
        for m in tf.getmembers():
            p = m.name.split("/")
            if not m.isfile():
                continue
            if len(p) == 3 and p[0] in ("chars", "sig") and p[1] in ARMS and p[2].endswith(".csv"):
                rows = list(csv.DictReader(io.TextIOWrapper(tf.extractfile(m), encoding="utf-8")))
                for i, r in enumerate(rows):
                    t.setdefault(p[0], {}).setdefault(p[1], {})[(p[2][:-4], i)] = r
            elif len(p) == 2 and p[1] in ("log.txt", "rc.txt"):
                logs.setdefault(p[0], {})[p[1]] = tf.extractfile(m).read().decode()
            elif len(p) == 1 and p[0] == "log.txt":
                logs["top"] = tf.extractfile(m).read().decode()
    return t, logs


def S(rows, col): return sum(float(r[col]) for r in rows)


def gmean_ci(r, B=2000, seed=1):
    lg = np.log(np.asarray(r)); n = len(lg); rng = np.random.default_rng(seed)
    bs = np.sort(np.array([lg[rng.integers(0, n, n)].mean() for _ in range(B)]))
    return math.exp(lg.mean()), math.exp(bs[int(0.025 * B)]), math.exp(bs[int(0.975 * B) - 1])


def sign_p(k, n):
    """two-sided exact binomial p for k successes of n at 1/2"""
    if n == 0:
        return 1.0
    k = min(k, n - k)
    p = sum(math.comb(n, i) for i in range(k + 1)) / 2 ** n
    return min(1.0, 2 * p)


def fmt_p(p): return f"{p:.0e}" if p < 1e-3 else f"{p:.3f}"


def compare(d, a, b, job_unit):
    """ratio a / b: totals, per-instance geomean [CI], median, b faster, calls, job-level sign test"""
    keys = sorted(set(d[a]) & set(d[b]))
    ta = np.array([float(d[a][k]["time_ms"]) for k in keys]); tb = np.array([float(d[b][k]["time_ms"]) for k in keys])
    ca = sum(int(d[a][k]["bbcalls"]) for k in keys); cb = sum(int(d[b][k]["bbcalls"]) for k in keys)
    r = ta / tb; g, lo, hi = gmean_ci(r)
    jobs = collections.defaultdict(lambda: [0.0, 0.0])
    for k, x, y in zip(keys, ta, tb):
        jobs[k[0]][0] += x; jobs[k[0]][1] += y
    jb = sum(1 for x, y in jobs.values() if y < x); jn = sum(1 for x, y in jobs.values() if x != y)
    return dict(n=len(keys), total=ta.sum() / tb.sum(), g=g, lo=lo, hi=hi, med=float(np.median(r)),
                bfast=int((r > 1).sum()), calls=ca / cb, jb=jb, jn=jn, p=sign_p(jb, jn), unit=job_unit)


def main():
    t, logs = load_timing()
    L = []; w = L.append
    w("# r8 — candidate8의 두 요소를 나눠 보기: 극대 집합 × 열거 반지름 여유 (2×2), 극대 집합 수 대 배열 꼭짓점 수\n")
    w("이 파일은 `scripts/analyze_r8.py`가 `raw/`에서 만든다. 설명과 결론은 [`README.md`](README.md)에 있다.\n")
    for line in logs.get("top", "").splitlines():
        w(f"- `{line}`")
    w("")
    effects = {}
    for bench, title, unit in BENCH:
        d = t[bench]
        n = min(len(d[a]) for a in ARMS)
        w(f"## {title} — 네 방식 ([BKN20] Table 4 형식, {n:,} 인스턴스 합)\n")
        w(f"`{logs.get(bench, {}).get('log.txt', '').splitlines()[0] if logs.get(bench) else ''}`\n")
        w("| Algorithm | " + " | ".join(NAME[a] for a in ARMS) + " |")
        w("|---|" + "--:|" * len(ARMS))
        rows = {a: list(d[a].values()) for a in ARMS}
        w("| **Time** | " + " | ".join(f"**{S(rows[a], 'time_ms'):,.0f} ms** ({S(rows[a], 'time_ms')/len(rows[a]):,.2f})" for a in ARMS) + " |")
        w("| **Black-Box Calls** | " + " | ".join(f"**{S(rows[a], 'bbcalls'):,.0f}** ({S(rows[a], 'bbcalls')/len(rows[a]):,.1f})" for a in ARMS) + " |")
        for lab, col in STAGES:
            w(f"| {'&nbsp;&nbsp;' if lab.startswith('∗') else '– '}{lab} | " + " | ".join(f"{S(rows[a], col):,.0f} ms" for a in ARMS) + " |")
        w("")
        rc = [l for l in logs.get(bench, {}).get("rc.txt", "").splitlines() if "rc=" in l]
        w(f"- 프로세스 {len(rc):,}개 중 비정상 종료 {sum('rc=0' not in l for l in rc)}개.\n")
        w(f"### {title} — 효과 (비 = 왼쪽 ÷ 오른쪽, 1보다 크면 오른쪽이 빠름)\n")
        w(f"기하평균의 95 % 신뢰구간은 인스턴스 부트스트랩 2,000회(seed 1). 부호 검정은 {unit} 단위(정확 이항, 양측).\n")
        w("| 비교 | 의미 | 총 시간 비 | 인스턴스별 기하평균 [95 % CI] | 중앙값 | 오른쪽이 빠른 인스턴스 | 호출 비 | 실행 단위 부호 검정 |")
        w("|---|---|--:|--:|--:|--:|--:|---|")
        for a, b, what in (("gitlab", "gitlab_slack", "여유의 효과 (배열 기저 사례)"), ("candidate7", "candidate8", "여유의 효과 (극대 집합 기저 사례)"),
                           ("gitlab", "candidate7", "극대 집합의 효과 (여유 없음)"), ("gitlab_slack", "candidate8", "극대 집합의 효과 (여유 있음)"),
                           ("gitlab", "candidate8", "전체 (r7의 비교)")):
            c = compare(d, a, b, unit); effects[(bench, a, b)] = c
            w(f"| {SHORT[a]} ÷ {SHORT[b]} | {what} | **{c['total']:.3f}×** | **{c['g']:.3f}×** [{c['lo']:.3f}, {c['hi']:.3f}] | {c['med']:.3f}× | "
              f"{c['bfast']:,} ({100*c['bfast']/c['n']:.1f} %) | {c['calls']:.3f}× | {c['jb']} / {c['jn']} {unit}, p = {fmt_p(c['p'])} |")
        w("")
        sa = effects[(bench, "gitlab", "gitlab_slack")]; sm = effects[(bench, "candidate7", "candidate8")]
        w(f"- 여유의 효과 비교(기하평균): 극대 집합 {sm['g']:.3f}× 대 배열 {sa['g']:.3f}× — 비 {sm['g']/sa['g']:.3f}.\n")
        w(f"### {title} — 값 (방식 − 저자 코드)\n")
        w("| 방식 | 중앙값 | 최대 \\|차이\\| | 10⁻⁷ 초과 | 저자 코드보다 작은 / 큰 인스턴스 |"); w("|---|--:|--:|--:|--:|")
        for a in ARMS[1:]:
            keys = sorted(set(d[a]) & set(d["gitlab"]))
            diff = np.array([float(d[a][k]["value"]) - float(d["gitlab"][k]["value"]) for k in keys])
            w(f"| {NAME[a]} | {np.median(diff):+.2e} | {np.abs(diff).max():.2e} | {(np.abs(diff) > 1e-7).sum()} | {(diff < 0).sum():,} / {(diff > 0).sum():,} |")
        w("")
    counts = count_section(w, t)
    open(os.path.join(FOLDER, "RESULTS.md"), "w").write("\n".join(L) + "\n")
    plot(effects, counts)
    print("RESULTS.md, fig_ablation.*, fig_counts.* written")


def load_counts():
    """{bench: np.array rows [m V K1 K K0 ovf ovf0 calls_c8 calls_A yes]}, {bench: {(job,row): value}}, rc lines"""
    rows = collections.defaultdict(list); vals = collections.defaultdict(dict); rc = collections.defaultdict(list); per_job = collections.defaultdict(lambda: collections.defaultdict(list))
    with tarfile.open(os.path.join(RAW, "r8_count.tar.xz")) as tf:
        for m in tf.getmembers():
            p = m.name.split("/")
            if not m.isfile() or p[0] != "count" or len(p) < 3:
                continue
            bench = p[1]
            if len(p) == 4 and p[3].endswith(".txt"):
                for line in tf.extractfile(m).read().decode().splitlines():
                    x = [int(v) for v in line.split()]
                    rows[bench].append(x); per_job[bench][p[2]].append(x)
            elif len(p) == 3 and p[2].endswith(".csv"):
                for i, r in enumerate(csv.DictReader(io.TextIOWrapper(tf.extractfile(m), encoding="utf-8"))):
                    vals[bench][(p[2][:-4], i)] = r["value"]
            elif len(p) == 3 and p[2] == "rc.txt":
                rc[bench] = [l for l in tf.extractfile(m).read().decode().splitlines() if "rc=" in l]
    return {b: np.array(v, dtype=np.int64) for b, v in rows.items()}, vals, rc, per_job


def count_section(w, t):
    a, vals, rc, per_job = load_counts()
    w("## 기저 사례: 극대 집합 수 대 배열 꼭짓점 수 (계수 실행)\n")
    w("candidate8(계수 빌드)이 실제로 들어간 기저 사례 판정마다, 같은 상자·같은 원판·같은 반지름 δ에서 저자 코드의 배열 꼭짓점 수 V와 "
      "candidate8의 상자 family 크기 K(반지름 δ + s), K0(반지름 δ)를 셌다. 판정 횟수는 저자 방식(꼭짓점을 CGAL 순서로, 첫 YES까지)과 "
      "candidate8(첫 단계 증인점 → 상자 family, 첫 YES까지)이다.\n")
    out = {}
    for bench, title, _ in BENCH:
        x = a.get(bench)
        if x is None or len(x) == 0:
            w(f"- {title}: 자료 없음\n"); continue
        m, V, K1, K, K0, ovf, ovf0, cc, ca, yes = x.T
        # values of the counting run against candidate8 of the timing run
        same = sum(1 for k, v in vals[bench].items() if k in t[bench]["candidate8"] and t[bench]["candidate8"][k]["value"] == v)
        w(f"### {title}\n")
        w(f"- 기저 사례 판정 {len(x):,}개 (YES {yes.mean()*100:.1f} %). 계수 실행의 값은 타이밍 측정의 candidate8과 {same:,} / {len(vals[bench]):,} 인스턴스에서 같다. "
          f"프로세스 {len(rc[bench]):,}개 중 비정상 종료 {sum('rc=0' not in l for l in rc[bench])}개.")
        r = V / np.maximum(K, 1); r0 = V / np.maximum(K0, 1)
        w(f"- 합계: ΣV = {V.sum():,}, ΣK = {K.sum():,} (V/K = {V.sum()/K.sum():.1f}), ΣK0 = {K0.sum():,} (V/K0 = {V.sum()/K0.sum():.1f}).")
        w(f"- 판정별 V/K: 중앙값 {np.median(r):.1f}, 사분위 [{np.percentile(r, 25):.1f}, {np.percentile(r, 75):.1f}], 기하평균 {math.exp(np.log(r).mean()):.1f}, "
          f"V ≤ K인 판정 {(V <= K).sum():,} ({100*(V <= K).mean():.2f} %).")
        w(f"- 판정 호출: 저자 방식 Σ {ca.sum():,}, candidate8 Σ {cc.sum():,} → {ca.sum()/cc.sum():.1f}배. NO 판정만: {ca[yes == 0].sum()/max(1, cc[yes == 0].sum()):.1f}배, YES 판정만: {ca[yes == 1].sum()/max(1, cc[yes == 1].sum()):.1f}배.")
        w(f"- 상자 family가 배열 꼭짓점으로 대체된(overflow) 판정: {ovf.sum():,} ({100*ovf.mean():.3f} %).\n")
        w("| 절단 원판 수 m | 판정 수 | 평균 V | 평균 K0 (s = 0) | 평균 K (s) | V/K 중앙값 | 평균 호출: 저자 방식 | 평균 호출: candidate8 |")
        w("|---|--:|--:|--:|--:|--:|--:|--:|")
        bins = [(1, 1), (2, 2), (3, 3), (4, 4), (5, 6), (7, 8), (9, 10), (11, 12), (13, 20), (21, 50), (51, 10**9)]
        for lo_, hi_ in bins:
            s = (m >= lo_) & (m <= hi_)
            if s.sum() == 0:
                continue
            lab = f"{lo_}" if lo_ == hi_ else (f"{lo_}–{hi_}" if hi_ < 10**9 else f"≥ {lo_}")
            w(f"| {lab} | {s.sum():,} | {V[s].mean():,.1f} | {K0[s].mean():,.2f} | {K[s].mean():,.2f} | {np.median(r[s]):.1f} | {ca[s].mean():,.1f} | {cc[s].mean():,.2f} |")
        w("")
        out[bench] = x
    return out


def plot(effects, counts):
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    INK, INK_2 = "#0b0b0b", "#52514e"
    plt.rcParams.update({"font.size": 8, "axes.labelsize": 8, "xtick.labelsize": 7, "ytick.labelsize": 7, "axes.edgecolor": INK_2,
                         "axes.labelcolor": INK, "xtick.color": INK_2, "ytick.color": INK_2, "axes.linewidth": 0.8,
                         "savefig.facecolor": "white", "figure.facecolor": "white", "pdf.fonttype": 42})
    C_ARR, C_MAX = "#9a9a9a", "#2a78d6"
    # ablation: per-instance geomean speedup over the authors' code, with 95 % CI
    fig, axes = plt.subplots(1, 2, figsize=(7.0, 2.8))
    for ax, (bench, title, _) in zip(axes, BENCH):
        labs, vals_, los, his, cols = [], [], [], [], []
        for b, lab, col in (("gitlab_slack", "Authors'\n+ slack", C_ARR), ("candidate7", "Maximal\nsets", C_MAX), ("candidate8", "Maximal sets\n+ slack", C_MAX)):
            c = effects.get((bench, "gitlab", b))
            if c is None:
                continue
            labs.append(lab); vals_.append(c["g"]); los.append(c["g"] - c["lo"]); his.append(c["hi"] - c["g"]); cols.append(col)
        xs = np.arange(len(labs))
        ax.bar(xs, vals_, color=cols, width=0.6, yerr=[los, his], capsize=3, error_kw={"elinewidth": 0.8, "ecolor": INK_2})
        ax.axhline(1.0, color=INK_2, linewidth=0.8, linestyle=(0, (4, 3)))
        for x_, v, h in zip(xs, vals_, his):
            ax.text(x_, v + h + 0.05 * max(vals_), f"{v:.2f}×", ha="center", va="bottom", fontsize=7, color=INK)
        ax.set_xticks(xs); ax.set_xticklabels(labs); ax.set_ylabel("Speedup over authors' LMF\n(per-instance geomean)")
        ax.set_title(title, fontsize=8, color=INK, loc="left", pad=4); ax.set_ylim(0, max(vals_) * 1.25)
        ax.spines["top"].set_visible(False); ax.spines["right"].set_visible(False)
    fig.tight_layout(w_pad=3.0)
    for ext in ("pdf", "png"):
        fig.savefig(os.path.join(FOLDER, f"fig_ablation.{ext}"), dpi=300, bbox_inches="tight")
    plt.close(fig)
    # counts: mean V, K0, K against m
    fig, axes = plt.subplots(1, 2, figsize=(7.0, 2.8))
    for ax, (bench, title, _) in zip(axes, BENCH):
        x = counts.get(bench)
        if x is None:
            continue
        m, V, K1, K, K0 = x[:, 0], x[:, 1], x[:, 2], x[:, 3], x[:, 4]
        # the cut-limit regime (m <= 12); depth-limit boxes with more cut discs are few and listed in RESULTS.md
        ms = [v for v in range(1, 13) if (m == v).sum() >= 20]
        for arr, lab, col in ((V, "Arrangement vertices (authors' base case)", C_ARR), (K, "Maximal sets (proposed)", C_MAX)):
            ax.plot(ms, [arr[m == v].mean() for v in ms], color=col, linewidth=1.4, marker="o", markersize=2.5, label=lab)
        ax.set_yscale("log"); ax.set_xticks(range(1, 13)); ax.set_xlabel("Cut discs m in the base-case box")
        ax.set_ylabel("Mean candidates per decision")
        ax.set_title(title, fontsize=8, color=INK, loc="left", pad=4)
        ax.spines["top"].set_visible(False); ax.spines["right"].set_visible(False)
    axes[0].legend(frameon=False, fontsize=7, loc="upper left")
    fig.tight_layout(w_pad=3.0)
    for ext in ("pdf", "png"):
        fig.savefig(os.path.join(FOLDER, f"fig_counts.{ext}"), dpi=300, bbox_inches="tight")
    plt.close(fig)


if __name__ == "__main__":
    main()
