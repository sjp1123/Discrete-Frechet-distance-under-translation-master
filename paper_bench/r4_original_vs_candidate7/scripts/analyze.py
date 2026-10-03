#!/usr/bin/env python3
"""r4: value computation (LMF, calcDistance2), original vs candidate7, measured in one server container.
Reads the raw archives of this folder (and, for comparison, the archived runs in paper_bench/results/)
and writes RESULTS.md (Korean, the tables in the format of Table 4 of [BKN20]).

    python3 paper_bench/r4_original_vs_candidate7/scripts/analyze.py
"""
import csv, io, math, os, random, statistics, tarfile
from collections import OrderedDict

HERE = os.path.dirname(os.path.abspath(__file__))
FOLDER = os.path.dirname(HERE)
RAW = os.path.join(FOLDER, "raw")
RES = os.path.normpath(os.path.join(FOLDER, "..", "results"))
OUT = os.path.join(FOLDER, "RESULTS.md")
# correction notice (2026-10-03): original/ is a modified copy of the authors' code (authors_check/REPORT.md)
CORRECTION = ("> **정정 (2026-10-03):** 이 문서의 기준선 `original`은 [BKN20] 저자 코드 그대로가 아니다. "
              "`original/`의 `src/frechet_under_translation.cpp`·`src/fut_n6_algorithm.cpp`가 기저 사례에서 탐색 상자를 빼도록 수정되어 있다. "
              "저자 코드([`authors_gitlab/`](../../authors_gitlab/), GitLab 3bbb305) 기준 수치는 "
              "[`paper_bench/authors_check/REPORT.md`](../authors_check/REPORT.md) 4절을 본다. 아래 수치는 수정판 기준 기록으로 남겨 둔다.\n")
SIG_CORRECTION = ("(정정: 수정판의 문제다. 저자 코드는 이 두 쌍을 5 GB 안에서 497.5 s, 338.0 s에 정답으로 끝낸다 — "
                  "[`authors_check/REPORT.md`](../authors_check/REPORT.md) 4.2)")
EPS = 1e-7
O, P = "original", "candidate7"
STAGES = [("Preprocessing", "pre2_ms"), ("Black-box calls (Lipschitz)", "bb2_ms"),
          ("Arrangement estimation", "disc2_ms"), ("Arrangement algorithm", "arr2_ms"),
          ("Construction", "n6_arr_ms"), ("Black-box calls", "n6_fre_ms")]
PAPER = dict(per=140.0, cpi=12387.1, constr_share=52.3)   # [BKN20] Table 4 (LMF, characters_full)


# ------------------------------------------------------------------ loading
def tar_rows(tar_path, prefix):
    """{job: [rows]} for members <prefix>/<job>.csv of a tar archive."""
    out = OrderedDict()
    with tarfile.open(tar_path) as tf:
        for n in sorted(m.name for m in tf.getmembers() if m.isfile()):
            if n.startswith(prefix + "/") and n.endswith(".csv"):
                job = os.path.basename(n)[:-4]
                out[job] = list(csv.DictReader(io.TextIOWrapper(tf.extractfile(n), encoding="utf-8")))
    return out


def tar_text(tar_path, name):
    with tarfile.open(tar_path) as tf:
        try:
            return tf.extractfile(name).read().decode()
        except KeyError:
            return ""


def tar_csv(tar_path, name):
    with tarfile.open(tar_path) as tf:
        return list(csv.DictReader(io.TextIOWrapper(tf.extractfile(name), encoding="utf-8")))


def keyed(jobs, per_job):
    """row index in the pair list -> row.  Characters: c<k> holds rows k*100..; Sigspatial: s<i> is pair i (1-based)."""
    d = {}
    for job, rows in jobs.items():
        for i, r in enumerate(rows):
            d[int(job[1:]) * per_job + i if per_job else int(job[1:]) - 1] = r
    return d


