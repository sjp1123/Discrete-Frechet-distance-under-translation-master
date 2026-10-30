# r6: 저자 코드(GitLab) 대 candidate8 — 값 계산(LMF), [BKN20] Table 4 형식

r5([`../r5_authors_vs_candidate7/RESULTS.md`](../r5_authors_vs_candidate7/RESULTS.md))와 같은 형식이다. 기준선은 [BKN20] 저자 코드(`authors_gitlab/`, GitLab 3bbb305), 제안 방법은 candidate8(`MAXREGION=cech MAXREGION_EXACT=1 MAXREGION_SLACK=budget`)이고, candidate7(`MAXREGION_SLACK=0`)을 옆에 둔다. 세 방식을 같은 코어(CPU 1)에서 묶음·쌍마다 별도 프로세스로 연달아, 순서를 돌려 가며 쟀다(r6 1차 측정, [`README.md`](README.md) 2절). 이 파일은 `scripts/analyze_vs_authors.py`가 `raw/r6_timing_3arm.tar.xz`에서 만든다.

- 측정한 candidate8은 커밋 e264dd4다. 수정판 f51bcbd는 Characters 21,000쌍 전부와 Sigspatial 998쌍에서 값·블랙박스 호출이 같고, 다른 2쌍은 값이 3.6e-9, 3.3e-9 다르다([`README.md`](README.md) 6.1). 수정판만 잰 2차 측정의 candidate7 ÷ candidate8은 1차와 같다(Characters 1.241× / 1.247×, Sigspatial 1.250× / 1.249×).
- **Characters**: 21,000쌍 (210묶음 × 100쌍). 측정 21,000쌍. `start 2026-10-29T16:26:14+09:00 cpu 1 arms gitlab candidate7 candidate8`
- **Sigspatial**: 저자의 결정 문제 1,000쌍 (전체 20,199곡선). 측정 1,000쌍. 저자 코드는 5 GB 주소 공간 제한. `start 2026-10-29T17:38:40+09:00 cpu 1 arms gitlab candidate7 candidate8`
- 이 기계의 시간은 r5 때보다 약 10 % 길다(저자 코드 Characters 123.9 → 138.2 ms/쌍). 비교는 같은 측정 안의 비로 한다.

## 1. Characters — [BKN20] Table 4 형식 (21,000 인스턴스 합)

| Algorithm | 기존 (저자 코드) | candidate7 | candidate8 | 기존 ÷ c8 | c7 ÷ c8 |
|---|--:|--:|--:|--:|--:|
| *[BKN20] Table 4, 기존 (저자 기계)* | *2,938,512 ms (140.0 ms/inst.)* | | | | |
| *— 호출* | *260,128,449 (12,387.1/inst.)* | | | | |
| **Time** | **2,902,195 ms** (138.20 ms/inst.) | **786,334 ms** (37.44) | **630,595 ms** (30.03) | **4.60×** | 1.25× |
| **Black-Box Calls** | **260,128,449** (12,387.1/inst.) | **68,690,225** (3,271.0) | **56,221,158** (2,677.2) | 4.63× | 1.22× |
| – Preprocessing | 85,721 ms | 91,049 ms | 90,292 ms | 0.95× | 1.01× |
| – Black-box calls (Lipschitz) | 325,458 ms | 340,975 ms | 296,811 ms | 1.10× | 1.15× |
| – Arrangement estimation | 193,112 ms | 170,802 ms | 104,689 ms | 1.84× | 1.63× |
| – Arrangement algorithm | 2,252,401 ms | 137,817 ms | 101,391 ms | 22.21× | 1.36× |
| &nbsp;&nbsp;∗ Construction / 극대 집합 열거 | 1,637,404 ms | 81,955 ms | 55,203 ms | 29.66× | 1.48× |
| &nbsp;&nbsp;∗ Black-box calls | 459,523 ms | 54,653 ms | 45,351 ms | 10.13× | 1.21× |
| – (단계에 안 잡힌 시간) | 45,503 ms | 45,691 ms | 37,412 ms | | |

- 블랙박스 호출: 저자 코드 260,128,449회 — 논문 Table 4와 같다.
- Arrangement algorithm 단계 비중: 기존 77.6 %, candidate7 17.5 %, candidate8 16.1 %. 전처리·Lipschitz 호출·배열 추정의 비중은 candidate7 76.7 %, candidate8 78.0 %다.
- candidate8은 기저 사례만이 아니라 Lipschitz 호출과 배열 추정도 줄인다. 기저 사례 값이 조금 낮게 정해져 상자가 일찍 종료되기 때문이다([`README.md`](README.md) 3절).
- 단계에 안 잡힌 시간은 r5와 같은 Lipschitz 단계 타이머 결함(버려지는 상자에서 `FUT_BLACKBOX2`가 멈추지 않음)이다.

## 2. Sigspatial — [BKN20] Table 4 형식 (1,000 인스턴스 합, Table 4에는 없는 벤치마크)

