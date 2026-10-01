# r4: 결정 문제 — 기존 구현(original) 대 candidate7, [BKN20] Table 2 형식

값 계산 r4(`RESULTS.md`)와 같은 서버 컨테이너·같은 방식으로 잰 결정 문제 결과다. 이 파일은 `scripts/analyze_decider.py`가 `raw/raw_decider_r4.tar.gz`에서 만든다.

- **인스턴스**: 벤치마크마다 저자의 곡선 쌍 1,000개 × 23세트. YES 세트는 δ = (δ*+ε)(1+b^ℓ), ℓ = −10…2이고 NO 세트는 δ = (δ*−ε)(1−b^ℓ), ℓ = −10…−1이다. 계수 밑 b는 두 가지다. b = 4(`paperq4`)는 논문 본문의 계수로, 저자가 인스턴스 파일을 배포하지 않아 같은 쌍에 이 저장소에서 만든 것이다. b = 2(`paperq`)는 저자가 배포한 파일 그대로다.
- **벤치마크**: same-characters, all-characters (Characters UCI 원본, 저자 번호), Sigspatial (전체 20,199곡선).
- **측정 방식**: 질의 파일(1,000질의)마다 두 arm을 같은 코어(CPU 1)에서 별도 프로세스로 연달아 실행하고, 순서는 파일 번호의 홀짝으로 번갈아 바꿨다(짝수: original 먼저). CPU 0은 비워 두었다. candidate7은 `MAXREGION=cech MAXREGION_EXACT=1 MAXREGION_SLACK=0`(= 기본값). 단계 타이머는 저자 `updateProfileDec`의 것(`FUT_PREPROCESSING1`, `FUT_BLACKBOX1`, `FUT_DISCSELECTION1`, `FUT_ARRANGEMENT1`과 하위 `FUT_N6_ARR`, `FUT_N6_FRECHET`)이고, 하위 행은 단계의 나머지(타이머 없는 부분)를 빼고 센다.
- **환경**: `RESULTS.md`와 같다. 프로세스 276개 중 비정상 종료 0개. `start 2026-09-30T23:55:34+09:00 2 cpus, pinned to 1` / `all done 2026-10-01T00:47:20+09:00`

## 1. 요약

| 계수 | 벤치마크 | 질의 | 기존 (ms/inst.) | candidate7 (ms/inst.) | 총 시간 비 | 인스턴스별 기하평균 [95 % CI] | 중앙값 | 블랙박스 호출/inst. | 오답 (기존 / 제안) | 답 불일치 |
|---|---|--:|--:|--:|--:|--:|--:|---|--:|--:|
| 4^ℓ | same-characters | 23,000 | 14.35 | 7.79 | **1.84×** | 1.19× [1.19, 1.20] | 1.07× | 997.0 → 272.7 | 0 / 0 | 0 |
| 4^ℓ | all-characters | 23,000 | 21.26 | 9.77 | **2.18×** | 1.14× [1.13, 1.15] | 1.02× | 1,616.3 → 423.3 | 0 / 0 | 0 |
| 4^ℓ | Sigspatial | 23,000 | 41.32 | 34.63 | **1.19×** | 1.06× [1.05, 1.06] | 1.03× | 1,146.4 → 319.1 | 0 / 0 | 0 |
| 2^ℓ | same-characters | 23,000 | 1.01 | 0.89 | **1.13×** | 1.01× [1.01, 1.02] | 1.00× | 69.2 → 54.0 | 0 / 0 | 0 |
| 2^ℓ | all-characters | 23,000 | 0.59 | 0.58 | **1.02×** | 0.99× [0.98, 0.99] | 0.98× | 61.4 → 58.1 | 0 / 0 | 0 |
| 2^ℓ | Sigspatial | 23,000 | 0.35 | 0.35 | **0.98×** | 1.00× [0.99, 1.00] | 0.99× | 24.2 → 22.8 | 0 / 0 | 0 |

- 결정 문제에서는 질의 대부분이 배열 단계 없이 Lipschitz 탐색만으로 끝난다. 그래서 인스턴스별 기하평균·중앙값은 1에 가깝고, 이득은 배열 단계가 무거운 δ* 근처 NO 질의에 몰린다(4절). 총 시간 비가 이 무거운 질의들의 이득을 반영한다.

## 2. [BKN20] Table 2 형식 — 4^ℓ (논문 본문의 계수, 주 결과)

