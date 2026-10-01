# r4: 기존 구현(original) 대 candidate7 — 값 계산(LMF), 한 컨테이너에서 측정

기존 구현(`original`, [BKN20] 저자 코드)과 제안 방법의 최종형(`candidate7`)을 [BKN20]의 두 벤치마크에서 같은 기계·같은 방식으로 쟀다. 인스턴스당 1회 측정. 표는 [BKN20] Table 4 형식이다. 이 파일은 `scripts/analyze.py`가 `raw/`에서 만든다.

- **Characters**: `characters_full` 21,000쌍 (`paper_bench/queries/characters_uci_lmf_pairs.txt`, 210파일 × 100쌍, 하네스 순서).
- **Sigspatial**: 저자의 결정 문제 1,000쌍 (`paper_bench/queries/sigspatial_pairs.txt`, 전체 20,199곡선). 기존 구현은 12 GB를 넘는 2쌍(125·432번째, experiment_log §6.9)을 건너뛰었다. 표는 두 방법 모두 잰 998쌍이다.

**측정 환경** — 서버 컨테이너: Intel Xeon @ 2.10 GHz, 2 vCPU, 7.8 GB, Ubuntu 24.04, g++ 13.3, CMake 3.28, CGAL 5.6 (header-only, GMPXX 백엔드), Boost 1.83, 시스템 GMP 6.3 / MPFR 4.2. `paper_bench`를 arm별 소스로 빌드(`RelWithDebInfo`, `-include cstdint -include array -include cstddef`). 시계 `steady_clock`.

**측정 방식** — 두 arm을 같은 코어(CPU 1, `taskset`)에서 별도 프로세스로 연달아 실행하고 순서를 번갈아 바꿨다. Characters는 100쌍 묶음마다, Sigspatial은 쌍마다 한 프로세스이고, 묶음·쌍 번호가 짝수면 original이 먼저다. CPU 0은 비워 두었다. candidate7은 `MAXREGION=cech MAXREGION_EXACT=1 MAXREGION_SLACK=0`(= 기본값)으로 실행했다. 프로세스 2,418개 중 비정상 종료 0개, 건너뛴 실행 2개.

- Characters 실행 기록: `start 2026-09-30T15:38:46+09:00 2 cpus, pinned to 1` / `all done 2026-09-30T16:27:55+09:00`
- Sigspatial 실행 기록: `start 2026-09-30T19:08:57+09:00 2 cpus, pinned to 1` / `all done 2026-09-30T19:49:18+09:00`

## 1. Characters — [BKN20] Table 4 형식

21,000개 인스턴스 합. 괄호 안은 인스턴스당 평균. 비 = 기존 ÷ 제안.

| Algorithm | 기존 (original) | 제안 (candidate7) | 비 |
|---|--:|--:|--:|
| **Time** | **2,307,977 ms** (109.90 ms/inst.) | **624,084 ms** (29.72 ms/inst.) | **3.70×** |
| **Black-Box Calls** | **257,162,361** (12,245.8/inst.) | **68,690,225** (3,271.0/inst.) | 3.74× |
| – Preprocessing | 78,961 ms | 80,901 ms | 0.98× |
| – Black-box calls (Lipschitz) | 247,013 ms | 256,024 ms | 0.96× |
| – Arrangement estimation | 166,393 ms | 144,415 ms | 1.15× |
| – Arrangement algorithm | 1,780,120 ms | 107,752 ms | 16.52× |
| &nbsp;&nbsp;∗ Construction / 극대 집합 열거 | 1,344,153 ms | 65,520 ms | 20.52× |
| &nbsp;&nbsp;∗ Black-box calls | 306,026 ms | 41,288 ms | 7.41× |

- Arrangement algorithm 단계 비중: 기존 77.1 % → 제안 17.3 %. Construction 비중: 58.2 % → 10.5 % ([BKN20] 52.3 %).
- 바꾸지 않은 세 단계(전처리·Lipschitz 호출·배열 추정)가 제안 방법 시간의 77.1 %다.
- 제안 방법의 Construction 행은 CGAL 배열 구성 대신 극대 집합·증인점 열거 시간이고, 그 아래 Black-box calls는 증인점에서의 판정이다.

## 2. Sigspatial — [BKN20] Table 4 형식 (Table 4에는 없는 벤치마크)

998개 인스턴스 합. 괄호 안은 인스턴스당 평균. 비 = 기존 ÷ 제안. 기존 구현이 12 GB를 넘는 2쌍은 제외했다.