def gmean_ci(ratios, B=2000, seed=1):
    logs = [math.log(x) for x in ratios if x > 0]
    g = math.exp(sum(logs) / len(logs))
    rng = random.Random(seed)
    bs = sorted(math.exp(sum(rng.choice(logs) for _ in logs) / len(logs)) for _ in range(B))
    return g, bs[int(0.025 * B)], bs[int(0.975 * B)]


def fnum(x, d=0):
    return f"{x:,.{d}f}"


def speed_row(label, base, prop):
    rat = [b / p for b, p in zip(base, prop)]
    g, lo, hi = gmean_ci(rat)
    n = len(rat)
    fast = sum(x > 1 for x in rat)
    return (f"| {label} | {n:,} | {sum(base)/1000:,.1f} s | {sum(prop)/1000:,.1f} s | {sum(base)/sum(prop):.2f}× | "
            f"{g:.2f}× [{lo:.2f}, {hi:.2f}] | {statistics.median(rat):.2f}× | {fast:,} ({fast/n*100:.1f} %) |")


def table4(w, R, keys, extra_note=""):
    n = len(keys)
    S = lambda a, c: sum(float(R[a][k][c]) for k in keys)
    to, tp = S(O, "time_ms"), S(P, "time_ms")
    co, cp = S(O, "bbcalls"), S(P, "bbcalls")
    w(f"{n:,}개 인스턴스 합. 괄호 안은 인스턴스당 평균. 비 = 기존 ÷ 제안.{extra_note}\n")
    w("| Algorithm | 기존 (original) | 제안 (candidate7) | 비 |")
    w("|---|--:|--:|--:|")
    w(f"| **Time** | **{fnum(to)} ms** ({to/n:,.2f} ms/inst.) | **{fnum(tp)} ms** ({tp/n:,.2f} ms/inst.) | **{to/tp:.2f}×** |")
    w(f"| **Black-Box Calls** | **{fnum(co)}** ({co/n:,.1f}/inst.) | **{fnum(cp)}** ({cp/n:,.1f}/inst.) | {co/cp:.2f}× |")
    for lab, col in STAGES:
        a, b = S(O, col), S(P, col)
        if col in ("n6_arr_ms", "n6_fre_ms"):
            lab2 = "∗ Construction / 극대 집합 열거" if col == "n6_arr_ms" else "∗ Black-box calls"
            w(f"| &nbsp;&nbsp;{lab2} | {fnum(a)} ms | {fnum(b)} ms | {a/b:.2f}× |")
        else:
            w(f"| – {lab} | {fnum(a)} ms | {fnum(b)} ms | {a/b:.2f}× |")
    w("")
    return dict(n=n, to=to, tp=tp, co=co, cp=cp, S=S)


def latex(w, label, caption, t):
    def fm(x): return f"{x:,.0f}".replace(",", "{,}")
    def fc(x): return f"{x:,.1f}".replace(",", "{,}")
    n, S = t["n"], t["S"]
    w("```latex")
    w("\\begin{table}[t]\n\\centering")
    w(f"\\caption{{{caption}}}")
    w(f"\\label{{{label}}}\n\\begin{{tabular}}{{llrr}}\n\\toprule")
    w("\\textbf{Algorithm} & \\multicolumn{2}{c}{\\textbf{Time}} & \\textbf{Black-Box Calls} \\\\\n\\midrule")
    for a, name, tt, cc in ((O, "LMF, baseline", t["to"], t["co"]), (P, "LMF, proposed", t["tp"], t["cp"])):
        w(f"{name} & \\multicolumn{{2}}{{c}}{{{fm(tt)} ms}} & {fm(cc)} \\\\")
        w(f"      & \\multicolumn{{2}}{{c}}{{({fc(tt/n)} ms/inst.)}} & ({fc(cc/n)}/inst.) \\\\")
        for lab, col in STAGES:
            v = S(a, col)
            if col in ("n6_arr_ms", "n6_fre_ms"):
                w(f"& \\hphantom{{bla}} * {lab} & {fm(v)} ms \\\\")
            else:
                w("\\cmidrule(r){2-3}")
                w(f"& -- {lab} & {fm(v)} ms \\\\")
        w("\\midrule" if a == O else "\\bottomrule")
    w("\\end{tabular}\n\\end{table}")
    w("```\n")