### same-characters (23,000질의, 오답 기존 0 / 제안 0)

| Algorithm | 기존 (original) | 제안 (candidate7) | 비 |
|---|--:|--:|--:|
| *[BKN20] Table 2, 기존 (저자 기계)* | *429,623 ms (18.7 ms/inst.)* | | |
| *— 호출 / 전처리 / Lipschitz / 배열 추정* | *1,159.2/inst. / 5 / 44,312 / 157,780 ms* | | |
| *— 배열 알고리즘 (Construction, Black-box calls)* | *226,469 ms (148,898, 60,156)* | | |
| **Time** | **329,939 ms** (14.35 ms/inst.) | **179,137 ms** (7.79 ms/inst.) | **1.84×** |
| **Black-Box Calls** | **22,931,233** (997.0/inst.) | **6,272,563** (272.7/inst.) | 3.66× |
| – Preprocessing | 11 ms | 11 ms | – |
| – Black-box calls (Lipschitz) | 34,150 ms | 35,201 ms | 0.97× |
| – Arrangement estimation | 126,997 ms | 129,048 ms | 0.98× |
| – Arrangement algorithm | 167,835 ms | 14,141 ms | 11.87× |
| &nbsp;&nbsp;∗ Construction / 극대 집합 열거 | 124,397 ms | 8,554 ms | 14.54× |
| &nbsp;&nbsp;∗ Black-box calls | 30,813 ms | 5,427 ms | 5.68× |

- Arrangement algorithm 비중: 기존 50.9 % → 제안 7.9 %. 바꾸지 않은 Arrangement estimation이 기존 시간의 38.5 %다.
- [BKN20]의 배열 알고리즘 비중은 52.7 %, 배열 추정 비중은 36.7 %다(인스턴스 파일이 달라 시간·호출은 직접 비교하지 않는다).

### all-characters (23,000질의, 오답 기존 0 / 제안 0)

| Algorithm | 기존 (original) | 제안 (candidate7) | 비 |
|---|--:|--:|--:|
| *[BKN20] Table 2, 기존 (저자 기계)* | *628,043 ms (27.3 ms/inst.)* | | |
| *— 호출 / 전처리 / Lipschitz / 배열 추정* | *1,860.1/inst. / 5 / 50,462 / 191,177 ms* | | |
| *— 배열 알고리즘 (Construction, Black-box calls)* | *385,145 ms (237,043, 120,149)* | | |
| **Time** | **488,881 ms** (21.26 ms/inst.) | **224,617 ms** (9.77 ms/inst.) | **2.18×** |
| **Black-Box Calls** | **37,175,347** (1,616.3/inst.) | **9,734,920** (423.3/inst.) | 3.82× |
| – Preprocessing | 11 ms | 11 ms | – |
| – Black-box calls (Lipschitz) | 38,893 ms | 39,483 ms | 0.99× |
| – Arrangement estimation | 163,330 ms | 162,232 ms | 1.01× |
| – Arrangement algorithm | 285,252 ms | 21,785 ms | 13.09× |
| &nbsp;&nbsp;∗ Construction / 극대 집합 열거 | 207,523 ms | 13,321 ms | 15.58× |
| &nbsp;&nbsp;∗ Black-box calls | 55,677 ms | 8,222 ms | 6.77× |

- Arrangement algorithm 비중: 기존 58.3 % → 제안 9.7 %. 바꾸지 않은 Arrangement estimation이 기존 시간의 33.4 %다.
- [BKN20]의 배열 알고리즘 비중은 61.3 %, 배열 추정 비중은 30.4 %다(인스턴스 파일이 달라 시간·호출은 직접 비교하지 않는다).

### Sigspatial (23,000질의, 오답 기존 0 / 제안 0)