| Algorithm | 기존 (original) | 제안 (candidate7) | 비 |
|---|--:|--:|--:|
| **Time** | **2,328,530 ms** (2,333.20 ms/inst.) | **70,969 ms** (71.11 ms/inst.) | **32.81×** |
| **Black-Box Calls** | **12,554,186** (12,579.3/inst.) | **3,616,226** (3,623.5/inst.) | 3.47× |
| – Preprocessing | 20,296 ms | 20,401 ms | 0.99× |
| – Black-box calls (Lipschitz) | 18,525 ms | 18,460 ms | 1.00× |
| – Arrangement estimation | 33,003 ms | 23,103 ms | 1.43× |
| – Arrangement algorithm | 2,253,750 ms | 6,293 ms | 358.14× |
| &nbsp;&nbsp;∗ Construction / 극대 집합 열거 | 2,229,745 ms | 3,742 ms | 595.85× |
| &nbsp;&nbsp;∗ Black-box calls | 16,686 ms | 2,449 ms | 6.81× |

- Arrangement algorithm 단계 비중: 기존 96.8 % → 제안 8.9 %.
- 총 시간 비는 소수의 긴 쌍이 좌우한다: 기존 구현이 가장 오래 걸린 1쌍이 기존 총 시간의 32.6 %, 4쌍이 86.0 %다. 전형적인 쌍은 표 3의 기하평균·중앙값으로 본다.
- 기존 구현이 없는 쌍 125번 (`file-003586.dat`/`file-002157.dat`): candidate7 0.59 s, 값 5110.942581.
- 기존 구현이 없는 쌍 432번 (`file-002502.dat`/`file-016674.dat`): candidate7 2.38 s, 값 3886.460162.

## 3. 인스턴스별 가속비 (기존 시간 ÷ 제안 시간)

기하평균의 95 % 신뢰구간은 부트스트랩 2,000회. 굵은 행이 이번 측정(r4)이고, 나머지는 같은 쌍에 대한 이전 측정이다.

| 비교 | 쌍 | 기존 합 | 제안 합 | 총 시간 비 | 기하평균 [95 % CI] | 중앙값 | 제안이 더 빠른 쌍 |
|---|--:|--:|--:|--:|--:|--:|--:|
| **Characters r4 (이 컨테이너): original / candidate7** | 21,000 | 2,308.0 s | 624.1 s | 3.70× | 3.51× [3.48, 3.54] | 3.81× | 20,069 (95.6 %) |
| Characters r2 (초록, 컨테이너): original / candidate5 | 21,000 | 2,842.3 s | 643.9 s | 4.41× | 4.07× [4.04, 4.10] | 4.61× | 20,904 (99.5 %) |
| Characters WSL (이전 c7 측정): original / candidate7 | 21,000 | 3,618.3 s | 975.6 s | 3.71× | 3.56× [3.53, 3.60] | 3.85× | 19,951 (95.0 %) |
| **Sigspatial r4 (이 컨테이너): original / candidate7** | 998 | 2,328.5 s | 71.0 s | 32.81× | 2.45× [2.33, 2.57] | 2.29× | 907 (90.9 %) |
| Sigspatial r3 (WSL): original / candidate5 | 998 | 2,456.4 s | 65.6 s | 37.46× | 2.47× [2.36, 2.59] | 2.42× | 908 (91.0 %) |
| Sigspatial WSL (이전 c7 측정): original / candidate7 | 998 | 3,860.6 s | 86.2 s | 44.77× | 2.52× [2.40, 2.66] | 2.34× | 916 (91.8 %) |

## 4. 정확성과 재현성

- **Characters**: |candidate7 − original| 최대 9.27e-09, 10⁻⁷ 초과 0쌍 / 21,000. 이전 WSL 측정과 값·블랙박스 호출 수가 비트 단위로 같은 쌍: original 21,000 / 21,000, candidate7 21,000 / 21,000.
- **Sigspatial**: |candidate7 − original| 최대 8.46e-09, 10⁻⁷ 초과 0쌍 / 998. 이전 WSL 측정과 값·블랙박스 호출 수가 비트 단위로 같은 쌍: original 998 / 998, candidate7 1,000 / 1,000.
- Characters original은 초록 측정(r2)과도 21,000 / 21,000쌍에서 비트 단위로 같다. 블랙박스 호출 수는 기계와 무관한 결정적 값이다.

## 5. 초록·[BKN20]과의 대조 — Characters, 인스턴스당

| | 시간 (ms/inst.) | 블랙박스 호출/inst. | Construction 비중 | 가속 (총 시간 비) |
|---|--:|--:|--:|--:|
| [BKN20] Table 4, LMF (저자 기계) | 140.0 | 12,387.1 | 52.3 % | — |
| 초록 r2 기존 (original) | 135.35 | 12,245.8 | 60.1 % | — |
| 초록 r2 제안 (candidate5) | 30.66 | 3,146.1 | 6.6 % | 4.41× |
| **r4 기존 (original)** | **109.90** | **12,245.8** | **58.2 %** | — |
| **r4 제안 (candidate7)** | **29.72** | **3,271.0** | **10.5 %** | **3.70×** |

