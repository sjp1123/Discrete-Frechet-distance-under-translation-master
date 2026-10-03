# r5: 저자 코드(GitLab) 대 candidate7 — 값 계산(LMF), [BKN20] Table 4 형식

기준선은 [BKN20] 저자가 공개한 코드(`authors_gitlab/`, GitLab 3bbb305)이고, 제안 방법은 `candidate7`(논문 설정 `MAXREGION=cech MAXREGION_EXACT=1 MAXREGION_SLACK=0`)이다. r4와 같은 방식으로 쟀다: 한 코어(CPU 1)에 고정, 다른 코어는 비움, 두 방식을 묶음·쌍마다 별도 프로세스로 연달아 실행하고 순서를 번갈아 바꿈. 표는 논문 형식(합계와 인스턴스당 평균)이고, 인스턴스별 가속은 3절에 따로 둔다. 이 파일은 `scripts/analyze_r5.py`가 `raw/`에서 만든다.

- **Characters**: `characters_full` 21,000쌍 (`paper_bench/queries/characters_uci_lmf_pairs.txt`, 210묶음 × 100쌍). 측정 21,000 / 21,000쌍. `start 2026-10-03T23:33:59+09:00 cpu 1 arms gitlab candidate7`
- **Sigspatial**: 저자의 결정 문제 1,000쌍 전부 (`paper_bench/queries/sigspatial_pairs.txt`, 전체 20,199곡선). 측정 1,000 / 1,000쌍. 저자 코드는 5 GB 주소 공간 제한을 걸고 돌렸다. `start 2026-10-04T00:29:10+09:00 cpu 1 arms gitlab candidate7`

## 1. Characters — [BKN20] Table 4 형식 (21,000 인스턴스 합)

| Algorithm | 기존 (저자 코드) | 제안 (candidate7) | 비 |
|---|--:|--:|--:|
| *[BKN20] Table 4, 기존 (저자 기계)* | *2,938,512 ms (140.0 ms/inst.)* | | |
| *— 호출* | *260,128,449 (12,387.1/inst.)* | | |
| *— 전처리 / Lipschitz / 배열 추정* | *71,728 / 400,189 / 166,479 ms* | | |
| *— 배열 알고리즘 (Construction, Black-box calls)* | *2,250,493 ms (1,537,500, 545,442)* | | |
| **Time** | **2,602,660 ms** (123.94 ms/inst.) | **693,325 ms** (33.02 ms/inst.) | **3.75×** |
| **Black-Box Calls** | **260,128,449** (12,387.1/inst.) | **68,690,225** (3,271.0/inst.) | 3.79× |
| – Preprocessing | 84,342 ms | 87,248 ms | 0.97× |
| – Black-box calls (Lipschitz) | 276,681 ms | 282,474 ms | 0.98× |
| – Arrangement estimation | 182,162 ms | 162,355 ms | 1.12× |
| – Arrangement algorithm | 2,019,843 ms | 122,092 ms | 16.54× |
| &nbsp;&nbsp;∗ Construction / 극대 집합 열거 | 1,503,082 ms | 74,558 ms | 20.16× |
| &nbsp;&nbsp;∗ Black-box calls | 385,300 ms | 46,483 ms | 8.29× |

- 블랙박스 호출: 저자 코드 260,128,449회 — 논문 Table 4는 260,128,449회(같다).
- Arrangement algorithm 단계 비중: 기존 77.6 % → 제안 17.6 %. 바꾸지 않은 세 단계(전처리·Lipschitz 호출·배열 추정)가 제안 방법 시간의 76.7 %다.
- 제안 방법의 Construction 행은 CGAL 배열 구성 대신 극대 집합·증인점 열거 시간이고, 그 아래 Black-box calls는 증인점에서의 판정이다.

## 2. Sigspatial — [BKN20] Table 4 형식 (1,000 인스턴스 합, Table 4에는 없는 벤치마크)

| Algorithm | 기존 (저자 코드) | 제안 (candidate7) | 비 |
|---|--:|--:|--:|
| **Time** | **2,158,580 ms** (2,158.58 ms/inst.) | **74,001 ms** (74.00 ms/inst.) | **29.17×** |
| **Black-Box Calls** | **13,235,168** (13,235.2/inst.) | **3,724,204** (3,724.2/inst.) | 3.55× |
| – Preprocessing | 20,189 ms | 20,391 ms | 0.99× |
| – Black-box calls (Lipschitz) | 17,880 ms | 19,769 ms | 0.90× |
| – Arrangement estimation | 27,748 ms | 24,236 ms | 1.14× |
| – Arrangement algorithm | 2,089,895 ms | 6,666 ms | 313.54× |
| &nbsp;&nbsp;∗ Construction / 극대 집합 열거 | 2,062,496 ms | 3,854 ms | 535.21× |
| &nbsp;&nbsp;∗ Black-box calls | 20,614 ms | 2,711 ms | 7.60× |