| Algorithm | 기존 (original) | 제안 (candidate7) | 비 |
|---|--:|--:|--:|
| *[BKN20] Table 2, 기존 (저자 기계)* | *1,207,560 ms (52.5 ms/inst.)* | | |
| *— 호출 / 전처리 / Lipschitz / 배열 추정* | *1,366.1/inst. / 5 / 43,861 / 913,266 ms* | | |
| *— 배열 알고리즘 (Construction, Black-box calls)* | *249,268 ms (155,332, 73,934)* | | |
| **Time** | **950,468 ms** (41.32 ms/inst.) | **796,400 ms** (34.63 ms/inst.) | **1.19×** |
| **Black-Box Calls** | **26,366,095** (1,146.4/inst.) | **7,339,507** (319.1/inst.) | 3.59× |
| – Preprocessing | 8 ms | 8 ms | – |
| – Black-box calls (Lipschitz) | 35,136 ms | 36,246 ms | 0.97× |
| – Arrangement estimation | 740,020 ms | 743,871 ms | 0.99× |
| – Arrangement algorithm | 173,828 ms | 15,019 ms | 11.57× |
| &nbsp;&nbsp;∗ Construction / 극대 집합 열거 | 127,015 ms | 9,299 ms | 13.66× |
| &nbsp;&nbsp;∗ Black-box calls | 32,753 ms | 5,479 ms | 5.98× |

- Arrangement algorithm 비중: 기존 18.3 % → 제안 1.9 %. 바꾸지 않은 Arrangement estimation이 기존 시간의 77.9 %다.
- [BKN20]의 배열 알고리즘 비중은 20.6 %, 배열 추정 비중은 75.6 %다(인스턴스 파일이 달라 시간·호출은 직접 비교하지 않는다).

## 3. [BKN20] Table 2 형식 — 2^ℓ (저자가 배포한 질의 파일)

### same-characters (23,000질의, 오답 기존 0 / 제안 0)

| Algorithm | 기존 (original) | 제안 (candidate7) | 비 |
|---|--:|--:|--:|
| **Time** | **23,116 ms** (1.01 ms/inst.) | **20,527 ms** (0.89 ms/inst.) | **1.13×** |
| **Black-Box Calls** | **1,590,691** (69.2/inst.) | **1,242,694** (54.0/inst.) | 1.28× |
| – Preprocessing | 6 ms | 6 ms | – |
| – Black-box calls (Lipschitz) | 9,281 ms | 9,583 ms | 0.97× |
| – Arrangement estimation | 10,106 ms | 10,444 ms | 0.97× |
| – Arrangement algorithm | 3,538 ms | 320 ms | 11.05× |
| &nbsp;&nbsp;∗ Construction / 극대 집합 열거 | 2,600 ms | 191 ms | 13.61× |
| &nbsp;&nbsp;∗ Black-box calls | 671 ms | 125 ms | 5.36× |

- Arrangement algorithm 비중: 기존 15.3 % → 제안 1.6 %. 바꾸지 않은 Arrangement estimation이 기존 시간의 43.7 %다.

### all-characters (23,000질의, 오답 기존 0 / 제안 0)

| Algorithm | 기존 (original) | 제안 (candidate7) | 비 |
|---|--:|--:|--:|
| **Time** | **13,671 ms** (0.59 ms/inst.) | **13,356 ms** (0.58 ms/inst.) | **1.02×** |
| **Black-Box Calls** | **1,412,302** (61.4/inst.) | **1,335,571** (58.1/inst.) | 1.06× |
| – Preprocessing | 7 ms | 7 ms | – |
| – Black-box calls (Lipschitz) | 7,200 ms | 7,526 ms | 0.96× |
| – Arrangement estimation | 5,490 ms | 5,576 ms | 0.98× |
| – Arrangement algorithm | 792 ms | 70 ms | 11.27× |
| &nbsp;&nbsp;∗ Construction / 극대 집합 열거 | 570 ms | 42 ms | 13.70× |
| &nbsp;&nbsp;∗ Black-box calls | 166 ms | 28 ms | 5.97× |

- Arrangement algorithm 비중: 기존 5.8 % → 제안 0.5 %. 바꾸지 않은 Arrangement estimation이 기존 시간의 40.2 %다.

### Sigspatial (23,000질의, 오답 기존 0 / 제안 0)

| Algorithm | 기존 (original) | 제안 (candidate7) | 비 |
|---|--:|--:|--:|
| **Time** | **8,015 ms** (0.35 ms/inst.) | **8,144 ms** (0.35 ms/inst.) | **0.98×** |
| **Black-Box Calls** | **557,540** (24.2/inst.) | **524,531** (22.8/inst.) | 1.06× |
| – Preprocessing | 6 ms | 5 ms | – |
| – Black-box calls (Lipschitz) | 2,795 ms | 2,953 ms | 0.95× |
| – Arrangement estimation | 4,875 ms | 5,082 ms | 0.96× |
| – Arrangement algorithm | 257 ms | 22 ms | 11.86× |
| &nbsp;&nbsp;∗ Construction / 극대 집합 열거 | 216 ms | 18 ms | 11.82× |
| &nbsp;&nbsp;∗ Black-box calls | 16 ms | 3 ms | – |