## 6. 초록의 4.41×와 달라진 이유 (추정)

- **실행 순서 효과는 없다**: 묶음별 가속비 기하평균이 original을 먼저 돌린 묶음 3.92×, candidate7을 먼저 돌린 묶음 3.93×.
- **기계 차이 (약 4.41× → 4.18×)**: 같은 original 코드의 시간이 초록 측정의 0.81배로 줄었는데, CGAL 배열 단계(0.80)가 바꾸지 않은 단계(전처리 0.91, Lipschitz 0.85, 배열 추정 0.87)보다 더 줄었다. 제안 방법은 시간 대부분이 바꾸지 않은 단계라 덜 빨라진다. 이 비율로 추정한 candidate5의 이 기계 시간은 약 553 s.
- **candidate5 → candidate7 (약 4.18× → 3.70×)**: candidate7이 추정 candidate5보다 1.13배 시간. 차이는 Arrangement algorithm 단계에 몰려 있다(r4 candidate7 107,752 ms vs 초록 candidate5 77,596 ms). 원인은 candidate6의 `union` 모드다(7절).
- 추정치는 candidate5를 이 기계에서 직접 잰 값이 아니라 단계별 시간 비율로 계산한 값이다.

## 7. 제안 방법의 블랙박스 호출 수 분해 — candidate5 대 candidate7 (Characters 21,000쌍)

카운터를 넣은 스크래치 사본(`scripts/bbcalls_split/`, 저장소 소스는 그대로)으로 candidate7의 실행 시 옵션을 하나씩 켜서 21,000쌍 전부를 돌렸다. 계측 빌드의 호출 수·값은 계측 전과 같다(A = 초록 r2 candidate5: 21,000 / 21,000쌍, B = r4 candidate7: 21,000 / 21,000쌍).

| 설정 | 전체 호출/inst. | Lipschitz 단계 | 기저 사례 | └ 게이트 `lessThan(max)` | └ 거리 탐색 | └ 그중 box family 2차 판정 |
|---|--:|--:|--:|--:|--:|--:|
| candidate5 (초록) | 3,146.1 | 2,740.4 | 405.7 | 243.9 | 161.8 | — |
| candidate7, `MAXREGION_FIX=none N6_RANGE=0` (candidate5 경로 + 감사 수정 A–H) | 3,146.1 | 2,740.4 | 405.7 | 243.9 | 161.8 | — |
| candidate7, `none`, `N6_RANGE=1` (+ 범위 수정) | 3,062.7 | 2,731.1 | 331.5 | 239.9 | 91.6 | — |
| candidate7, `union`, `N6_RANGE=0` (+ union만) | 3,370.9 | 2,740.0 | 630.9 | 421.4 | 209.6 | 217.5 |
| **candidate7 기본** (`union` + 범위 수정) | 3,271.0 | 2,731.0 | 539.9 | 414.2 | 125.8 | 204.4 |

- **감사 수정 A–H: 0.** candidate5 경로로 돌린 candidate7은 21,000 / 21,000쌍에서 candidate5와 값·호출 수가 같다.
- **범위 수정 `N6_RANGE`(candidate6): -83.4/inst.** 기저 사례의 거리 탐색이 [0, f(τ_start)] 대신 [ℓ_B, max]만 이분 탐색한다. 탐색 시작 때의 고정 평행이동 거리 평가(43.7/inst.)가 없어지고 탐침이 준다. 15,883쌍에서 줄고 1,747쌍에서 는다.
- **`union` 모드(candidate6의 건전성 수정): +208.3/inst.** candidate5의 증인점이 모두 NO일 때 상자 안 극대 집합(box family)의 증인점을 더 판정한다. 기저 사례 1,201,019개 중 게이트가 NO인 1,172,884개(97.7 %)가 거의 모두 이 판정을 치르고, 2차 판정 1,378,691회 × 평균 3.1개 증인점 = 204.4/inst.다. 19,626쌍에서 늘고 34쌍에서 준다.
- Lipschitz 단계는 거의 그대로다(2,740.4 → 2,731.0). 늘어난 호출은 모두 기저 사례에서 나온다(405.7 → 539.9).
- 2차 판정은 candidate5 기저 사례의 결함(상자 전체를 덮는 원판을 무시해 YES 상자를 NO로 판정할 수 있음)을 막는 비용이다. 이 데이터에서도 2차 판정이 YES를 찾아 게이트 YES가 늘었다(`N6_RANGE=1` 기준 26,865 → 28,135회). 기존 구현 대비 호출 감소는 3.89×(candidate5)에서 3.74×(candidate7)로 줄었다.