| Algorithm | 기존 (저자 코드) | candidate7 | candidate8 | 기존 ÷ c8 | c7 ÷ c8 |
|---|--:|--:|--:|--:|--:|
| **Time** | **2,417,441 ms** (2,417.44 ms/inst.) | **85,606 ms** (85.61) | **68,562 ms** (68.56) | **35.26×** | 1.25× |
| **Black-Box Calls** | **13,235,168** (13,235.2/inst.) | **3,724,204** (3,724.2) | **3,039,109** (3,039.1) | 4.35× | 1.23× |
| – Preprocessing | 22,449 ms | 22,602 ms | 22,633 ms | 0.99× | 1.00× |
| – Black-box calls (Lipschitz) | 21,089 ms | 24,751 ms | 20,203 ms | 1.04× | 1.23× |
| – Arrangement estimation | 30,341 ms | 26,904 ms | 17,539 ms | 1.73× | 1.53× |
| – Arrangement algorithm | 2,340,266 ms | 7,782 ms | 5,455 ms | 429.04× | 1.43× |
| &nbsp;&nbsp;∗ Construction / 극대 집합 열거 | 2,307,450 ms | 4,470 ms | 3,031 ms | 761.27× | 1.47× |
| &nbsp;&nbsp;∗ Black-box calls | 24,700 ms | 3,191 ms | 2,339 ms | 10.56× | 1.36× |
| – (단계에 안 잡힌 시간) | 3,297 ms | 3,565 ms | 2,733 ms | | |

- 저자 코드의 총 시간은 소수의 긴 쌍이 좌우한다: 가장 긴 1쌍이 28.9 %, 4쌍이 82.9 %다. 전형적인 쌍은 3절의 인스턴스별 값으로 본다.
- 쌍 125 (`file-003586.dat`/`file-002157.dat`): 저자 코드 495.5 s, candidate7 0.71 s, candidate8 0.16 s.
- 쌍 379 (`file-018549.dat`/`file-006616.dat`): 저자 코드 697.9 s, candidate7 2.89 s, candidate8 3.42 s.
- 쌍 432 (`file-002502.dat`/`file-016674.dat`): 저자 코드 397.4 s, candidate7 2.10 s, candidate8 2.27 s.
- 쌍 544 (`file-005762.dat`/`file-017462.dat`): 저자 코드 413.6 s, candidate7 1.91 s, candidate8 1.70 s.

## 3. 인스턴스별 가속

기하평균의 95 % 신뢰구간은 인스턴스 부트스트랩 2,000회(seed 1)다. '빠른 인스턴스'는 분모 쪽 방식이 더 빠른 인스턴스다.

| 벤치마크 | 비교 | 인스턴스 | 기하평균 [95 % CI] | 중앙값 | 분모가 빠른 인스턴스 | 최소 | 최대 |
|---|---|--:|--:|--:|--:|--:|--:|
| Characters | 기존 ÷ candidate7 | 21,000 | **3.68×** [3.65, 3.71] | 3.87× | 19,952 (95.0 %) | 0.25× | 57.6× |
| Characters | 기존 ÷ candidate8 | 21,000 | **5.16×** [5.10, 5.23] | 4.46× | 20,180 (96.1 %) | 0.36× | 71.5× |
| Characters | candidate7 ÷ candidate8 | 21,000 | **1.40×** [1.39, 1.42] | 1.09× | 14,413 (68.6 %) | 0.27× | 18.4× |
| Sigspatial | 기존 ÷ candidate7 | 1,000 | **2.84×** [2.71, 3.00] | 2.63× | 928 (92.8 %) | 0.21× | 694.7× |
| Sigspatial | 기존 ÷ candidate8 | 1,000 | **3.50×** [3.30, 3.71] | 2.97× | 939 (93.9 %) | 0.66× | 3,141.8× |
| Sigspatial | candidate7 ÷ candidate8 | 1,000 | **1.23×** [1.19, 1.27] | 1.04× | 649 (64.9 %) | 0.47× | 10.2× |

## 4. 정확성

- **Characters**, candidate7 − 저자 코드: 최대 |차이| 1.35e-08, 중앙값 +2.35e-09, 10⁻⁷ 초과 0쌍 / 21,000.
- **Characters**, candidate8 − 저자 코드: 최대 |차이| 1.26e-08, 중앙값 -5.07e-09, 10⁻⁷ 초과 0쌍 / 21,000.
- **Sigspatial**, candidate7 − 저자 코드: 최대 |차이| 1.25e-08, 중앙값 +2.21e-09, 10⁻⁷ 초과 0쌍 / 1,000.
- **Sigspatial**, candidate8 − 저자 코드: 최대 |차이| 9.12e-09, 중앙값 -9.97e-10, 10⁻⁷ 초과 0쌍 / 1,000.
- Characters: 프로세스 630개 중 비정상 종료 0개.
- Sigspatial: 프로세스 3,000개 중 비정상 종료 0개.