- Arrangement algorithm 비중: 기존 3.2 % → 제안 0.3 %. 바꾸지 않은 Arrangement estimation이 기존 시간의 60.8 %다.

## 4. 세트별 총 시간 비 — 4^ℓ

셀: 기존 ms/inst. → 제안 ms/inst. (총 시간 비). 가속은 δ*에 가까운 NO 세트(1 − 4^ℓ, ℓ ≤ −6)에 몰려 있다. YES 세트와 먼 NO 세트는 대부분 1× 안팎이고, 질의당 0.01 ms 미만인 세트의 비는 측정 잡음 수준이다.

| 세트 | same-characters | all-characters | Sigspatial |
|---|--:|--:|--:|
| 1 + 4^2 (YES) | 0.003 → 0.001 (1.81×) | 0.001 → 0.003 (0.52×) | 0.003 → 0.002 (1.89×) |
| 1 + 4^1 (YES) | 0.006 → 0.008 (0.71×) | 0.002 → 0.002 (0.92×) | 0.001 → 0.004 (0.39×) |
| 1 + 4^0 (YES) | 0.020 → 0.020 (0.99×) | 0.005 → 0.006 (0.92×) | 0.003 → 0.002 (1.19×) |
| 1 + 4^-1 (YES) | 0.040 → 0.045 (0.90×) | 0.016 → 0.020 (0.81×) | 0.006 → 0.005 (1.12×) |
| 1 + 4^-2 (YES) | 0.134 → 0.155 (0.86×) | 0.056 → 0.063 (0.89×) | 0.018 → 0.017 (1.04×) |
| 1 + 4^-3 (YES) | 0.370 → 0.458 (0.81×) | 0.198 → 0.204 (0.97×) | 0.067 → 0.071 (0.95×) |
| 1 + 4^-4 (YES) | 1.05 → 0.98 (1.08×) | 0.576 → 0.500 (1.15×) | 0.259 → 0.271 (0.96×) |
| 1 + 4^-5 (YES) | 2.51 → 2.55 (0.98×) | 1.10 → 1.18 (0.94×) | 0.959 → 0.957 (1.00×) |
| 1 + 4^-6 (YES) | 5.90 → 5.41 (1.09×) | 2.83 → 2.76 (1.02×) | 4.04 → 3.89 (1.04×) |
| 1 + 4^-7 (YES) | 9.09 → 8.93 (1.02×) | 6.06 → 5.81 (1.04×) | 10.39 → 10.27 (1.01×) |
| 1 + 4^-8 (YES) | 10.72 → 10.58 (1.01×) | 10.30 → 9.52 (1.08×) | 25.17 → 24.85 (1.01×) |
| 1 + 4^-9 (YES) | 10.85 → 10.59 (1.03×) | 13.12 → 12.35 (1.06×) | 51.99 → 51.46 (1.01×) |
| 1 + 4^-10 (YES) | 11.71 → 10.60 (1.11×) | 13.56 → 11.98 (1.13×) | 67.13 → 68.31 (0.98×) |
| 1 − 4^-1 (NO) | 0.006 → 0.006 (1.04×) | 0.004 → 0.005 (0.87×) | 0.001 → 0.001 (0.87×) |
| 1 − 4^-2 (NO) | 0.114 → 0.119 (0.96×) | 0.074 → 0.075 (0.98×) | 0.011 → 0.014 (0.80×) |
| 1 − 4^-3 (NO) | 0.471 → 0.465 (1.01×) | 0.364 → 0.374 (0.97×) | 0.082 → 0.101 (0.81×) |
| 1 − 4^-4 (NO) | 1.84 → 1.72 (1.07×) | 1.26 → 1.30 (0.97×) | 0.536 → 0.517 (1.04×) |
| 1 − 4^-5 (NO) | 9.17 → 6.98 (1.31×) | 5.45 → 4.76 (1.15×) | 3.55 → 3.40 (1.04×) |
| 1 − 4^-6 (NO) | 34.21 → 16.62 (2.06×) | 35.18 → 18.89 (1.86×) | 19.09 → 16.77 (1.14×) |
| 1 − 4^-7 (NO) | 51.95 → 23.27 (2.23×) | 80.79 → 33.62 (2.40×) | 76.88 → 65.10 (1.18×) |
| 1 − 4^-8 (NO) | 58.12 → 26.33 (2.21×) | 102.18 → 38.35 (2.66×) | 177.84 → 137.72 (1.29×) |
| 1 − 4^-9 (NO) | 62.22 → 27.41 (2.27×) | 109.09 → 40.07 (2.72×) | 236.18 → 188.51 (1.25×) |
| 1 − 4^-10 (NO) | 59.42 → 25.89 (2.29×) | 106.66 → 42.78 (2.49×) | 276.26 → 224.16 (1.23×) |

