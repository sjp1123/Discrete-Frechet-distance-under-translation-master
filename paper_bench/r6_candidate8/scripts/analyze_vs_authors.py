#!/usr/bin/env python3
"""r6: the authors' code vs candidate8 (with candidate7 alongside) in the r5 format -> RESULTS_vs_authors.md.
Value computation (LMF), [BKN20] Table 4 format: sums and per-instance means per stage, per-instance speedups
(geometric mean with a percentile bootstrap 95 % interval, 2,000 resamples, seed 1, and median), correctness, LaTeX.
Source: raw/r6_timing_3arm.tar.xz (run 1 of run_r6.sh: the three arms measured back-to-back on one core).

    python3 paper_bench/r6_candidate8/scripts/analyze_vs_authors.py
"""
import csv, io, math, os, random, tarfile

HERE = os.path.dirname(os.path.abspath(__file__)); FOLDER = os.path.dirname(HERE); RAW = os.path.join(FOLDER, "raw")
TAR = "r6_timing_3arm.tar.xz"
A, C7, C8 = "gitlab", "candidate7", "candidate8"
STAGES = [("Preprocessing", "pre2_ms"), ("Black-box calls (Lipschitz)", "bb2_ms"), ("Arrangement estimation", "disc2_ms"),
          ("Arrangement algorithm", "arr2_ms"), ("Construction", "n6_arr_ms"), ("Black-box calls", "n6_fre_ms")]
PAPER_T4 = {"time_ms": 2938512, "bbcalls": 260128449, "pre2_ms": 71728, "bb2_ms": 400189, "disc2_ms": 166479,
            "arr2_ms": 2250493, "n6_arr_ms": 1537500, "n6_fre_ms": 545442}


def load():
    """{bench: {arm: {(job, row): row}}}, {bench: text members}"""
    out, text = {}, {}
    with tarfile.open(os.path.join(RAW, TAR)) as tf:
        for m in tf.getmembers():
            if not m.isfile():
                continue
            parts = m.name.split("/")
            data = tf.extractfile(m).read().decode("utf-8")
            if len(parts) == 3 and parts[2].endswith(".csv"):
                for i, r in enumerate(csv.DictReader(io.StringIO(data))):
                    out.setdefault(parts[0], {}).setdefault(parts[1], {})[(parts[2][:-4], i)] = r
            else:
                text.setdefault(parts[0], {})[m.name] = data
    return out, text


def aligned(d):
    keys = sorted(set(d[A]) & set(d[C7]) & set(d[C8]))
    return [d[A][k] for k in keys], [d[C7][k] for k in keys], [d[C8][k] for k in keys]


def S(rows, col): return sum(float(r[col]) for r in rows)
def fnum(x, dg=0): return f"{x:,.{dg}f}"


def gmean_ci(ratios, B=2000, seed=1):
    lg = [math.log(x) for x in ratios]; n = len(lg); rnd = random.Random(seed); bs = []
    for _ in range(B):
        bs.append(sum(lg[rnd.randrange(n)] for _ in range(n)) / n)
    bs.sort()
    return math.exp(sum(lg) / n), math.exp(bs[int(0.025 * B)]), math.exp(bs[int(0.975 * B) - 1])


