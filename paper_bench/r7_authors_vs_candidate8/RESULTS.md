# r7: 저자 코드(GitLab) 대 candidate8 — 값 계산(LMF), [BKN20] Table 4 형식

기준선은 [BKN20] 저자가 공개한 코드(`authors_gitlab/`, GitLab 3bbb305)이고, 제안 방법은 `candidate8`(기본 설정 `MAXREGION=cech MAXREGION_EXACT=1 MAXREGION_SLACK=budget`, 소스는 수정판 f51bcbd)이다. r5와 같은 방식으로 쟀다: 한 코어(CPU 1)에 고정, 다른 코어는 비움, 두 방식을 묶음·쌍마다 별도 프로세스로 연달아 실행하고 순서를 번갈아 바꿈. 표는 논문 형식(합계와 인스턴스당 평균)이고, 인스턴스별 가속은 3절에 따로 둔다. 이 파일은 `scripts/analyze_r7.py`가 `raw/`에서 만든다.

- **Characters**: `characters_full` 21,000쌍 (`paper_bench/queries/characters_uci_lmf_pairs.txt`, 210묶음 × 100쌍). 측정 21,000 / 21,000쌍. `start 2026-10-31T12:00:13+09:00 cpu 1 arms gitlab candidate8`
- **Sigspatial**: 저자의 결정 문제 1,000쌍 전부 (`paper_bench/queries/sigspatial_pairs.txt`, 전체 20,199곡선). 측정 1,000 / 1,000쌍. 저자 코드는 5 GB 주소 공간 제한을 걸고 돌렸다. `start 2026-10-31T12:43:59+09:00 cpu 1 arms gitlab candidate8`

## 1. Characters — [BKN20] Table 4 형식 (21,000 인스턴스 합)

| Algorithm | 기존 (저자 코드) | 제안 (candidate8) | 비 |
|---|--:|--:|--:|
| *[BKN20] Table 4, 기존 (저자 기계)* | *2,938,512 ms (140.0 ms/inst.)* | | |
| *— 호출* | *260,128,449 (12,387.1/inst.)* | | |
| *— 전처리 / Lipschitz / 배열 추정* | *71,728 / 400,189 / 166,479 ms* | | |
| *— 배열 알고리즘 (Construction, Black-box calls)* | *2,250,493 ms (1,537,500, 545,442)* | | |
| **Time** | **2,141,994 ms** (102.00 ms/inst.) | **471,105 ms** (22.43 ms/inst.) | **4.55×** |
| **Black-Box Calls** | **260,128,449** (12,387.1/inst.) | **56,221,158** (2,677.2/inst.) | 4.63× |
| – Preprocessing | 70,248 ms | 69,591 ms | 1.01× |
| – Black-box calls (Lipschitz) | 232,434 ms | 213,874 ms | 1.09× |
| – Arrangement estimation | 153,950 ms | 85,461 ms | 1.80× |
| – Arrangement algorithm | 1,652,208 ms | 74,838 ms | 22.08× |
| &nbsp;&nbsp;∗ Construction / 극대 집합 열거 | 1,220,395 ms | 42,256 ms | 28.88× |
| &nbsp;&nbsp;∗ Black-box calls | 319,609 ms | 31,977 ms | 10.00× |

