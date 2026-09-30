#!/usr/bin/env python3
"""r4 decision problem: original vs candidate7 on the same server container as the r4 value computation.
Reads raw/raw_decider_r4.tar.gz (and, for comparison, the archived WSL runs in paper_bench/results/) and
writes RESULTS_decider.md (Korean, the tables in the format of Table 2 of [BKN20]).

    python3 paper_bench/r4_original_vs_candidate7/scripts/analyze_decider.py
"""
import csv, io, math, os, random, statistics, tarfile
from collections import OrderedDict

HERE = os.path.dirname(os.path.abspath(__file__))
FOLDER = os.path.dirname(HERE)
RAW = os.path.join(FOLDER, "raw")
RES = os.path.normpath(os.path.join(FOLDER, "..", "results"))
OUT = os.path.join(FOLDER, "RESULTS_decider.md")
O, P = "original", "candidate7"
SETS = OrderedDict([("characters_uci_same", "same-characters"), ("characters_uci_all", "all-characters"), ("sigspatial", "Sigspatial")])
TAGS = OrderedDict([("paperq4", "4^ℓ (논문 본문의 계수, 주 결과)"), ("paperq", "2^ℓ (저자가 배포한 질의 파일)")])
LEVELS = [(l, "plus") for l in range(2, -11, -1)] + [(l, "minus") for l in range(-1, -11, -1)]
STAGES = [("Preprocessing", "pre1_ms"), ("Black-box calls (Lipschitz)", "bb1_ms"),
          ("Arrangement estimation", "disc1_ms"), ("Arrangement algorithm", "arr1_ms"),
          ("Construction", "n6_arr_ms"), ("Black-box calls", "n6_fre_ms")]
# [BKN20] Table 2 (4^l, authors' machine): total ms, calls, pre, bb Lipschitz, estimation, arr alg, construction, arr bb
PAPER_T2 = {"characters_uci_same": (429623, 26661524, 5, 44312, 157780, 226469, 148898, 60156),
            "characters_uci_all": (628043, 42781931, 5, 50462, 191177, 385145, 237043, 120149),
            "sigspatial": (1207560, 31420517, 5, 43861, 913266, 249268, 155332, 73934)}


def tar_members(tar_path):
    with tarfile.open(tar_path) as tf:
        return {m.name: list(csv.DictReader(io.TextIOWrapper(tf.extractfile(m), encoding="utf-8")))
                for m in tf.getmembers() if m.isfile() and m.name.endswith(".csv")}


def tar_text(tar_path, name):
    with tarfile.open(tar_path) as tf:
        try:
            return tf.extractfile(name).read().decode()
        except KeyError:
            return ""


def gmean_ci(ratios, B=2000, seed=1):
    logs = [math.log(x) for x in ratios if x > 0]
    g = math.exp(sum(logs) / len(logs))
    rng = random.Random(seed)
    bs = sorted(math.exp(sum(rng.choice(logs) for _ in logs) / len(logs)) for _ in range(B))
    return g, bs[int(0.025 * B)], bs[int(0.975 * B)]


def fnum(x, d=0):
    return f"{x:,.{d}f}"


def names(s, tag):
    return [f"{s}_{tag}_{l}_{sign}" for l, sign in LEVELS]