## LaTeX

```latex
\begin{table}[t]
\centering
\caption{Value computation on the 21{,}000 \texttt{characters\_full} instances of~\cite{BKN20}, in the format of their Table~4: LMF with the original arrangement construction (baseline) and with the proposed maximal-set enumeration (proposed), measured back-to-back on the same core, one measurement per instance.}
\label{tab:lmf-profile-chars}
\begin{tabular}{llrr}
\toprule
\textbf{Algorithm} & \multicolumn{2}{c}{\textbf{Time}} & \textbf{Black-Box Calls} \\
\midrule
LMF, baseline & \multicolumn{2}{c}{2{,}307{,}977 ms} & 257{,}162{,}361 \\
      & \multicolumn{2}{c}{(109.9 ms/inst.)} & (12{,}245.8/inst.) \\
\cmidrule(r){2-3}
& -- Preprocessing & 78{,}961 ms \\
\cmidrule(r){2-3}
& -- Black-box calls (Lipschitz) & 247{,}013 ms \\
\cmidrule(r){2-3}
& -- Arrangement estimation & 166{,}393 ms \\
\cmidrule(r){2-3}
& -- Arrangement algorithm & 1{,}780{,}120 ms \\
& \hphantom{bla} * Construction & 1{,}344{,}153 ms \\
& \hphantom{bla} * Black-box calls & 306{,}026 ms \\
\midrule
LMF, proposed & \multicolumn{2}{c}{624{,}084 ms} & 68{,}690{,}225 \\
      & \multicolumn{2}{c}{(29.7 ms/inst.)} & (3{,}271.0/inst.) \\
\cmidrule(r){2-3}
& -- Preprocessing & 80{,}901 ms \\
\cmidrule(r){2-3}
& -- Black-box calls (Lipschitz) & 256{,}024 ms \\
\cmidrule(r){2-3}
& -- Arrangement estimation & 144{,}415 ms \\
\cmidrule(r){2-3}
& -- Arrangement algorithm & 107{,}752 ms \\
& \hphantom{bla} * Construction & 65{,}520 ms \\
& \hphantom{bla} * Black-box calls & 41{,}288 ms \\
\bottomrule
\end{tabular}
\end{table}
```

```latex
\begin{table}[t]
\centering
\caption{Value computation on the Sigspatial decider pairs of~\cite{BKN20} (full 20{,}199-curve set), in the format of their Table~4, without the 2 pairs on which the baseline exceeds 12\,GB of memory. Same machine and protocol as the Characters table.}
\label{tab:lmf-profile-sig}
\begin{tabular}{llrr}
\toprule
\textbf{Algorithm} & \multicolumn{2}{c}{\textbf{Time}} & \textbf{Black-Box Calls} \\
\midrule
LMF, baseline & \multicolumn{2}{c}{2{,}328{,}530 ms} & 12{,}554{,}186 \\
      & \multicolumn{2}{c}{(2{,}333.2 ms/inst.)} & (12{,}579.3/inst.) \\
\cmidrule(r){2-3}
& -- Preprocessing & 20{,}296 ms \\
\cmidrule(r){2-3}
& -- Black-box calls (Lipschitz) & 18{,}525 ms \\
\cmidrule(r){2-3}
& -- Arrangement estimation & 33{,}003 ms \\
\cmidrule(r){2-3}
& -- Arrangement algorithm & 2{,}253{,}750 ms \\
& \hphantom{bla} * Construction & 2{,}229{,}745 ms \\
& \hphantom{bla} * Black-box calls & 16{,}686 ms \\
\midrule
LMF, proposed & \multicolumn{2}{c}{70{,}969 ms} & 3{,}616{,}226 \\
      & \multicolumn{2}{c}{(71.1 ms/inst.)} & (3{,}623.5/inst.) \\
\cmidrule(r){2-3}
& -- Preprocessing & 20{,}401 ms \\
\cmidrule(r){2-3}
& -- Black-box calls (Lipschitz) & 18{,}460 ms \\
\cmidrule(r){2-3}
& -- Arrangement estimation & 23{,}103 ms \\
\cmidrule(r){2-3}
& -- Arrangement algorithm & 6{,}293 ms \\
& \hphantom{bla} * Construction & 3{,}742 ms \\
& \hphantom{bla} * Black-box calls & 2{,}449 ms \\
\bottomrule
\end{tabular}
\end{table}
```