def median(xs):
    s = sorted(xs); n = len(s)
    return s[n // 2] if n % 2 else (s[n // 2 - 1] + s[n // 2]) / 2


def table(w, ra, r7, r8, paper=None):
    n = len(ra)
    w("| Algorithm | 기존 (저자 코드) | candidate7 | candidate8 | 기존 ÷ c8 | c7 ÷ c8 |")
    w("|---|--:|--:|--:|--:|--:|")
    if paper:
        w(f"| *[BKN20] Table 4, 기존 (저자 기계)* | *{fnum(paper['time_ms'])} ms (140.0 ms/inst.)* | | | | |")
        w(f"| *— 호출* | *{fnum(paper['bbcalls'])} ({paper['bbcalls']/21000:,.1f}/inst.)* | | | | |")
    ta, t7, t8 = S(ra, "time_ms"), S(r7, "time_ms"), S(r8, "time_ms")
    ca, c7, c8 = S(ra, "bbcalls"), S(r7, "bbcalls"), S(r8, "bbcalls")
    w(f"| **Time** | **{fnum(ta)} ms** ({ta/n:,.2f} ms/inst.) | **{fnum(t7)} ms** ({t7/n:,.2f}) | **{fnum(t8)} ms** ({t8/n:,.2f}) "
      f"| **{ta/t8:.2f}×** | {t7/t8:.2f}× |")
    w(f"| **Black-Box Calls** | **{fnum(ca)}** ({ca/n:,.1f}/inst.) | **{fnum(c7)}** ({c7/n:,.1f}) | **{fnum(c8)}** ({c8/n:,.1f}) "
      f"| {ca/c8:.2f}× | {c7/c8:.2f}× |")
    for lab, col in STAGES:
        a, b, c = S(ra, col), S(r7, col), S(r8, col)
        lab2 = (f"&nbsp;&nbsp;∗ {lab}{' / 극대 집합 열거' if col == 'n6_arr_ms' else ''}"
                if col in ("n6_arr_ms", "n6_fre_ms") else f"– {lab}")
        w(f"| {lab2} | {fnum(a)} ms | {fnum(b)} ms | {fnum(c)} ms | {a/c:.2f}× | {b/c:.2f}× |")
    rest = lambda rows: S(rows, "time_ms") - sum(S(rows, c) for c in ("pre2_ms", "bb2_ms", "disc2_ms", "arr2_ms"))
    w(f"| – (단계에 안 잡힌 시간) | {fnum(rest(ra))} ms | {fnum(rest(r7))} ms | {fnum(rest(r8))} ms | | |")


def speedups(w, label, ra, r7, r8):
    for name, x, y in (("기존 ÷ candidate7", ra, r7), ("기존 ÷ candidate8", ra, r8), ("candidate7 ÷ candidate8", r7, r8)):
        r = [float(p["time_ms"]) / float(q["time_ms"]) for p, q in zip(x, y)]
        g, lo, hi = gmean_ci(r); k = sum(v > 1 for v in r)
        w(f"| {label} | {name} | {len(r):,} | **{g:.2f}×** [{lo:.2f}, {hi:.2f}] | {median(r):.2f}× | {k:,} ({100*k/len(r):.1f} %) "
          f"| {min(r):.2f}× | {max(r):,.1f}× |")


def latex(w, label, caption, ra, r8):
    n = len(ra)
    def fm(x): return f"{x:,.0f}".replace(",", "{,}")
    def fc(x): return f"{x:,.1f}".replace(",", "{,}")
    w("```latex"); w("\\begin{table}[t]"); w("\\centering"); w(f"\\caption{{{caption}}}"); w(f"\\label{{{label}}}")
    w("\\begin{tabular}{llrr}"); w("\\toprule")
    w("\\textbf{Algorithm} & \\multicolumn{2}{c}{\\textbf{Time}} & \\textbf{Black-Box Calls} \\\\"); w("\\midrule")
    for name, rows in (("LMF, authors' code", ra), ("LMF, proposed", r8)):
        t, c = S(rows, "time_ms"), S(rows, "bbcalls")
        w(f"{name} & \\multicolumn{{2}}{{c}}{{{fm(t)} ms}} & {fm(c)} \\\\")
        w(f"      & \\multicolumn{{2}}{{c}}{{({fc(t/n)} ms/inst.)}} & ({fc(c/n)}/inst.) \\\\")
        for lab, col in STAGES:
            if col in ("n6_arr_ms", "n6_fre_ms"):
                w(f"& \\hphantom{{bla}} * {lab} & {fm(S(rows, col))} ms \\\\")
            else:
                w("\\cmidrule(r){2-3}"); w(f"& -- {lab} & {fm(S(rows, col))} ms \\\\")
        w("\\midrule" if name.startswith("LMF, authors") else "\\bottomrule")
    w("\\end{tabular}"); w("\\end{table}"); w("```"); w("")


def log_line(text, key):
    for k, v in text.items():
        if k.endswith("log.txt"):
            for l in v.splitlines():
                if l.startswith(key):
                    return l
    return ""


def main():
    d, text = load()
    ca, c7, c8 = aligned(d["chars"]); sa, s7, s8 = aligned(d["sig"])
    L = []; w = L.append
    w("# r6: 저자 코드(GitLab) 대 candidate8 — 값 계산(LMF), [BKN20] Table 4 형식\n")
    w("r5([`../r5_authors_vs_candidate7/RESULTS.md`](../r5_authors_vs_candidate7/RESULTS.md))와 같은 형식이다. 기준선은 [BKN20] 저자 코드"
      "(`authors_gitlab/`, GitLab 3bbb305), 제안 방법은 candidate8(`MAXREGION=cech MAXREGION_EXACT=1 MAXREGION_SLACK=budget`)이고, "
      "candidate7(`MAXREGION_SLACK=0`)을 옆에 둔다. 세 방식을 같은 코어(CPU 1)에서 묶음·쌍마다 별도 프로세스로 연달아, 순서를 돌려 가며 쟀다"
      "(r6 1차 측정, [`README.md`](README.md) 2절). 이 파일은 `scripts/analyze_vs_authors.py`가 `raw/r6_timing_3arm.tar.xz`에서 만든다.\n")
    w("- 측정한 candidate8은 커밋 e264dd4다. 수정판 f51bcbd는 Characters 21,000쌍 전부와 Sigspatial 998쌍에서 값·블랙박스 호출이 같고, "
      "다른 2쌍은 값이 3.6e-9, 3.3e-9 다르다([`README.md`](README.md) 6.1). 수정판만 잰 2차 측정의 candidate7 ÷ candidate8은 "
      "1차와 같다(Characters 1.241× / 1.247×, Sigspatial 1.250× / 1.249×).")
    w(f"- **Characters**: 21,000쌍 (210묶음 × 100쌍). 측정 {len(ca):,}쌍. `{log_line(text['chars'], 'start')}`")
    w(f"- **Sigspatial**: 저자의 결정 문제 1,000쌍 (전체 20,199곡선). 측정 {len(sa):,}쌍. 저자 코드는 5 GB 주소 공간 제한. "
      f"`{log_line(text['sig'], 'start')}`")
    w("- 이 기계의 시간은 r5 때보다 약 10 % 길다(저자 코드 Characters 123.9 → 138.2 ms/쌍). 비교는 같은 측정 안의 비로 한다.\n")
    w("## 1. Characters — [BKN20] Table 4 형식 (21,000 인스턴스 합)\n")
    table(w, ca, c7, c8, PAPER_T4); w("")
    arr = lambda rows: S(rows, "arr2_ms") / S(rows, "time_ms") * 100
    unch = lambda rows: sum(S(rows, c) for c in ("pre2_ms", "bb2_ms", "disc2_ms")) / S(rows, "time_ms") * 100
    w(f"- 블랙박스 호출: 저자 코드 {fnum(S(ca, 'bbcalls'))}회 — 논문 Table 4와 "
      f"{'같다' if int(S(ca, 'bbcalls')) == PAPER_T4['bbcalls'] else '다르다'}.")
    w(f"- Arrangement algorithm 단계 비중: 기존 {arr(ca):.1f} %, candidate7 {arr(c7):.1f} %, candidate8 {arr(c8):.1f} %. "
      f"전처리·Lipschitz 호출·배열 추정의 비중은 candidate7 {unch(c7):.1f} %, candidate8 {unch(c8):.1f} %다.")
    w("- candidate8은 기저 사례만이 아니라 Lipschitz 호출과 배열 추정도 줄인다. 기저 사례 값이 조금 낮게 정해져 상자가 일찍 종료되기 때문이다"
      "([`README.md`](README.md) 3절).")
    w("- 단계에 안 잡힌 시간은 r5와 같은 Lipschitz 단계 타이머 결함(버려지는 상자에서 `FUT_BLACKBOX2`가 멈추지 않음)이다.\n")
    w("## 2. Sigspatial — [BKN20] Table 4 형식 (1,000 인스턴스 합, Table 4에는 없는 벤치마크)\n")
    table(w, sa, s7, s8); w("")
    ts = sorted((float(r["time_ms"]) for r in sa), reverse=True); tsum = sum(ts)
    w(f"- 저자 코드의 총 시간은 소수의 긴 쌍이 좌우한다: 가장 긴 1쌍이 {100*ts[0]/tsum:.1f} %, 4쌍이 {100*sum(ts[:4])/tsum:.1f} %다. "
      f"전형적인 쌍은 3절의 인스턴스별 값으로 본다.")
    for i, (x, y, z) in enumerate(zip(sa, s7, s8), 1):
        if float(x["time_ms"]) > 100000:
            w(f"- 쌍 {i} (`{x['file1']}`/`{x['file2']}`): 저자 코드 {float(x['time_ms'])/1000:.1f} s, candidate7 {float(y['time_ms'])/1000:.2f} s, "
              f"candidate8 {float(z['time_ms'])/1000:.2f} s.")
    w("")
    w("## 3. 인스턴스별 가속\n")
    w("기하평균의 95 % 신뢰구간은 인스턴스 부트스트랩 2,000회(seed 1)다. '빠른 인스턴스'는 분모 쪽 방식이 더 빠른 인스턴스다.\n")
    w("| 벤치마크 | 비교 | 인스턴스 | 기하평균 [95 % CI] | 중앙값 | 분모가 빠른 인스턴스 | 최소 | 최대 |")
    w("|---|---|--:|--:|--:|--:|--:|--:|")
    speedups(w, "Characters", ca, c7, c8); speedups(w, "Sigspatial", sa, s7, s8); w("")
    w("## 4. 정확성\n")
    for name, ra, r7, r8 in (("Characters", ca, c7, c8), ("Sigspatial", sa, s7, s8)):
        for lab, rp in (("candidate7", r7), ("candidate8", r8)):
            dv = [float(y["value"]) - float(x["value"]) for x, y in zip(ra, rp)]
            w(f"- **{name}**, {lab} − 저자 코드: 최대 |차이| {max(map(abs, dv)):.2e}, 중앙값 {median(dv):+.2e}, "
              f"10⁻⁷ 초과 {sum(abs(v) > 1e-7 for v in dv)}쌍 / {len(dv):,}.")
    for name, key in (("Characters", "chars"), ("Sigspatial", "sig")):
        rc = [l for k, v in text[key].items() if k.endswith("rc.txt") for l in v.splitlines() if "rc=" in l]
        w(f"- {name}: 프로세스 {len(rc):,}개 중 비정상 종료 {sum('rc=0' not in l for l in rc)}개.")
    w("")
    w("결정 문제(Table 2 형식)는 r6에서 저자 코드를 다시 재지 않았다. candidate8은 4^ℓ 69,000질의에서 candidate7과 블랙박스 호출 수가 같고"
      "(차이 ≤ 4회), 시간 차이도 일관되지 않으므로([`README.md`](README.md) 4절), 저자 코드 대비 결정 문제 결과는 r5의 "
      "[`RESULTS_decider.md`](../r5_authors_vs_candidate7/RESULTS_decider.md)가 그대로 해당한다.\n")
    w("## LaTeX\n")
    latex(w, "tab:lmf-profile-chars-c8", "Value computation on the 21{,}000 \\texttt{characters\\_full} instances of~\\cite{BKN20}, "
          "in the format of their Table~4: LMF of the authors' code and with the proposed maximal-set enumeration "
          "(enumeration radius budgeted by the decider's slack), measured back-to-back on one core.", ca, c8)
    latex(w, "tab:lmf-profile-sig-c8", "Value computation on the 1{,}000 Sigspatial decider pairs of~\\cite{BKN20} (full 20{,}199-curve set), "
          "in the format of their Table~4. Same machine and protocol as the Characters table.", sa, s8)
    open(os.path.join(FOLDER, "RESULTS_vs_authors.md"), "w").write("\n".join(L) + "\n")
    print("wrote RESULTS_vs_authors.md")


if __name__ == "__main__":
    main()
