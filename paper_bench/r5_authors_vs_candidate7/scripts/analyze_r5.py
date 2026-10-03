#!/usr/bin/env python3
"""r5 analysis: raw/ -> RESULTS.md (value computation, [BKN20] Table 4 format) and RESULTS_decider.md
(decision problem, [BKN20] Table 2 format).  Paper-format tables (sums, per-instance means, ratios) plus
per-instance speedups (geometric mean with a percentile bootstrap 95 % interval, 2,000 resamples, seed 1, and median), as in r4.

    python3 paper_bench/r5_authors_vs_candidate7/scripts/analyze_r5.py
"""
import csv, io, math, os, random, re, tarfile

HERE = os.path.dirname(os.path.abspath(__file__)); FOLDER = os.path.dirname(HERE); RAW = os.path.join(FOLDER, "raw")
ROOT = os.path.dirname(os.path.dirname(FOLDER))
B, P = "gitlab", "candidate7"                       # baseline = the authors' code (authors_gitlab/), proposed = candidate7
LMF_STAGES = [("Preprocessing", "pre2_ms"), ("Black-box calls (Lipschitz)", "bb2_ms"), ("Arrangement estimation", "disc2_ms"),
              ("Arrangement algorithm", "arr2_ms"), ("Construction", "n6_arr_ms"), ("Black-box calls", "n6_fre_ms")]
DEC_STAGES = [("Preprocessing", "pre1_ms"), ("Black-box calls (Lipschitz)", "bb1_ms"), ("Arrangement estimation", "disc1_ms"),
              ("Arrangement algorithm", "arr1_ms"), ("Construction", "n6_arr_ms"), ("Black-box calls", "n6_fre_ms")]
# [BKN20] (arXiv 2008.07510) Table 4 (LMF, N_samples = 100, 21,000 pairs) and Table 2 (decider, 23,000 queries per benchmark)
PAPER_T4 = {"time_ms": 2938512, "bbcalls": 260128449, "pre2_ms": 71728, "bb2_ms": 400189, "disc2_ms": 166479,
            "arr2_ms": 2250493, "n6_arr_ms": 1537500, "n6_fre_ms": 545442}
PAPER_T2 = {"characters_uci_same": (429623, 26661524, 5, 44312, 157780, 226469, 148898, 60156),
            "characters_uci_all": (628043, 42781931, 5, 50462, 191177, 385145, 237043, 120149),
            "sigspatial": (1207560, 31420517, 5, 43861, 913266, 249268, 155332, 73934)}
BENCH = [("characters_uci_same", "same-characters"), ("characters_uci_all", "all-characters"), ("sigspatial", "Sigspatial")]
ORDER = [(l, "plus") for l in range(2, -11, -1)] + [(l, "minus") for l in range(-1, -11, -1)]


def tar_csvs(name):
    """{arm: {job: [rows]}} and the text members (log, rc) of raw/<name>.tar.gz"""
    out, text = {}, {}
    with tarfile.open(os.path.join(RAW, name)) as tf:
        for m in tf.getmembers():
            if not m.isfile():
                continue
            parts = m.name.split("/")
            data = tf.extractfile(m).read().decode("utf-8")
            if m.name.endswith(".csv") and len(parts) == 2:
                out.setdefault(parts[0], {})[parts[1][:-4]] = list(csv.DictReader(io.StringIO(data)))
            else:
                text[m.name] = data
    return out, text


def pairs(d):
    """instance-wise rows of both arms, keyed by (job, row)"""
    kb = {(j, i): r for j, rs in d[B].items() for i, r in enumerate(rs)}
    kp = {(j, i): r for j, rs in d[P].items() for i, r in enumerate(rs)}
    keys = sorted(set(kb) & set(kp))
    return [kb[k] for k in keys], [kp[k] for k in keys], len(kb), len(kp)


def S(rows, col): return sum(float(r[col]) for r in rows)
def fnum(x, d=0): return f"{x:,.{d}f}"


def gmean(xs): return math.exp(sum(math.log(x) for x in xs) / len(xs))


def gmean_ci(ratios, B=2000, seed=1):
    """geometric mean of the per-instance ratios with a percentile bootstrap 95 % interval (as in r4)"""
    lg = [math.log(x) for x in ratios]; n = len(lg); rnd = random.Random(seed); bs = []
    for _ in range(B):
        bs.append(sum(lg[rnd.randrange(n)] for _ in range(n)) / n)
    bs.sort()
    return math.exp(sum(lg) / n), math.exp(bs[int(0.025 * B)]), math.exp(bs[int(0.975 * B) - 1])