def main():
    tar = os.path.join(RAW, "raw_decider_r4.tar.gz")
    M = tar_members(tar)
    R = {a: {k.split("/", 1)[1][:-4]: v for k, v in M.items() if k.startswith(a + "/")} for a in (O, P)}
    wsl = tar_members(os.path.join(RES, "raw_c7_timing.tar.gz"))
    WSL = {a: {k.split("/")[-1][2:-4]: v for k, v in wsl.items() if k.startswith(f"decider/{a}/")} for a in (O, P)}
    r3 = {**tar_members(os.path.join(RES, "raw_characters_uci_decider_r3.tar.gz")),
          **tar_members(os.path.join(RES, "raw_sigspatial_decider_r3.tar.gz"))}
    rc = tar_text(tar, "rc.txt").splitlines()
    bad = [l for l in rc if "rc=" in l and not l.endswith("rc=0")]
    log = tar_text(tar, "log.txt").strip().splitlines()

    present = lambda s, tag: [n for n in names(s, tag) if n in R[O] and n in R[P] and len(R[O][n]) == len(R[P][n]) > 0]
    rows = lambda a, s, tag: [r for n in present(s, tag) for r in R[a][n]]
    L = []
    w = L.append
    w("# r4: 결정 문제 — 기존 구현(original) 대 candidate7, [BKN20] Table 2 형식\n")
    w("값 계산 r4(`RESULTS.md`)와 같은 서버 컨테이너·같은 방식으로 잰 결정 문제 결과다. 이 파일은 `scripts/analyze_decider.py`가 "
      "`raw/raw_decider_r4.tar.gz`에서 만든다.\n")
    w("- **인스턴스**: 벤치마크마다 저자의 곡선 쌍 1,000개 × 23세트. YES 세트는 δ = (δ*+ε)(1+b^ℓ), ℓ = −10…2이고 NO 세트는 "
      "δ = (δ*−ε)(1−b^ℓ), ℓ = −10…−1이다. 계수 밑 b는 두 가지다. b = 4(`paperq4`)는 논문 본문의 계수로, 저자가 인스턴스 파일을 "
      "배포하지 않아 같은 쌍에 이 저장소에서 만든 것이다. b = 2(`paperq`)는 저자가 배포한 파일 그대로다.")
    w("- **벤치마크**: same-characters, all-characters (Characters UCI 원본, 저자 번호), Sigspatial (전체 20,199곡선).")
    w("- **측정 방식**: 질의 파일(1,000질의)마다 두 arm을 같은 코어(CPU 1)에서 별도 프로세스로 연달아 실행하고, 순서는 파일 번호의 "
      "홀짝으로 번갈아 바꿨다(짝수: original 먼저). CPU 0은 비워 두었다. candidate7은 `MAXREGION=cech MAXREGION_EXACT=1 "
      "MAXREGION_SLACK=0`(= 기본값). 단계 타이머는 저자 `updateProfileDec`의 것(`FUT_PREPROCESSING1`, `FUT_BLACKBOX1`, "
      "`FUT_DISCSELECTION1`, `FUT_ARRANGEMENT1`과 하위 `FUT_N6_ARR`, `FUT_N6_FRECHET`)이고, 하위 행은 단계의 나머지(타이머 없는 부분)를 빼고 센다.")
    w(f"- **환경**: `RESULTS.md`와 같다. 프로세스 {len([l for l in rc if 'rc=' in l]):,}개 중 비정상 종료 {len(bad)}개. "
      + (" / ".join(f"`{l}`" for l in log) if log else "") + "\n")

    # ---------------- summary
    w("## 1. 요약\n")
    w("| 계수 | 벤치마크 | 질의 | 기존 (ms/inst.) | candidate7 (ms/inst.) | 총 시간 비 | 인스턴스별 기하평균 [95 % CI] | 중앙값 | 블랙박스 호출/inst. | 오답 (기존 / 제안) | 답 불일치 |")
    w("|---|---|--:|--:|--:|--:|--:|--:|---|--:|--:|")
    summ = {}
    for tag in TAGS:
        for s, sname in SETS.items():
            ro, rp = rows(O, s, tag), rows(P, s, tag)
            n = len(ro)
            if n == 0:
                continue
            assert n == len(rp) and all((a["file1"], a["file2"], a["distance"]) == (b["file1"], b["file2"], b["distance"]) for a, b in zip(ro, rp))
            exp = []
            for nm in present(s, tag):
                exp += [nm.endswith("_plus")] * len(R[O][nm])
            wo = sum((r["answer"] == "1") != e for r, e in zip(ro, exp))
            wp = sum((r["answer"] == "1") != e for r, e in zip(rp, exp))
            dis = sum(a["answer"] != b["answer"] for a, b in zip(ro, rp))
            to, tp = sum(float(r["time_ms"]) for r in ro), sum(float(r["time_ms"]) for r in rp)
            rat = [float(a["time_ms"]) / float(b["time_ms"]) for a, b in zip(ro, rp)]
            g, lo, hi = gmean_ci(rat)
            co, cp = sum(int(r["bbcalls"]) for r in ro), sum(int(r["bbcalls"]) for r in rp)
            summ[(tag, s)] = dict(n=n, to=to, tp=tp, co=co, cp=cp, g=g, lo=lo, hi=hi, med=statistics.median(rat),
                                  fast=sum(x > 1 for x in rat), wo=wo, wp=wp, dis=dis)
            w(f"| {'4^ℓ' if tag == 'paperq4' else '2^ℓ'} | {sname} | {n:,} | {to/n:.2f} | {tp/n:.2f} | **{to/tp:.2f}×** | {g:.2f}× [{lo:.2f}, {hi:.2f}] | "
              f"{statistics.median(rat):.2f}× | {co/n:,.1f} → {cp/n:,.1f} | {wo} / {wp} | {dis} |")
    w("")
    w("- 결정 문제에서는 질의 대부분이 배열 단계 없이 Lipschitz 탐색만으로 끝난다. 그래서 인스턴스별 기하평균·중앙값은 1에 가깝고, "
      "이득은 배열 단계가 무거운 δ* 근처 NO 질의에 몰린다(4절). 총 시간 비가 이 무거운 질의들의 이득을 반영한다.\n")

    # ---------------- Table 2 format, 4^l
    for tag in TAGS:
        w(f"## {'2' if tag == 'paperq4' else '3'}. [BKN20] Table 2 형식 — {TAGS[tag]}\n")
        for s, sname in SETS.items():
            ro, rp = rows(O, s, tag), rows(P, s, tag)
            n = len(ro)
            if n == 0:
                continue
            S = lambda rr, c: sum(float(r[c]) for r in rr)
            st = summ[(tag, s)]
            w(f"### {sname} ({n:,}질의, 오답 기존 {st['wo']} / 제안 {st['wp']})\n")
            w("| Algorithm | 기존 (original) | 제안 (candidate7) | 비 |")
            w("|---|--:|--:|--:|")
            if tag == "paperq4":
                p = PAPER_T2[s]
                w(f"| *[BKN20] Table 2, 기존 (저자 기계)* | *{fnum(p[0])} ms ({p[0]/23000:.1f} ms/inst.)* | | |")
                w(f"| *— 호출 / 전처리 / Lipschitz / 배열 추정* | *{p[1]/23000:,.1f}/inst. / {p[2]} / {fnum(p[3])} / {fnum(p[4])} ms* | | |")
                w(f"| *— 배열 알고리즘 (Construction, Black-box calls)* | *{fnum(p[5])} ms ({fnum(p[6])}, {fnum(p[7])})* | | |")
            w(f"| **Time** | **{fnum(st['to'])} ms** ({st['to']/n:.2f} ms/inst.) | **{fnum(st['tp'])} ms** ({st['tp']/n:.2f} ms/inst.) | **{st['to']/st['tp']:.2f}×** |")
            w(f"| **Black-Box Calls** | **{fnum(st['co'])}** ({st['co']/n:,.1f}/inst.) | **{fnum(st['cp'])}** ({st['cp']/n:,.1f}/inst.) | {st['co']/st['cp']:.2f}× |")
            for lab, col in STAGES:
                a, b = S(ro, col), S(rp, col)
                ratio = f"{a/b:.2f}×" if b > 0 and a >= 100 else "–"
                if col in ("n6_arr_ms", "n6_fre_ms"):
                    lab2 = "∗ Construction / 극대 집합 열거" if col == "n6_arr_ms" else "∗ Black-box calls"
                    w(f"| &nbsp;&nbsp;{lab2} | {fnum(a)} ms | {fnum(b)} ms | {ratio} |")
                else:
                    w(f"| – {lab} | {fnum(a)} ms | {fnum(b)} ms | {ratio} |")
            arr_o, arr_p = S(ro, "arr1_ms") / st["to"] * 100, S(rp, "arr1_ms") / st["tp"] * 100
            est_o = S(ro, "disc1_ms") / st["to"] * 100
            w("")
            w(f"- Arrangement algorithm 비중: 기존 {arr_o:.1f} % → 제안 {arr_p:.1f} %. 바꾸지 않은 Arrangement estimation이 기존 시간의 {est_o:.1f} %다.")
            if tag == "paperq4":
                p = PAPER_T2[s]
                w(f"- [BKN20]의 배열 알고리즘 비중은 {p[5]/p[0]*100:.1f} %, 배열 추정 비중은 {p[4]/p[0]*100:.1f} %다(인스턴스 파일이 달라 시간·호출은 직접 비교하지 않는다).")
            w("")

    # ---------------- per level
    w("## 4. 세트별 총 시간 비 — 4^ℓ\n")
    w("셀: 기존 ms/inst. → 제안 ms/inst. (총 시간 비). 가속은 δ*에 가까운 NO 세트(1 − 4^ℓ, ℓ ≤ −6)에 몰려 있다. "
      "YES 세트와 먼 NO 세트는 대부분 1× 안팎이고, 질의당 0.01 ms 미만인 세트의 비는 측정 잡음 수준이다.\n")
    w("| 세트 | " + " | ".join(SETS.values()) + " |")
    w("|---|" + "--:|" * len(SETS))
    for l, sign in LEVELS:
        cells = []
        for s in SETS:
            nm = f"{s}_paperq4_{l}_{sign}"
            if nm not in present(s, "paperq4"):
                cells.append("—")
                continue
            ro, rp = R[O][nm], R[P][nm]
            to, tp = sum(float(r["time_ms"]) for r in ro), sum(float(r["time_ms"]) for r in rp)
            fmt = (lambda x: f"{x:.3f}") if to / len(ro) < 1 else (lambda x: f"{x:.2f}")
            cells.append(f"{fmt(to/len(ro))} → {fmt(tp/len(rp))} ({to/tp:.2f}×)")
        w(f"| {'1 + 4^' if sign == 'plus' else '1 − 4^'}{l} ({'YES' if sign == 'plus' else 'NO'}) | " + " | ".join(cells) + " |")
    w("")

    # ---------------- comparison with earlier runs
    w("## 5. 이전 측정과의 비교\n")
    w("| 계수 | 벤치마크 | r4 (이 컨테이너) orig / c7 | WSL c7 측정 orig / c7 | WSL r3 orig / candidate5 |")
    w("|---|---|--:|--:|--:|")
    for tag in TAGS:
        for s, sname in SETS.items():
            if (tag, s) not in summ:
                continue
            st = summ[(tag, s)]
            cells = [f"{st['to']/st['tp']:.2f}× (기하 {st['g']:.2f}×)"]
            if tag == "paperq4" and all(n in WSL[O] and n in WSL[P] for n in present(s, tag)):
                a = [float(r["time_ms"]) for n in present(s, tag) for r in WSL[O][n]]
                b = [float(r["time_ms"]) for n in present(s, tag) for r in WSL[P][n]]
                cells.append(f"{sum(a)/sum(b):.2f}× (기하 {gmean_ci([x/y for x, y in zip(a, b)], B=200)[0]:.2f}×)")
            else:
                cells.append("—")
            a = [float(r["time_ms"]) for n in present(s, tag) for r in r3.get(f"{n}_original_r3.csv", [])]
            b = [float(r["time_ms"]) for n in present(s, tag) for r in r3.get(f"{n}_candidate5_r3.csv", [])]
            cells.append(f"{sum(a)/sum(b):.2f}× (기하 {gmean_ci([x/y for x, y in zip(a, b)], B=200)[0]:.2f}×)" if a and len(a) == len(b) else "—")
            w(f"| {'4^ℓ' if tag == 'paperq4' else '2^ℓ'} | {sname} | " + " | ".join(cells) + " |")
    w("")
    # identity
    same_c7 = {a: [0, 0] for a in (O, P)}
    for tag in ("paperq4",):
        for s in SETS:
            for n in present(s, tag):
                for a in (O, P):
                    if n in WSL[a]:
                        for x, y in zip(R[a][n], WSL[a][n]):
                            same_c7[a][1] += 1
                            same_c7[a][0] += x["answer"] == y["answer"] and x["bbcalls"] == y["bbcalls"]
    same_r3 = [0, 0]
    for tag in TAGS:
        for s in SETS:
            for n in present(s, tag):
                for x, y in zip(R[O][n], r3.get(f"{n}_original_r3.csv", [])):
                    same_r3[1] += 1
                    same_r3[0] += x["answer"] == y["answer"] and x["bbcalls"] == y["bbcalls"]
    w(f"- 답과 블랙박스 호출 수가 이전 WSL candidate7 측정(4^ℓ)과 같은 질의: original {same_c7[O][0]:,} / {same_c7[O][1]:,}, "
      f"candidate7 {same_c7[P][0]:,} / {same_c7[P][1]:,}. original은 WSL r3(4^ℓ·2^ℓ)과도 {same_r3[0]:,} / {same_r3[1]:,}질의에서 같다.")
    w("- WSL c7 측정은 스트림 8개를 동시에 돌렸고, r3는 original 대 candidate5다. 절대 시간은 기계끼리 비교하지 않는다.\n")

    # ---------------- LaTeX (4^l)
    def fm(x): return f"{x:,.0f}".replace(",", "{,}")
    def fc(x): return f"{x:,.1f}".replace(",", "{,}")
    w("## LaTeX (4^ℓ)\n")
    w("```latex")
    w("\\begin{table}[t]\n\\centering")
    w("\\caption{Decision problem on the instances of~\\cite{BKN20} with distance factors $(1\\pm4^{\\ell})$, "
      "23{,}000 queries per data set, one measurement each; baseline and proposed measured back-to-back on the same core. "
      "Times in ms summed over all queries; the sub-rows of the arrangement algorithm omit its untimed rest.}")
    w("\\label{tab:decider-profile}\n\\begin{tabular}{lrrrrrr}\n\\toprule")
    w("& \\multicolumn{2}{c}{same-characters} & \\multicolumn{2}{c}{all-characters} & \\multicolumn{2}{c}{Sigspatial} \\\\")
    w("\\cmidrule(lr){2-3}\\cmidrule(lr){4-5}\\cmidrule(lr){6-7}")
    w("& baseline & proposed & baseline & proposed & baseline & proposed \\\\\n\\midrule")
    rows_tex = [("Time (ms)", "time_ms", fm), ("\\quad per query (ms)", None, None), ("Black-box calls", "bbcalls", fm),
                ("-- Preprocessing", "pre1_ms", fm), ("-- Black-box calls (Lipschitz)", "bb1_ms", fm),
                ("-- Arrangement estimation", "disc1_ms", fm), ("-- Arrangement algorithm", "arr1_ms", fm),
                ("\\quad * Construction", "n6_arr_ms", fm), ("\\quad * Black-box calls", "n6_fre_ms", fm)]
    for lab, col, f in rows_tex:
        cells = []
        for s in SETS:
            for a in (O, P):
                rr = rows(a, s, "paperq4")
                if not rr:
                    cells.append("—")
                    continue
                if col is None:
                    cells.append(fc(sum(float(r["time_ms"]) for r in rr) / len(rr)).replace("{,}", ","))
                else:
                    cells.append(f(sum(float(r[col]) for r in rr)))
        w(f"{lab} & " + " & ".join(cells) + " \\\\")
    w("\\bottomrule\n\\end{tabular}\n\\end{table}")
    w("```")

    open(OUT, "w", encoding="utf-8").write("\n".join(L) + "\n")
    print(f"wrote {OUT}")


if __name__ == "__main__":
    main()