# ------------------------------------------------------------------ main
def main():
    L = []
    w = L.append
    ch_tar = os.path.join(RAW, "raw_characters_lmf_r4.tar.gz")
    sg_tar = os.path.join(RAW, "raw_sigspatial_lmf_r4.tar.gz")
    bb_tar = os.path.join(RAW, "raw_characters_bbcalls_split.tar.gz")

    CH = {a: keyed(tar_rows(ch_tar, a), 100) for a in (O, P)}
    SG = {a: keyed(tar_rows(sg_tar, a), 0) for a in (O, P)}
    ch_keys = sorted(set(CH[O]) & set(CH[P]))
    sg_keys = sorted(set(SG[O]) & set(SG[P]))
    sg_only = sorted(set(SG[P]) - set(SG[O]))

    # archived runs for comparison
    r2o = tar_csv(os.path.join(RES, "raw_characters_uci_lmf_r2.tar.gz"), "characters_uci_lmf_original_r2.csv")
    r2c = tar_csv(os.path.join(RES, "raw_characters_uci_lmf_r2.tar.gz"), "characters_uci_lmf_candidate5_exact_r2.csv")
    wsl_c7 = {a: tar_rows(os.path.join(RES, "raw_c7_timing.tar.gz"), f"lmf_chars/{a}") for a in (O, P)}
    wsl_c7 = {a: keyed(v, 100) for a, v in wsl_c7.items()}
    wsl_sig = {a: keyed(tar_rows(os.path.join(RES, "raw_c7_timing.tar.gz"), f"lmf_sig/{a}"), 0) for a in (O, P, "candidate5")}
    r3o = {(r["file1"], r["file2"]): r for r in tar_csv(os.path.join(RES, "raw_sigspatial_lmf_r3.tar.gz"), "sigspatial_lmf_original_r3.csv")}
    r3c = {(r["file1"], r["file2"]): r for r in tar_csv(os.path.join(RES, "raw_sigspatial_lmf_r3.tar.gz"), "sigspatial_lmf_candidate5_exact_r3.csv")}

    rc = (tar_text(ch_tar, "rc.txt") + tar_text(sg_tar, "rc.txt")).splitlines()
    bad = [l for l in rc if "rc=" in l and not l.endswith("rc=0")]
    skipped = [l for l in rc if "skipped" in l]
    logs = [("Characters", tar_text(ch_tar, "log.txt")), ("Sigspatial", tar_text(sg_tar, "log.txt"))]

    w("# r4: 기존 구현(original) 대 candidate7 — 값 계산(LMF), 한 컨테이너에서 측정\n")
    w(CORRECTION)
    w("기존 구현(`original`, [BKN20] 저자 코드)과 제안 방법의 최종형(`candidate7`)을 [BKN20]의 두 벤치마크에서 같은 기계·같은 방식으로 쟀다. "
      "인스턴스당 1회 측정. 표는 [BKN20] Table 4 형식이다. 이 파일은 `scripts/analyze.py`가 `raw/`에서 만든다.\n")
    w("- **Characters**: `characters_full` 21,000쌍 (`paper_bench/queries/characters_uci_lmf_pairs.txt`, 210파일 × 100쌍, 하네스 순서).")
    w("- **Sigspatial**: 저자의 결정 문제 1,000쌍 (`paper_bench/queries/sigspatial_pairs.txt`, 전체 20,199곡선). 기존 구현은 12 GB를 넘는 "
      "2쌍(125·432번째, experiment_log §6.9)을 건너뛰었다. " + SIG_CORRECTION + " 표는 두 방법 모두 잰 998쌍이다.\n")
    w("**측정 환경** — 서버 컨테이너: Intel Xeon @ 2.10 GHz, 2 vCPU, 7.8 GB, Ubuntu 24.04, g++ 13.3, CMake 3.28, CGAL 5.6 "
      "(header-only, GMPXX 백엔드), Boost 1.83, 시스템 GMP 6.3 / MPFR 4.2. `paper_bench`를 arm별 소스로 빌드(`RelWithDebInfo`, "
      "`-include cstdint -include array -include cstddef`). 시계 `steady_clock`.\n")
    w("**측정 방식** — 두 arm을 같은 코어(CPU 1, `taskset`)에서 별도 프로세스로 연달아 실행하고 순서를 번갈아 바꿨다. "
      "Characters는 100쌍 묶음마다, Sigspatial은 쌍마다 한 프로세스이고, 묶음·쌍 번호가 짝수면 original이 먼저다. CPU 0은 비워 두었다. "
      "candidate7은 `MAXREGION=cech MAXREGION_EXACT=1 MAXREGION_SLACK=0`(= 기본값)으로 실행했다. "
      f"프로세스 {len([l for l in rc if 'rc=' in l]):,}개 중 비정상 종료 {len(bad)}개, 건너뛴 실행 {len(skipped)}개.\n")
    for name, lg in logs:
        if lg.strip():
            w(f"- {name} 실행 기록: " + " / ".join(f"`{l}`" for l in lg.strip().splitlines()))
    w("")

    # ---------------- Characters table
    w("## 1. Characters — [BKN20] Table 4 형식\n")
    tc = table4(w, CH, ch_keys)
    arr_o = tc["S"](O, "arr2_ms") / tc["to"] * 100; arr_p = tc["S"](P, "arr2_ms") / tc["tp"] * 100
    con_o = tc["S"](O, "n6_arr_ms") / tc["to"] * 100; con_p = tc["S"](P, "n6_arr_ms") / tc["tp"] * 100
    unch = sum(tc["S"](P, c) for c in ("pre2_ms", "bb2_ms", "disc2_ms")) / tc["tp"] * 100
    w(f"- Arrangement algorithm 단계 비중: 기존 {arr_o:.1f} % → 제안 {arr_p:.1f} %. Construction 비중: {con_o:.1f} % → {con_p:.1f} % "
      f"([BKN20] {PAPER['constr_share']} %).")
    w(f"- 바꾸지 않은 세 단계(전처리·Lipschitz 호출·배열 추정)가 제안 방법 시간의 {unch:.1f} %다.")
    w("- 제안 방법의 Construction 행은 CGAL 배열 구성 대신 극대 집합·증인점 열거 시간이고, 그 아래 Black-box calls는 증인점에서의 판정이다.\n")

    # ---------------- Sigspatial table
    w("## 2. Sigspatial — [BKN20] Table 4 형식 (Table 4에는 없는 벤치마크)\n")
    ts = table4(w, SG, sg_keys, " 기존 구현이 12 GB를 넘는 2쌍은 제외했다. " + SIG_CORRECTION)
    arr_so = ts["S"](O, "arr2_ms") / ts["to"] * 100; arr_sp = ts["S"](P, "arr2_ms") / ts["tp"] * 100
    w(f"- Arrangement algorithm 단계 비중: 기존 {arr_so:.1f} % → 제안 {arr_sp:.1f} %.")
    tso = {k: float(SG[O][k]["time_ms"]) for k in sg_keys}
    top = sorted(sg_keys, key=lambda k: -tso[k])
    share1 = tso[top[0]] / ts["to"] * 100; share4 = sum(tso[k] for k in top[:4]) / ts["to"] * 100
    w(f"- 총 시간 비는 소수의 긴 쌍이 좌우한다: 기존 구현이 가장 오래 걸린 1쌍이 기존 총 시간의 {share1:.1f} %, 4쌍이 {share4:.1f} %다. "
      "전형적인 쌍은 표 3의 기하평균·중앙값으로 본다.")
    for k in sg_only:
        r = SG[P][k]
        w(f"- 기존 구현이 없는 쌍 {k+1}번 (`{r['file1']}`/`{r['file2']}`): candidate7 {float(r['time_ms'])/1000:.2f} s, 값 {float(r['value']):.6f}.")
    w("")

    # ---------------- per-instance speed-ups
    w("## 3. 인스턴스별 가속비 (기존 시간 ÷ 제안 시간)\n")
    w("기하평균의 95 % 신뢰구간은 부트스트랩 2,000회. 굵은 행이 이번 측정(r4)이고, 나머지는 같은 쌍에 대한 이전 측정이다.\n")
    w("| 비교 | 쌍 | 기존 합 | 제안 합 | 총 시간 비 | 기하평균 [95 % CI] | 중앙값 | 제안이 더 빠른 쌍 |")
    w("|---|--:|--:|--:|--:|--:|--:|--:|")
    b = [float(CH[O][k]["time_ms"]) for k in ch_keys]; p = [float(CH[P][k]["time_ms"]) for k in ch_keys]
    w(speed_row("**Characters r4 (이 컨테이너): original / candidate7**", b, p))
    w(speed_row("Characters r2 (초록, 컨테이너): original / candidate5", [float(r2o[k]["time_ms"]) for k in ch_keys], [float(r2c[k]["time_ms"]) for k in ch_keys]))
    w(speed_row("Characters WSL (이전 c7 측정): original / candidate7", [float(wsl_c7[O][k]["time_ms"]) for k in ch_keys], [float(wsl_c7[P][k]["time_ms"]) for k in ch_keys]))
    b = [tso[k] for k in sg_keys]; p = [float(SG[P][k]["time_ms"]) for k in sg_keys]
    w(speed_row("**Sigspatial r4 (이 컨테이너): original / candidate7**", b, p))
    fk = lambda k: (SG[P][k]["file1"], SG[P][k]["file2"])
    w(speed_row("Sigspatial r3 (WSL): original / candidate5", [float(r3o[fk(k)]["time_ms"]) for k in sg_keys], [float(r3c[fk(k)]["time_ms"]) for k in sg_keys]))
    w(speed_row("Sigspatial WSL (이전 c7 측정): original / candidate7", [float(wsl_sig[O][k]["time_ms"]) for k in sg_keys], [float(wsl_sig[P][k]["time_ms"]) for k in sg_keys]))
    w("")

    # ---------------- correctness / identity
    w("## 4. 정확성과 재현성\n")
    for name, R, keys, ref in (("Characters", CH, ch_keys, wsl_c7), ("Sigspatial", SG, sg_keys, wsl_sig)):
        dv = [abs(float(R[O][k]["value"]) - float(R[P][k]["value"])) for k in keys]
        allp = sorted(R[P])
        same = {a: sum(R[a][k]["value"] == ref[a][k]["value"] and R[a][k]["bbcalls"] == ref[a][k]["bbcalls"] for k in sorted(R[a]) if k in ref[a]) for a in (O, P)}
        tot = {a: sum(1 for k in sorted(R[a]) if k in ref[a]) for a in (O, P)}
        w(f"- **{name}**: |candidate7 − original| 최대 {max(dv):.2e}, 10⁻⁷ 초과 {sum(x > EPS for x in dv)}쌍 / {len(keys):,}. "
          f"이전 WSL 측정과 값·블랙박스 호출 수가 비트 단위로 같은 쌍: original {same[O]:,} / {tot[O]:,}, candidate7 {same[P]:,} / {tot[P]:,}.")
    same_r2 = sum(CH[O][k]["value"] == r2o[k]["value"] and CH[O][k]["bbcalls"] == r2o[k]["bbcalls"] for k in ch_keys)
    w(f"- Characters original은 초록 측정(r2)과도 {same_r2:,} / {len(ch_keys):,}쌍에서 비트 단위로 같다. 블랙박스 호출 수는 기계와 무관한 결정적 값이다.\n")

    # ---------------- comparison with abstract and paper
    w("## 5. 초록·[BKN20]과의 대조 — Characters, 인스턴스당\n")
    n = len(ch_keys)
    ab_o = sum(float(r2o[k]["time_ms"]) for k in ch_keys); ab_c = sum(float(r2c[k]["time_ms"]) for k in ch_keys)
    w("| | 시간 (ms/inst.) | 블랙박스 호출/inst. | Construction 비중 | 가속 (총 시간 비) |")
    w("|---|--:|--:|--:|--:|")
    w(f"| [BKN20] Table 4, LMF (저자 기계) | {PAPER['per']:.1f} | {PAPER['cpi']:,.1f} | {PAPER['constr_share']} % | — |")
    w(f"| 초록 r2 기존 (original) | {ab_o/n:.2f} | {sum(float(r2o[k]['bbcalls']) for k in ch_keys)/n:,.1f} | {sum(float(r2o[k]['n6_arr_ms']) for k in ch_keys)/ab_o*100:.1f} % | — |")
    w(f"| 초록 r2 제안 (candidate5) | {ab_c/n:.2f} | {sum(float(r2c[k]['bbcalls']) for k in ch_keys)/n:,.1f} | {sum(float(r2c[k]['n6_arr_ms']) for k in ch_keys)/ab_c*100:.1f} % | {ab_o/ab_c:.2f}× |")
    w(f"| **r4 기존 (original)** | **{tc['to']/n:.2f}** | **{tc['co']/n:,.1f}** | **{con_o:.1f} %** | — |")
    w(f"| **r4 제안 (candidate7)** | **{tc['tp']/n:.2f}** | **{tc['cp']/n:,.1f}** | **{con_p:.1f} %** | **{tc['to']/tc['tp']:.2f}×** |")
    w("")

    # ---------------- why 4.41x -> 3.70x (estimate)
    w("## 6. 초록의 4.41×와 달라진 이유 (추정)\n")
    ev, od = [], []
    for job in sorted({f"c{k // 100:03d}" for k in ch_keys}):
        ks = [k for k in ch_keys if k // 100 == int(job[1:])]
        r = sum(float(CH[O][k]["time_ms"]) for k in ks) / sum(float(CH[P][k]["time_ms"]) for k in ks)
        (ev if int(job[1:]) % 2 == 0 else od).append(r)
    gm = lambda v: math.exp(statistics.mean(map(math.log, v)))
    f = {c: tc["S"](O, c) / sum(float(r2o[k][c]) for k in ch_keys) for c in ("time_ms", "pre2_ms", "bb2_ms", "disc2_ms", "arr2_ms")}
    s5 = lambda c: sum(float(r2c[k][c]) for k in ch_keys)
    rest5 = s5("time_ms") - sum(s5(c) for c in ("pre2_ms", "bb2_ms", "disc2_ms", "arr2_ms"))
    est5 = sum(s5(c) * f[c] for c in ("pre2_ms", "bb2_ms", "disc2_ms")) + (s5("arr2_ms") + rest5) * f["time_ms"]
    w(f"- **실행 순서 효과는 없다**: 묶음별 가속비 기하평균이 original을 먼저 돌린 묶음 {gm(ev):.2f}×, candidate7을 먼저 돌린 묶음 {gm(od):.2f}×.")
    w(f"- **기계 차이 (약 {ab_o/ab_c:.2f}× → {tc['to']/est5:.2f}×)**: 같은 original 코드의 시간이 초록 측정의 {f['time_ms']:.2f}배로 줄었는데, "
      f"CGAL 배열 단계({f['arr2_ms']:.2f})가 바꾸지 않은 단계(전처리 {f['pre2_ms']:.2f}, Lipschitz {f['bb2_ms']:.2f}, 배열 추정 {f['disc2_ms']:.2f})보다 "
      f"더 줄었다. 제안 방법은 시간 대부분이 바꾸지 않은 단계라 덜 빨라진다. 이 비율로 추정한 candidate5의 이 기계 시간은 약 {est5/1000:,.0f} s.")
    w(f"- **candidate5 → candidate7 (약 {tc['to']/est5:.2f}× → {tc['to']/tc['tp']:.2f}×)**: candidate7이 추정 candidate5보다 {tc['tp']/est5:.2f}배 시간. "
      f"차이는 Arrangement algorithm 단계에 몰려 있다(r4 candidate7 {fnum(tc['S'](P, 'arr2_ms'))} ms vs 초록 candidate5 {fnum(s5('arr2_ms'))} ms). "
      "원인은 candidate6의 `union` 모드다(7절).")
    w("- 추정치는 candidate5를 이 기계에서 직접 잰 값이 아니라 단계별 시간 비율로 계산한 값이다.\n")

    # ---------------- black-box call split
    w("## 7. 제안 방법의 블랙박스 호출 수 분해 — candidate5 대 candidate7 (Characters 21,000쌍)\n")
    bb = {k: tar_csv(bb_tar, f"{k}.csv") for k in "ABCDE"}
    names = OrderedDict([("A", "candidate5 (초록)"), ("C", "candidate7, `MAXREGION_FIX=none N6_RANGE=0` (candidate5 경로 + 감사 수정 A–H)"),
                         ("D", "candidate7, `none`, `N6_RANGE=1` (+ 범위 수정)"), ("E", "candidate7, `union`, `N6_RANGE=0` (+ union만)"),
                         ("B", "**candidate7 기본** (`union` + 범위 수정)")])
    nb = len(bb["A"])
    Sb = lambda k, c: sum(int(float(r[c])) for r in bb[k])
    w("카운터를 넣은 스크래치 사본(`scripts/bbcalls_split/`, 저장소 소스는 그대로)으로 candidate7의 실행 시 옵션을 하나씩 켜서 21,000쌍 전부를 돌렸다. "
      f"계측 빌드의 호출 수·값은 계측 전과 같다(A = 초록 r2 candidate5: "
      f"{sum(a['bbcalls'] == c['bbcalls'] and a['value'] == c['value'] for a, c in zip(bb['A'], r2c)):,} / {nb:,}쌍, "
      f"B = r4 candidate7: {sum(a['bbcalls'] == CH[P][i]['bbcalls'] and a['value'] == CH[P][i]['value'] for i, a in enumerate(bb['B'])):,} / {nb:,}쌍).\n")
    w("| 설정 | 전체 호출/inst. | Lipschitz 단계 | 기저 사례 | └ 게이트 `lessThan(max)` | └ 거리 탐색 | └ 그중 box family 2차 판정 |")
    w("|---|--:|--:|--:|--:|--:|--:|")
    for k, lab in names.items():
        tot, base, gate = Sb(k, "bbcalls"), Sb(k, "base_calls"), Sb(k, "gate_calls")
        box = Sb(k, "boxfam_calls")
        w(f"| {lab} | {tot/nb:,.1f} | {(tot-base)/nb:,.1f} | {base/nb:,.1f} | {gate/nb:,.1f} | {(base-gate)/nb:,.1f} | {box/nb:,.1f} |" if box else
          f"| {lab} | {tot/nb:,.1f} | {(tot-base)/nb:,.1f} | {base/nb:,.1f} | {gate/nb:,.1f} | {(base-gate)/nb:,.1f} | — |")
    w("")
    ident_ac = sum(a["value"] == c["value"] and a["bbcalls"] == c["bbcalls"] for a, c in zip(bb["A"], bb["C"]))
    dRA = [int(d["bbcalls"]) - int(a["bbcalls"]) for d, a in zip(bb["D"], bb["A"])]
    dBD = [int(b_["bbcalls"]) - int(d["bbcalls"]) for b_, d in zip(bb["B"], bb["D"])]
    bases_B, gno_B = Sb("B", "bases"), Sb("B", "bases") - Sb("B", "gate_yes")
    passes_B, boxc_B = Sb("B", "boxfam_passes"), Sb("B", "boxfam_calls")
    w(f"- **감사 수정 A–H: 0.** candidate5 경로로 돌린 candidate7은 {ident_ac:,} / {nb:,}쌍에서 candidate5와 값·호출 수가 같다.")
    w(f"- **범위 수정 `N6_RANGE`(candidate6): {(Sb('D','bbcalls')-Sb('A','bbcalls'))/nb:+.1f}/inst.** 기저 사례의 거리 탐색이 [0, f(τ_start)] 대신 [ℓ_B, max]만 "
      f"이분 탐색한다. 탐색 시작 때의 고정 평행이동 거리 평가({Sb('A','init_eval_calls')/nb:.1f}/inst.)가 없어지고 탐침이 준다. "
      f"{sum(x < 0 for x in dRA):,}쌍에서 줄고 {sum(x > 0 for x in dRA):,}쌍에서 는다.")
    w(f"- **`union` 모드(candidate6의 건전성 수정): {(Sb('B','bbcalls')-Sb('D','bbcalls'))/nb:+.1f}/inst.** candidate5의 증인점이 모두 NO일 때 상자 안 "
      f"극대 집합(box family)의 증인점을 더 판정한다. 기저 사례 {bases_B:,}개 중 게이트가 NO인 {gno_B:,}개({gno_B/bases_B*100:.1f} %)가 거의 모두 "
      f"이 판정을 치르고, 2차 판정 {passes_B:,}회 × 평균 {boxc_B/passes_B:.1f}개 증인점 = {boxc_B/nb:.1f}/inst.다. "
      f"{sum(x > 0 for x in dBD):,}쌍에서 늘고 {sum(x < 0 for x in dBD):,}쌍에서 준다.")
    w(f"- Lipschitz 단계는 거의 그대로다({(Sb('A','bbcalls')-Sb('A','base_calls'))/nb:,.1f} → {(Sb('B','bbcalls')-Sb('B','base_calls'))/nb:,.1f}). "
      f"늘어난 호출은 모두 기저 사례에서 나온다({Sb('A','base_calls')/nb:.1f} → {Sb('B','base_calls')/nb:.1f}).")
    w(f"- 2차 판정은 candidate5 기저 사례의 결함(상자 전체를 덮는 원판을 무시해 YES 상자를 NO로 판정할 수 있음)을 막는 비용이다. "
      f"이 데이터에서도 2차 판정이 YES를 찾아 게이트 YES가 늘었다(`N6_RANGE=1` 기준 {Sb('D','gate_yes'):,} → {Sb('B','gate_yes'):,}회). "
      f"기존 구현 대비 호출 감소는 {tc['co']/Sb('A','bbcalls'):.2f}×(candidate5)에서 {tc['co']/Sb('B','bbcalls'):.2f}×(candidate7)로 줄었다.\n")

    # ---------------- LaTeX
    w("## LaTeX\n")
    latex(w, "tab:lmf-profile-chars",
          "Value computation on the 21{,}000 \\texttt{characters\\_full} instances of~\\cite{BKN20}, in the format of their Table~4: "
          "LMF with the original arrangement construction (baseline) and with the proposed maximal-set enumeration (proposed), "
          "measured back-to-back on the same core, one measurement per instance.", tc)
    latex(w, "tab:lmf-profile-sig",
          "Value computation on the Sigspatial decider pairs of~\\cite{BKN20} (full 20{,}199-curve set), in the format of their Table~4, "
          "without the 2 pairs on which the baseline exceeds 12\\,GB of memory. Same machine and protocol as the Characters table.", ts)

    open(OUT, "w", encoding="utf-8").write("\n".join(L) + "\n")
    print(f"wrote {OUT}")


if __name__ == "__main__":
    main()