결정 문제(Table 2 형식)는 r6에서 저자 코드를 다시 재지 않았다. candidate8은 4^ℓ 69,000질의에서 candidate7과 블랙박스 호출 수가 같고(차이 ≤ 4회), 시간 차이도 일관되지 않으므로([`README.md`](README.md) 4절), 저자 코드 대비 결정 문제 결과는 r5의 [`RESULTS_decider.md`](../r5_authors_vs_candidate7/RESULTS_decider.md)가 그대로 해당한다.

## LaTeX

```latex
\begin{table}[t]
\centering
\caption{Value computation on the 21{,}000 \texttt{characters\_full} instances of~\cite{BKN20}, in the format of their Table~4: LMF of the authors' code and with the proposed maximal-set enumeration (enumeration radius budgeted by the decider's slack), measured back-to-back on one core.}
\label{tab:lmf-profile-chars-c8}
\begin{tabular}{llrr}
\toprule
\textbf{Algorithm} & \multicolumn{2}{c}{\textbf{Time}} & \textbf{Black-Box Calls} \\
\midrule
LMF, authors' code & \multicolumn{2}{c}{2{,}902{,}195 ms} & 260{,}128{,}449 \\
      & \multicolumn{2}{c}{(138.2 ms/inst.)} & (12{,}387.1/inst.) \\
\cmidrule(r){2-3}
& -- Preprocessing & 85{,}721 ms \\
\cmidrule(r){2-3}
& -- Black-box calls (Lipschitz) & 325{,}458 ms \\
\cmidrule(r){2-3}
& -- Arrangement estimation & 193{,}112 ms \\
\cmidrule(r){2-3}
& -- Arrangement algorithm & 2{,}252{,}401 ms \\
& \hphantom{bla} * Construction & 1{,}637{,}404 ms \\
& \hphantom{bla} * Black-box calls & 459{,}523 ms \\
\midrule
LMF, proposed & \multicolumn{2}{c}{630{,}595 ms} & 56{,}221{,}158 \\
      & \multicolumn{2}{c}{(30.0 ms/inst.)} & (2{,}677.2/inst.) \\
\cmidrule(r){2-3}
& -- Preprocessing & 90{,}292 ms \\
\cmidrule(r){2-3}
& -- Black-box calls (Lipschitz) & 296{,}811 ms \\
\cmidrule(r){2-3}
& -- Arrangement estimation & 104{,}689 ms \\
\cmidrule(r){2-3}
& -- Arrangement algorithm & 101{,}391 ms \\
& \hphantom{bla} * Construction & 55{,}203 ms \\
& \hphantom{bla} * Black-box calls & 45{,}351 ms \\
\bottomrule
\end{tabular}
\end{table}
```

```latex
\begin{table}[t]
\centering
\caption{Value computation on the 1{,}000 Sigspatial decider pairs of~\cite{BKN20} (full 20{,}199-curve set), in the format of their Table~4. Same machine and protocol as the Characters table.}
\label{tab:lmf-profile-sig-c8}
\begin{tabular}{llrr}
\toprule
\textbf{Algorithm} & \multicolumn{2}{c}{\textbf{Time}} & \textbf{Black-Box Calls} \\
\midrule
LMF, authors' code & \multicolumn{2}{c}{2{,}417{,}441 ms} & 13{,}235{,}168 \\
      & \multicolumn{2}{c}{(2{,}417.4 ms/inst.)} & (13{,}235.2/inst.) \\
\cmidrule(r){2-3}
& -- Preprocessing & 22{,}449 ms \\
\cmidrule(r){2-3}
& -- Black-box calls (Lipschitz) & 21{,}089 ms \\
\cmidrule(r){2-3}
& -- Arrangement estimation & 30{,}341 ms \\
\cmidrule(r){2-3}
& -- Arrangement algorithm & 2{,}340{,}266 ms \\
& \hphantom{bla} * Construction & 2{,}307{,}450 ms \\
& \hphantom{bla} * Black-box calls & 24{,}700 ms \\
\midrule
LMF, proposed & \multicolumn{2}{c}{68{,}562 ms} & 3{,}039{,}109 \\
      & \multicolumn{2}{c}{(68.6 ms/inst.)} & (3{,}039.1/inst.) \\
\cmidrule(r){2-3}
& -- Preprocessing & 22{,}633 ms \\
\cmidrule(r){2-3}
& -- Black-box calls (Lipschitz) & 20{,}203 ms \\
\cmidrule(r){2-3}
& -- Arrangement estimation & 17{,}539 ms \\
\cmidrule(r){2-3}
& -- Arrangement algorithm & 5{,}455 ms \\
& \hphantom{bla} * Construction & 3{,}031 ms \\
& \hphantom{bla} * Black-box calls & 2{,}339 ms \\
\bottomrule
\end{tabular}
\end{table}
```