- 저자 코드의 총 시간은 소수의 긴 쌍이 좌우한다: 가장 긴 1쌍이 26.1 %, 4쌍이 82.5 %다. 전형적인 쌍은 3절의 인스턴스별 값으로 본다.
- 쌍 125 (`file-003586.dat`/`file-002157.dat`): 저자 코드 452.6 s, candidate7 0.57 s, 값 5110.942581 / 5110.942581.
- 쌍 379 (`file-018549.dat`/`file-006616.dat`): 저자 코드 562.4 s, candidate7 2.14 s, 값 2527.563936 / 2527.563936.
- 쌍 432 (`file-002502.dat`/`file-016674.dat`): 저자 코드 388.2 s, candidate7 2.32 s, 값 3886.460162 / 3886.460162.
- 쌍 544 (`file-005762.dat`/`file-017462.dat`): 저자 코드 378.7 s, candidate7 1.50 s, 값 5524.748655 / 5524.748655.

## 3. 인스턴스별 가속 (기존 시간 ÷ 제안 시간)

기하평균의 95 % 신뢰구간은 인스턴스 부트스트랩 2,000회(seed 1)다.

| 벤치마크 | 인스턴스 | 기하평균 [95 % CI] | 중앙값 | 제안이 빠른 인스턴스 | 최소 | 최대 |
|---|--:|--:|--:|--:|--:|--:|
| Characters | 21,000 | **3.72×** [3.70, 3.75] | 3.89× | 20,095 (95.7 %) | 0.32× | 46.5× |
| Sigspatial | 1,000 | **2.83×** [2.70, 2.98] | 2.61× | 925 (92.5 %) | 0.29× | 800.5× |

## 4. 정확성

- **Characters**: |candidate7 − 저자 코드| 최대 1.35e-08, 10⁻⁷ 초과 0쌍 / 21,000.
- **Sigspatial**: |candidate7 − 저자 코드| 최대 1.25e-08, 10⁻⁷ 초과 0쌍 / 1,000.
- Characters: 프로세스 420개 중 비정상 종료 0개.
- Sigspatial: 프로세스 2,000개 중 비정상 종료 0개.

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
LMF, authors' code & \multicolumn{2}{c}{2{,}602{,}660 ms} & 260{,}128{,}449 \\
      & \multicolumn{2}{c}{(123.9 ms/inst.)} & (12{,}387.1/inst.) \\
\cmidrule(r){2-3}
& -- Preprocessing & 84{,}342 ms \\
\cmidrule(r){2-3}
& -- Black-box calls (Lipschitz) & 276{,}681 ms \\
\cmidrule(r){2-3}
& -- Arrangement estimation & 182{,}162 ms \\
\cmidrule(r){2-3}
& -- Arrangement algorithm & 2{,}019{,}843 ms \\
& \hphantom{bla} * Construction & 1{,}503{,}082 ms \\
& \hphantom{bla} * Black-box calls & 385{,}300 ms \\
\midrule
LMF, proposed & \multicolumn{2}{c}{693{,}325 ms} & 68{,}690{,}225 \\
      & \multicolumn{2}{c}{(33.0 ms/inst.)} & (3{,}271.0/inst.) \\
\cmidrule(r){2-3}
& -- Preprocessing & 87{,}248 ms \\
\cmidrule(r){2-3}
& -- Black-box calls (Lipschitz) & 282{,}474 ms \\
\cmidrule(r){2-3}
& -- Arrangement estimation & 162{,}355 ms \\
\cmidrule(r){2-3}
& -- Arrangement algorithm & 122{,}092 ms \\
& \hphantom{bla} * Construction & 74{,}558 ms \\
& \hphantom{bla} * Black-box calls & 46{,}483 ms \\
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
LMF, authors' code & \multicolumn{2}{c}{2{,}158{,}580 ms} & 13{,}235{,}168 \\
      & \multicolumn{2}{c}{(2{,}158.6 ms/inst.)} & (13{,}235.2/inst.) \\
\cmidrule(r){2-3}
& -- Preprocessing & 20{,}189 ms \\
\cmidrule(r){2-3}
& -- Black-box calls (Lipschitz) & 17{,}880 ms \\
\cmidrule(r){2-3}
& -- Arrangement estimation & 27{,}748 ms \\
\cmidrule(r){2-3}
& -- Arrangement algorithm & 2{,}089{,}895 ms \\
& \hphantom{bla} * Construction & 2{,}062{,}496 ms \\
& \hphantom{bla} * Black-box calls & 20{,}614 ms \\
\midrule
LMF, proposed & \multicolumn{2}{c}{74{,}001 ms} & 3{,}724{,}204 \\
      & \multicolumn{2}{c}{(74.0 ms/inst.)} & (3{,}724.2/inst.) \\
\cmidrule(r){2-3}
& -- Preprocessing & 20{,}391 ms \\
\cmidrule(r){2-3}
& -- Black-box calls (Lipschitz) & 19{,}769 ms \\
\cmidrule(r){2-3}
& -- Arrangement estimation & 24{,}236 ms \\
\cmidrule(r){2-3}
& -- Arrangement algorithm & 6{,}666 ms \\
& \hphantom{bla} * Construction & 3{,}854 ms \\
& \hphantom{bla} * Black-box calls & 2{,}711 ms \\
\bottomrule
\end{tabular}
\end{table}
```