## 5. 이전 측정과의 비교

| 계수 | 벤치마크 | r4 (이 컨테이너) orig / c7 | WSL c7 측정 orig / c7 | WSL r3 orig / candidate5 |
|---|---|--:|--:|--:|
| 4^ℓ | same-characters | 1.84× (기하 1.19×) | 1.87× (기하 1.17×) | 1.97× (기하 1.24×) |
| 4^ℓ | all-characters | 2.18× (기하 1.14×) | 2.08× (기하 1.13×) | 2.23× (기하 1.20×) |
| 4^ℓ | Sigspatial | 1.19× (기하 1.06×) | 1.19× (기하 1.01×) | 1.26× (기하 1.06×) |
| 2^ℓ | same-characters | 1.13× (기하 1.01×) | — | 1.20× (기하 1.04×) |
| 2^ℓ | all-characters | 1.02× (기하 0.99×) | — | 1.06× (기하 1.01×) |
| 2^ℓ | Sigspatial | 0.98× (기하 1.00×) | — | 1.03× (기하 0.97×) |

- 답과 블랙박스 호출 수가 이전 WSL candidate7 측정(4^ℓ)과 같은 질의: original 69,000 / 69,000, candidate7 69,000 / 69,000. original은 WSL r3(4^ℓ·2^ℓ)과도 138,000 / 138,000질의에서 같다.
- WSL c7 측정은 스트림 8개를 동시에 돌렸고, r3는 original 대 candidate5다. 절대 시간은 기계끼리 비교하지 않는다.

## LaTeX (4^ℓ)

```latex
\begin{table}[t]
\centering
\caption{Decision problem on the instances of~\cite{BKN20} with distance factors $(1\pm4^{\ell})$, 23{,}000 queries per data set, one measurement each; baseline and proposed measured back-to-back on the same core. Times in ms summed over all queries; the sub-rows of the arrangement algorithm omit its untimed rest.}
\label{tab:decider-profile}
\begin{tabular}{lrrrrrr}
\toprule
& \multicolumn{2}{c}{same-characters} & \multicolumn{2}{c}{all-characters} & \multicolumn{2}{c}{Sigspatial} \\
\cmidrule(lr){2-3}\cmidrule(lr){4-5}\cmidrule(lr){6-7}
& baseline & proposed & baseline & proposed & baseline & proposed \\
\midrule
Time (ms) & 329{,}939 & 179{,}137 & 488{,}881 & 224{,}617 & 950{,}468 & 796{,}400 \\
\quad per query (ms) & 14.3 & 7.8 & 21.3 & 9.8 & 41.3 & 34.6 \\
Black-box calls & 22{,}931{,}233 & 6{,}272{,}563 & 37{,}175{,}347 & 9{,}734{,}920 & 26{,}366{,}095 & 7{,}339{,}507 \\
-- Preprocessing & 11 & 11 & 11 & 11 & 8 & 8 \\
-- Black-box calls (Lipschitz) & 34{,}150 & 35{,}201 & 38{,}893 & 39{,}483 & 35{,}136 & 36{,}246 \\
-- Arrangement estimation & 126{,}997 & 129{,}048 & 163{,}330 & 162{,}232 & 740{,}020 & 743{,}871 \\
-- Arrangement algorithm & 167{,}835 & 14{,}141 & 285{,}252 & 21{,}785 & 173{,}828 & 15{,}019 \\
\quad * Construction & 124{,}397 & 8{,}554 & 207{,}523 & 13{,}321 & 127{,}015 & 9{,}299 \\
\quad * Black-box calls & 30{,}813 & 5{,}427 & 55{,}677 & 8{,}222 & 32{,}753 & 5{,}479 \\
\bottomrule
\end{tabular}
\end{table}
```