def median(xs):
    s = sorted(xs); n = len(s)
    return s[n // 2] if n % 2 else (s[n // 2 - 1] + s[n // 2]) / 2


def lmf_table(w, rb, rp, paper=None):
    n = len(rb)
    w("| Algorithm | 기존 (저자 코드) | 제안 (candidate7) | 비 |")
    w("|---|--:|--:|--:|")
    if paper:
        w(f"| *[BKN20] Table 4, 기존 (저자 기계)* | *{fnum(paper['time_ms'])} ms (140.0 ms/inst.)* | | |")
        w(f"| *— 호출* | *{fnum(paper['bbcalls'])} ({paper['bbcalls']/21000:,.1f}/inst.)* | | |")
        w(f"| *— 전처리 / Lipschitz / 배열 추정* | *{fnum(paper['pre2_ms'])} / {fnum(paper['bb2_ms'])} / {fnum(paper['disc2_ms'])} ms* | | |")
        w(f"| *— 배열 알고리즘 (Construction, Black-box calls)* | *{fnum(paper['arr2_ms'])} ms ({fnum(paper['n6_arr_ms'])}, {fnum(paper['n6_fre_ms'])})* | | |")
    tb, tp = S(rb, "time_ms"), S(rp, "time_ms"); cb, cp = S(rb, "bbcalls"), S(rp, "bbcalls")
    w(f"| **Time** | **{fnum(tb)} ms** ({tb/n:,.2f} ms/inst.) | **{fnum(tp)} ms** ({tp/n:,.2f} ms/inst.) | **{tb/tp:.2f}×** |")
    w(f"| **Black-Box Calls** | **{fnum(cb)}** ({cb/n:,.1f}/inst.) | **{fnum(cp)}** ({cp/n:,.1f}/inst.) | {cb/cp:.2f}× |")
    for lab, col in LMF_STAGES:
        b, p = S(rb, col), S(rp, col)
        lab2 = f"&nbsp;&nbsp;∗ {lab}{' / 극대 집합 열거' if col == 'n6_arr_ms' else ''}" if col in ("n6_arr_ms", "n6_fre_ms") else f"– {lab}"
        w(f"| {lab2} | {fnum(b)} ms | {fnum(p)} ms | {b/p:.2f}× |")


def per_instance(w, label, rb, rp):
    r = [float(x["time_ms"]) / float(y["time_ms"]) for x, y in zip(rb, rp)]
    g, lo, hi = gmean_ci(r)
    w(f"| {label} | {len(r):,} | **{g:.2f}×** [{lo:.2f}, {hi:.2f}] | {median(r):.2f}× | {sum(x > 1 for x in r):,} ({100*sum(x > 1 for x in r)/len(r):.1f} %) | {min(r):.2f}× | {max(r):,.1f}× |")


def latex_lmf(w, label, caption, rb, rp, n):
    def fm(x): return f"{x:,.0f}".replace(",", "{,}")
    def fc(x): return f"{x:,.1f}".replace(",", "{,}")
    w("```latex"); w("\\begin{table}[t]"); w("\\centering"); w(f"\\caption{{{caption}}}"); w(f"\\label{{{label}}}")
    w("\\begin{tabular}{llrr}"); w("\\toprule")
    w("\\textbf{Algorithm} & \\multicolumn{2}{c}{\\textbf{Time}} & \\textbf{Black-Box Calls} \\\\"); w("\\midrule")
    for name, rows in (("LMF, authors' code", rb), ("LMF, proposed", rp)):
        t, c = S(rows, "time_ms"), S(rows, "bbcalls")
        w(f"{name} & \\multicolumn{{2}}{{c}}{{{fm(t)} ms}} & {fm(c)} \\\\")
        w(f"      & \\multicolumn{{2}}{{c}}{{({fc(t/n)} ms/inst.)}} & ({fc(c/n)}/inst.) \\\\")
        for lab, col in LMF_STAGES:
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


def lmf_results():
    ch, cht = tar_csvs("characters_lmf_r5.tar.gz"); sg, sgt = tar_csvs("sigspatial_lmf_r5.tar.gz")
    cb, cp, ncb, ncp = pairs(ch); sb, sp, nsb, nsp = pairs(sg)
    L = []; w = L.append
    w("# r5: 저자 코드(GitLab) 대 candidate7 — 값 계산(LMF), [BKN20] Table 4 형식\n")
    w("기준선은 [BKN20] 저자가 공개한 코드(`authors_gitlab/`, GitLab 3bbb305)이고, 제안 방법은 `candidate7`(논문 설정 "
      "`MAXREGION=cech MAXREGION_EXACT=1 MAXREGION_SLACK=0`)이다. r4와 같은 방식으로 쟀다: 한 코어(CPU 1)에 고정, 다른 코어는 비움, "
      "두 방식을 묶음·쌍마다 별도 프로세스로 연달아 실행하고 순서를 번갈아 바꿈. 표는 논문 형식(합계와 인스턴스당 평균)이고, 인스턴스별 가속은 3절에 따로 둔다. "
      "이 파일은 `scripts/analyze_r5.py`가 `raw/`에서 만든다.\n")
    w(f"- **Characters**: `characters_full` 21,000쌍 (`paper_bench/queries/characters_uci_lmf_pairs.txt`, 210묶음 × 100쌍). "
      f"측정 {ncb:,} / {ncp:,}쌍. `{log_line(cht, 'start')}`")
    w(f"- **Sigspatial**: 저자의 결정 문제 1,000쌍 전부 (`paper_bench/queries/sigspatial_pairs.txt`, 전체 20,199곡선). "
      f"측정 {nsb:,} / {nsp:,}쌍. 저자 코드는 5 GB 주소 공간 제한을 걸고 돌렸다. `{log_line(sgt, 'start')}`\n")
    w("## 1. Characters — [BKN20] Table 4 형식 (21,000 인스턴스 합)\n")
    lmf_table(w, cb, cp, PAPER_T4); w("")
    n = len(cb)
    arr_b = S(cb, "arr2_ms") / S(cb, "time_ms") * 100; arr_p = S(cp, "arr2_ms") / S(cp, "time_ms") * 100
    unch = sum(S(cp, c) for c in ("pre2_ms", "bb2_ms", "disc2_ms")) / S(cp, "time_ms") * 100
    w(f"- 블랙박스 호출: 저자 코드 {fnum(S(cb, 'bbcalls'))}회 — 논문 Table 4는 {fnum(PAPER_T4['bbcalls'])}회"
      f"({'같다' if int(S(cb, 'bbcalls')) == PAPER_T4['bbcalls'] else '다르다'}).")
    w(f"- Arrangement algorithm 단계 비중: 기존 {arr_b:.1f} % → 제안 {arr_p:.1f} %. 바꾸지 않은 세 단계(전처리·Lipschitz 호출·배열 추정)가 제안 방법 시간의 {unch:.1f} %다.")
    w("- 제안 방법의 Construction 행은 CGAL 배열 구성 대신 극대 집합·증인점 열거 시간이고, 그 아래 Black-box calls는 증인점에서의 판정이다.\n")
    w("## 2. Sigspatial — [BKN20] Table 4 형식 (1,000 인스턴스 합, Table 4에는 없는 벤치마크)\n")
    lmf_table(w, sb, sp); w("")
    ts = sorted((float(r["time_ms"]) for r in sb), reverse=True); tsum = sum(ts)
    w(f"- 저자 코드의 총 시간은 소수의 긴 쌍이 좌우한다: 가장 긴 1쌍이 {100*ts[0]/tsum:.1f} %, 4쌍이 {100*sum(ts[:4])/tsum:.1f} %다. "
      f"전형적인 쌍은 3절의 인스턴스별 값으로 본다.")
    heavy = [(i, x, y) for i, (x, y) in enumerate(zip(sb, sp), 1) if float(x["time_ms"]) > 100000]
    for i, x, y in heavy:
        w(f"- 쌍 {i} (`{x['file1']}`/`{x['file2']}`): 저자 코드 {float(x['time_ms'])/1000:.1f} s, candidate7 {float(y['time_ms'])/1000:.2f} s, 값 {float(x['value']):.6f} / {float(y['value']):.6f}.")
    w("")
    w("## 3. 인스턴스별 가속 (기존 시간 ÷ 제안 시간)\n")
    w("기하평균의 95 % 신뢰구간은 인스턴스 부트스트랩 2,000회(seed 1)다.\n")
    w("| 벤치마크 | 인스턴스 | 기하평균 [95 % CI] | 중앙값 | 제안이 빠른 인스턴스 | 최소 | 최대 |")
    w("|---|--:|--:|--:|--:|--:|--:|")
    per_instance(w, "Characters", cb, cp); per_instance(w, "Sigspatial", sb, sp); w("")
    w("## 4. 정확성\n")
    for name, rb, rp in (("Characters", cb, cp), ("Sigspatial", sb, sp)):
        d = [abs(float(x["value"]) - float(y["value"])) for x, y in zip(rb, rp)]
        w(f"- **{name}**: |candidate7 − 저자 코드| 최대 {max(d):.2e}, 10⁻⁷ 초과 {sum(x > 1e-7 for x in d)}쌍 / {len(d):,}.")
    for name, t in (("Characters", cht), ("Sigspatial", sgt)):
        rc = [l for k, v in t.items() if k.endswith("rc.txt") for l in v.splitlines() if "rc=" in l]
        w(f"- {name}: 프로세스 {len(rc):,}개 중 비정상 종료 {sum('rc=0' not in l for l in rc)}개.")
    w("")
    w("## LaTeX\n")
    latex_lmf(w, "tab:lmf-profile-chars", "Value computation on the 21{,}000 \\texttt{characters\\_full} instances of~\\cite{BKN20}, in the format of "
              "their Table~4: LMF of the authors' code and with the proposed maximal-set enumeration, measured back-to-back on one core.", cb, cp, len(cb))
    latex_lmf(w, "tab:lmf-profile-sig", "Value computation on the 1{,}000 Sigspatial decider pairs of~\\cite{BKN20} (full 20{,}199-curve set), "
              "in the format of their Table~4. Same machine and protocol as the Characters table.", sb, sp, len(sb))
    open(os.path.join(FOLDER, "RESULTS.md"), "w").write("\n".join(L) + "\n")
    return cb, cp, sb, sp


def truth_class(sign):
    return 1 if sign == "plus" else 0


def dec_results():
    D4, t4 = tar_csvs("decider_4l_r5.tar.gz"); D2, t2 = tar_csvs("decider_2l_r5.tar.gz")
    L = []; w = L.append
    summary = {}
    def collect(D, tag, bench):
        rb, rp, wrong_b, wrong_p, diff, per_set = [], [], 0, 0, 0, []
        for l, s in ORDER:
            job = f"{bench}_{tag}_{l}_{s}"
            xb, xp = D[B][job], D[P][job]
            assert len(xb) == len(xp) == 1000, (job, len(xb), len(xp))
            rb += xb; rp += xp
            want = truth_class(s)
            wrong_b += sum(int(r["answer"]) != want for r in xb); wrong_p += sum(int(r["answer"]) != want for r in xp)
            diff += sum(x["answer"] != y["answer"] for x, y in zip(xb, xp))
            per_set.append((l, s, S(xb, "time_ms") / 1000, S(xp, "time_ms") / 1000, S(xb, "bbcalls") / 1000, S(xp, "bbcalls") / 1000))
        return rb, rp, wrong_b, wrong_p, diff, per_set
    w("# r5: 저자 코드(GitLab) 대 candidate7 — 결정 문제, [BKN20] Table 2 형식\n")
    w("값 계산 r5(`RESULTS.md`)와 같은 방식으로 잰 결정 문제 결과다. 이 파일은 `scripts/analyze_r5.py`가 `raw/`에서 만든다.\n")
    w("- **인스턴스**: 벤치마크마다 저자의 곡선 쌍 1,000개 × 23세트. YES 세트는 δ = (δ*+ε)(1+b^ℓ), ℓ = −10…2, NO 세트는 δ = (δ*−ε)(1−b^ℓ), ℓ = −10…−1. "
      "b = 4(`paperq4`)는 논문 본문의 계수(같은 쌍에 이 저장소에서 만든 파일, 논문의 인스턴스 파일은 배포되지 않음), b = 2(`paperq`)는 저자가 배포한 파일이다.")
    w("- **측정**: 질의 파일(1,000질의)마다 두 방식을 같은 코어(CPU 1)에서 별도 프로세스로 연달아, 순서를 번갈아 실행. 다른 코어는 비움. "
      "단계 타이머는 저자 결정 문제 프로파일의 것(`FUT_PREPROCESSING1`, `FUT_BLACKBOX1`, `FUT_DISCSELECTION1`, `FUT_ARRANGEMENT1`, 하위 `FUT_N6_ARR`, `FUT_N6_FRECHET`).")
    w(f"- 4^ℓ `{log_line(t4, 'start')}`, 2^ℓ `{log_line(t2, 'start')}`\n")
    for tag, D, b in (("paperq4", D4, 4), ("paperq", D2, 2)):
        for bench, name in BENCH:
            summary[(b, bench)] = collect(D, tag, bench)
    w("## 1. 요약\n")
    w("기하평균의 95 % 신뢰구간은 인스턴스 부트스트랩 2,000회(seed 1)다.\n")
    w("| 계수 | 벤치마크 | 질의 | 기존 (ms/inst.) | 제안 (ms/inst.) | 총 시간 비 | 블랙박스 호출/inst. | 인스턴스별 기하평균 [95 % CI] | 중앙값 | 오답 (기존 / 제안) | 답 불일치 |")
    w("|---|---|--:|--:|--:|--:|---|--:|--:|--:|--:|")
    for b in (4, 2):
        for bench, name in BENCH:
            rb, rp, wb, wp, diff, _ = summary[(b, bench)]
            n = len(rb); tb, tp = S(rb, "time_ms"), S(rp, "time_ms")
            r = [float(x["time_ms"]) / float(y["time_ms"]) for x, y in zip(rb, rp) if float(x["time_ms"]) > 0 and float(y["time_ms"]) > 0]
            g, lo, hi = gmean_ci(r)
            w(f"| {b}^ℓ | {name} | {n:,} | {tb/n:.2f} | {tp/n:.2f} | **{tb/tp:.2f}×** | {S(rb,'bbcalls')/n:,.1f} → {S(rp,'bbcalls')/n:,.1f} | {g:.2f}× [{lo:.2f}, {hi:.2f}] | {median(r):.2f}× | {wb} / {wp} | {diff} |")
    w("")
    for b, title in ((4, "## 2. [BKN20] Table 2 형식 — 4^ℓ (논문 본문의 계수, 주 결과)"), (2, "## 3. [BKN20] Table 2 형식 — 2^ℓ (저자가 배포한 질의 파일)")):
        w(title + "\n")
        for bench, name in BENCH:
            rb, rp, wb, wp, diff, _ = summary[(b, bench)]
            n = len(rb); tb, tp = S(rb, "time_ms"), S(rp, "time_ms")
            w(f"### {name} ({n:,}질의, 오답 기존 {wb} / 제안 {wp})\n")
            w("| Algorithm | 기존 (저자 코드) | 제안 (candidate7) | 비 |"); w("|---|--:|--:|--:|")
            if b == 4:
                p = PAPER_T2[bench]
                w(f"| *[BKN20] Table 2, 기존 (저자 기계)* | *{fnum(p[0])} ms ({p[0]/23000:.1f} ms/inst.)* | | |")
                w(f"| *— 호출 / 전처리 / Lipschitz / 배열 추정* | *{p[1]/23000:,.1f}/inst. / {p[2]} / {fnum(p[3])} / {fnum(p[4])} ms* | | |")
                w(f"| *— 배열 알고리즘 (Construction, Black-box calls)* | *{fnum(p[5])} ms ({fnum(p[6])}, {fnum(p[7])})* | | |")
            w(f"| **Time** | **{fnum(tb)} ms** ({tb/n:.2f} ms/inst.) | **{fnum(tp)} ms** ({tp/n:.2f} ms/inst.) | **{tb/tp:.2f}×** |")
            w(f"| **Black-Box Calls** | **{fnum(S(rb,'bbcalls'))}** ({S(rb,'bbcalls')/n:,.1f}/inst.) | **{fnum(S(rp,'bbcalls'))}** ({S(rp,'bbcalls')/n:,.1f}/inst.) | {S(rb,'bbcalls')/S(rp,'bbcalls'):.2f}× |")
            for lab, col in DEC_STAGES:
                x, y = S(rb, col), S(rp, col)
                lab2 = f"&nbsp;&nbsp;∗ {lab}{' / 극대 집합 열거' if col == 'n6_arr_ms' else ''}" if col in ("n6_arr_ms", "n6_fre_ms") else f"– {lab}"
                w(f"| {lab2} | {fnum(x)} ms | {fnum(y)} ms | {(x/y if y > 0 else float('nan')):.2f}× |")
            w("")
            if b == 4:
                p = PAPER_T2[bench]
                w(f"- 저자 코드의 블랙박스 호출 {S(rb,'bbcalls')/n:,.1f}/inst.는 논문 Table 2의 {p[1]/23000:,.1f}/inst.와 {100*(S(rb,'bbcalls')/n - p[1]/23000)/(p[1]/23000):+.1f} % 차이다.")
                w(f"- Arrangement algorithm 비중: 기존 {100*S(rb,'arr1_ms')/tb:.1f} % → 제안 {100*S(rp,'arr1_ms')/tp:.1f} %. 바꾸지 않은 Arrangement estimation이 기존 시간의 {100*S(rb,'disc1_ms')/tb:.1f} %다. "
                  f"[BKN20]의 배열 알고리즘 비중은 {100*p[5]/p[0]:.1f} %, 배열 추정 비중은 {100*p[4]/p[0]:.1f} %다.\n")
    w("## 4. 세트별 결과 — 4^ℓ\n")
    w("값은 세트(1,000질의)의 질의당 평균이다. 비 = 기존 ÷ 제안 (총 시간).\n")
    for bench, name in BENCH:
        per_set = summary[(4, bench)][5]
        w(f"### {name}\n")
        w("| 세트 | 기존 ms/inst. | 제안 ms/inst. | 시간 비 | 기존 호출/inst. | 제안 호출/inst. |"); w("|---|--:|--:|--:|--:|--:|")
        for l, s, tb, tp, cb, cp in per_set:
            lab = f"(1 {'+' if s == 'plus' else '−'} 4^{l})"
            w(f"| {lab} | {tb:.3f} | {tp:.3f} | {tb/tp:.2f}× | {cb:,.1f} | {cp:,.1f} |")
        w("")
    w("## LaTeX (4^ℓ)\n")
    def fm(x): return f"{x:,.0f}".replace(",", "{,}")
    def fc(x): return f"{x:,.1f}".replace(",", "{,}")
    w("```latex"); w("\\begin{table}[t]"); w("\\centering")
    w("\\caption{Decision problem in the format of~\\cite[Table~2]{BKN20}: the authors' code and the proposed maximal-set enumeration "
      "on 23{,}000 queries per benchmark (factors $1 \\pm 4^{\\ell}$), measured back-to-back on one core.}")
    w("\\label{tab:decider-profile}"); w("\\begin{tabular}{lrrrrrr}"); w("\\toprule")
    w(" & \\multicolumn{2}{c}{\\textsc{Same-Characters}} & \\multicolumn{2}{c}{\\textsc{All-Characters}} & \\multicolumn{2}{c}{\\textsc{Sigspatial}} \\\\")
    w(" & authors' & proposed & authors' & proposed & authors' & proposed \\\\"); w("\\midrule")
    rows = [("Time (ms)", "time_ms", fm), ("\\quad per instance", None, None), ("Black-box calls", "bbcalls", fm),
            ("-- Preprocessing", "pre1_ms", fm), ("-- Black-box calls (Lipschitz)", "bb1_ms", fm),
            ("-- Arrangement estimation", "disc1_ms", fm), ("-- Arrangement algorithm", "arr1_ms", fm),
            ("\\quad * Construction", "n6_arr_ms", fm), ("\\quad * Black-box calls", "n6_fre_ms", fm)]
    for lab, col, f in rows:
        cells = []
        for bench, _ in BENCH:
            rb, rp = summary[(4, bench)][0], summary[(4, bench)][1]
            if col is None:
                cells += [fc(S(rb, "time_ms") / len(rb)), fc(S(rp, "time_ms") / len(rp))]
            else:
                cells += [f(S(rb, col)), f(S(rp, col))]
        w(f"{lab} & " + " & ".join(cells) + " \\\\")
    w("\\bottomrule"); w("\\end{tabular}"); w("\\end{table}"); w("```")
    open(os.path.join(FOLDER, "RESULTS_decider.md"), "w").write("\n".join(L) + "\n")
    return summary


if __name__ == "__main__":
    lmf = lmf_results()
    dec = dec_results()
    print("RESULTS.md, RESULTS_decider.md written")