- 블랙박스 호출: 저자 코드 260,128,449회 — 논문 Table 4는 260,128,449회(같다).
- Arrangement algorithm 단계 비중: 기존 77.1 % → 제안 15.9 %. 바꾸지 않은 세 단계(전처리·Lipschitz 호출·배열 추정)가 제안 방법 시간의 78.3 %다.
- (2026-10-21) 네 단계의 합은 Time보다 작다: 저자 코드 / candidate8에서 Characters 33,155 / 27,342 ms, Sigspatial 2,442 / 1,956 ms. 대부분 Lipschitz 단계 타이머 결함(버려지는 상자에서 `FUT_BLACKBOX2`가 멈추지 않음, [`README.md`](../r5_authors_vs_candidate7/README.md#측정-주의--lipschitz-단계-타이머-2026-10-21))이고, 이를 Lipschitz 호출로 보면 위 비중은 약 84 %다.
- 제안 방법의 Construction 행은 CGAL 배열 구성 대신 극대 집합·증인점 열거 시간이고, 그 아래 Black-box calls는 증인점에서의 판정이다.

## 2. Sigspatial — [BKN20] Table 4 형식 (1,000 인스턴스 합, Table 4에는 없는 벤치마크)

| Algorithm | 기존 (저자 코드) | 제안 (candidate8) | 비 |
|---|--:|--:|--:|
| **Time** | **1,773,377 ms** (1,773.38 ms/inst.) | **53,212 ms** (53.21 ms/inst.) | **33.33×** |
| **Black-Box Calls** | **13,235,168** (13,235.2/inst.) | **3,039,118** (3,039.1/inst.) | 4.35× |
| – Preprocessing | 18,743 ms | 19,183 ms | 0.98× |
| – Black-box calls (Lipschitz) | 15,207 ms | 14,031 ms | 1.08× |
| – Arrangement estimation | 24,312 ms | 14,018 ms | 1.73× |
| – Arrangement algorithm | 1,712,673 ms | 4,026 ms | 425.45× |
| &nbsp;&nbsp;∗ Construction / 극대 집합 열거 | 1,688,745 ms | 2,329 ms | 725.06× |
| &nbsp;&nbsp;∗ Black-box calls | 17,895 ms | 1,632 ms | 10.97× |

- 저자 코드의 총 시간은 소수의 긴 쌍이 좌우한다: 가장 긴 1쌍이 26.0 %, 4쌍이 82.2 %다. 전형적인 쌍은 3절의 인스턴스별 값으로 본다.
- 쌍 125 (`file-003586.dat`/`file-002157.dat`): 저자 코드 367.8 s, candidate8 0.11 s, 값 5110.942581 / 5110.942581.
- 쌍 379 (`file-018549.dat`/`file-006616.dat`): 저자 코드 461.6 s, candidate8 1.83 s, 값 2527.563936 / 2527.563936.
- 쌍 432 (`file-002502.dat`/`file-016674.dat`): 저자 코드 311.6 s, candidate8 1.71 s, 값 3886.460162 / 3886.460162.
- 쌍 544 (`file-005762.dat`/`file-017462.dat`): 저자 코드 316.6 s, candidate8 1.31 s, 값 5524.748655 / 5524.748655.

## 3. 인스턴스별 가속 (기존 시간 ÷ 제안 시간)

기하평균의 95 % 신뢰구간은 인스턴스 부트스트랩 2,000회(seed 1)다.

| 벤치마크 | 인스턴스 | 기하평균 [95 % CI] | 중앙값 | 제안이 빠른 인스턴스 | 최소 | 최대 |
|---|--:|--:|--:|--:|--:|--:|
| Characters | 21,000 | **5.09×** [5.03, 5.15] | 4.29× | 20,496 (97.6 %) | 0.31× | 63.7× |
| Sigspatial | 1,000 | **3.24×** [3.07, 3.43] | 2.67× | 935 (93.5 %) | 0.73× | 3,279.0× |

## 4. 정확성

- **Characters**: |candidate8 − 저자 코드| 최대 1.26e-08, 10⁻⁷ 초과 0쌍 / 21,000. candidate8 − 저자 코드의 중앙값 -5.07e-09, 저자 코드보다 작은 쌍 18,130 / 큰 쌍 1,531.
- **Sigspatial**: |candidate8 − 저자 코드| 최대 9.12e-09, 10⁻⁷ 초과 0쌍 / 1,000. candidate8 − 저자 코드의 중앙값 -9.68e-10, 저자 코드보다 작은 쌍 558 / 큰 쌍 335.
- Characters: 프로세스 420개 중 비정상 종료 0개.
- Sigspatial: 프로세스 2,000개 중 비정상 종료 0개.

## 5. r5·r6과의 대조 (같은 코드, 같은 인스턴스)

블랙박스 호출 수와 값은 기계와 무관한 결정적 값이다. 시간 비는 같은 코드가 이 측정에서 얼마나 빨리/느리게 돌았는지를 보여 준다.

| 벤치마크 | 방식 | 비교 대상 | 쌍 | 호출 수 같음 | 값 같음 | 총 시간 비 (r7 ÷ 대상) |
|---|---|---|--:|--:|--:|--:|
| Characters | 저자 코드 | r5 | 21,000 | 21,000 | 21,000 | 0.823 |
| Characters | candidate8 | r6 2차 (f51bcbd) | 21,000 | 21,000 | 21,000 | 0.784 |
| Sigspatial | 저자 코드 | r5 | 1,000 | 1,000 | 1,000 | 0.822 |
| Sigspatial | candidate8 | r6 2차 (f51bcbd) | 1,000 | 1,000 | 1,000 | 0.802 |

## LaTeX

```latex
\begin{table}[t]
\centering
\caption{Value computation on the 21{,}000 \texttt{characters\_full} instances of~\cite{BKN20}, in the format of their Table~4: LMF of the authors' code and with the proposed maximal-set enumeration, measured back-to-back on one core.}
\label{tab:lmf-profile-chars}
\begin{tabular}{llrr}
\toprule
\textbf{Algorithm} & \multicolumn{2}{c}{\textbf{Time}} & \textbf{Black-Box Calls} \\
\midrule
LMF, authors' code & \multicolumn{2}{c}{2{,}141{,}994 ms} & 260{,}128{,}449 \\
      & \multicolumn{2}{c}{(102.0 ms/inst.)} & (12{,}387.1/inst.) \\
\cmidrule(r){2-3}
& -- Preprocessing & 70{,}248 ms \\
\cmidrule(r){2-3}
& -- Black-box calls (Lipschitz) & 232{,}434 ms \\
\cmidrule(r){2-3}
& -- Arrangement estimation & 153{,}950 ms \\
\cmidrule(r){2-3}
& -- Arrangement algorithm & 1{,}652{,}208 ms \\
& \hphantom{bla} * Construction & 1{,}220{,}395 ms \\
& \hphantom{bla} * Black-box calls & 319{,}609 ms \\
\midrule
LMF, proposed & \multicolumn{2}{c}{471{,}105 ms} & 56{,}221{,}158 \\
      & \multicolumn{2}{c}{(22.4 ms/inst.)} & (2{,}677.2/inst.) \\
\cmidrule(r){2-3}
& -- Preprocessing & 69{,}591 ms \\
\cmidrule(r){2-3}
& -- Black-box calls (Lipschitz) & 213{,}874 ms \\
\cmidrule(r){2-3}
& -- Arrangement estimation & 85{,}461 ms \\
\cmidrule(r){2-3}
& -- Arrangement algorithm & 74{,}838 ms \\
& \hphantom{bla} * Construction & 42{,}256 ms \\
& \hphantom{bla} * Black-box calls & 31{,}977 ms \\
\bottomrule
\end{tabular}
\end{table}
```

```latex
\begin{table}[t]
\centering
\caption{Value computation on the 1{,}000 Sigspatial decider pairs of~\cite{BKN20} (full 20{,}199-curve set), in the format of their Table~4. Same machine and protocol as the Characters table.}
\label{tab:lmf-profile-sig}
\begin{tabular}{llrr}
\toprule
\textbf{Algorithm} & \multicolumn{2}{c}{\textbf{Time}} & \textbf{Black-Box Calls} \\
\midrule
LMF, authors' code & \multicolumn{2}{c}{1{,}773{,}377 ms} & 13{,}235{,}168 \\
      & \multicolumn{2}{c}{(1{,}773.4 ms/inst.)} & (13{,}235.2/inst.) \\
\cmidrule(r){2-3}
& -- Preprocessing & 18{,}743 ms \\
\cmidrule(r){2-3}
& -- Black-box calls (Lipschitz) & 15{,}207 ms \\
\cmidrule(r){2-3}
& -- Arrangement estimation & 24{,}312 ms \\
\cmidrule(r){2-3}
& -- Arrangement algorithm & 1{,}712{,}673 ms \\
& \hphantom{bla} * Construction & 1{,}688{,}745 ms \\
& \hphantom{bla} * Black-box calls & 17{,}895 ms \\
\midrule
LMF, proposed & \multicolumn{2}{c}{53{,}212 ms} & 3{,}039{,}118 \\
      & \multicolumn{2}{c}{(53.2 ms/inst.)} & (3{,}039.1/inst.) \\
\cmidrule(r){2-3}
& -- Preprocessing & 19{,}183 ms \\
\cmidrule(r){2-3}
& -- Black-box calls (Lipschitz) & 14{,}031 ms \\
\cmidrule(r){2-3}
& -- Arrangement estimation & 14{,}018 ms \\
\cmidrule(r){2-3}
& -- Arrangement algorithm & 4{,}026 ms \\
& \hphantom{bla} * Construction & 2{,}329 ms \\
& \hphantom{bla} * Black-box calls & 1{,}632 ms \\
\bottomrule
\end{tabular}
\end{table}
```

