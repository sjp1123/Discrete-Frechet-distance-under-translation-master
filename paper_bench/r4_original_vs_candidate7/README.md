# r4 — 기존 구현(original) 대 candidate7, 값 계산(LMF)과 결정 문제 재측정

> **정정 (2026-10-03):** 이 문서의 기준선 `original`은 [BKN20] 저자 코드 그대로가 아니다. `original/`의 `src/frechet_under_translation.cpp`·`src/fut_n6_algorithm.cpp`가 기저 사례에서 탐색 상자를 빼도록 수정되어 있다. 저자 코드([`authors_gitlab/`](../../authors_gitlab/), GitLab 3bbb305) 기준 수치는 [`paper_bench/authors_check/REPORT.md`](../authors_check/REPORT.md) 4절을 본다. 아래 수치는 수정판 기준 기록으로 남겨 둔다.

기존 구현(`original`, [BKN20] 저자 코드)과 제안 방법 최종형(`candidate7`)을 [BKN20]의 두 벤치마크에서
**같은 서버 컨테이너·같은 방식**으로 잰 결과(2026-09-30 ~ 10-01). 값 계산 표는 [BKN20] Table 4 형식, 결정 문제 표는 Table 2 형식이고,
그림은 그 논문의 Figure 6 형식이다. 자세한 표·검증·해석은 [`RESULTS.md`](RESULTS.md)(값 계산)와
[`RESULTS_decider.md`](RESULTS_decider.md)(결정 문제)에 있다.

## 요약 — 값 계산 (LMF)

| 벤치마크 | 인스턴스 | 기존 (ms/inst.) | candidate7 (ms/inst.) | 총 시간 비 | 인스턴스별 기하평균 [95 % CI] | 중앙값 | 블랙박스 호출/inst. |
|---|--:|--:|--:|--:|--:|--:|---|
| Characters ([BKN20] Table 4의 21,000쌍) | 21,000 | 109.90 | 29.72 | **3.70×** | **3.51×** [3.48, 3.54] | 3.81× | 12,245.8 → 3,271.0 |
| Sigspatial (저자 결정 문제 1,000쌍, 전체 20,199곡선) | 998 | 2,333.20 | 71.11 | 32.81×\* | **2.45×** [2.33, 2.57] | 2.29× | 12,579.3 → 3,623.5 |

- \* Sigspatial의 총 시간 비는 기존 구현이 가장 느린 1쌍(기존 총 시간의 32.6 %)과 4쌍(86.0 %)이 좌우한다. 전형적인 쌍은 기하평균·중앙값으로 본다.
- Sigspatial의 나머지 2쌍(125·432번째)은 기존 구현이 12 GB를 넘겨 실행하지 않았다(experiment_log §6.9). (정정: 수정판의 문제다. 저자 코드는 이 두 쌍을 5 GB 안에서 497.5 s, 338.0 s에 정답으로 끝낸다 — [`authors_check/REPORT.md`](../authors_check/REPORT.md) 4.2) candidate7은 각각 0.59 s, 2.38 s에 끝낸다.
- 거리 값은 모든 쌍에서 10⁻⁷ 안에서 같다(최대 차이: Characters 9.3×10⁻⁹, Sigspatial 8.5×10⁻⁹).
- 값과 블랙박스 호출 수는 이전 WSL 측정(`../results/raw_c7_timing.tar.gz`)과 두 벤치마크의 모든 쌍에서 비트 단위로 같다.
- 초록의 Characters 4.41×(candidate5, r2)와 다른 이유는 두 가지다. 측정 기계가 다르고, candidate5에서 candidate7로 오면서 `union` 모드가 들어갔다. candidate7의 호출 수가 candidate5보다 3.97 % 많은 이유도 이 모드다. 분해 결과는 `RESULTS.md` §6–7에 있다.

## 요약 — 결정 문제 ([BKN20] Table 2 형식, 계수 1 ± 4^ℓ, 벤치마크마다 1,000쌍 × 23세트)

| 벤치마크 | 질의 | 기존 (ms/inst.) | candidate7 (ms/inst.) | 총 시간 비 | 인스턴스별 기하평균 [95 % CI] | 블랙박스 호출/inst. | 배열 알고리즘 단계 |
|---|--:|--:|--:|--:|--:|---|--:|
| same-characters | 23,000 | 14.35 | 7.79 | **1.84×** | 1.19× [1.19, 1.20] | 997.0 → 272.7 | 11.9× |
| all-characters | 23,000 | 21.26 | 9.77 | **2.18×** | 1.14× [1.13, 1.15] | 1,616.3 → 423.3 | 13.1× |
| Sigspatial | 23,000 | 41.32 | 34.63 | **1.19×** | 1.06× [1.05, 1.06] | 1,146.4 → 319.1 | 11.6× |

- 두 방법 모두 오답 0, 서로 답이 다른 질의 0 (138,000질의씩, 2^ℓ 세트 포함).
- 제안 방법이 바꾸는 것은 배열 알고리즘 단계뿐이고, 이 단계는 세 벤치마크에서 11.6–13.1배 줄었다. 총 시간 비는 바꾸지 않은 배열 추정 단계의 비중(Characters 33–39 %, Sigspatial 78 %)에 막힌다.
- 이득은 δ*에 가까운 NO 세트에 몰린다: 1 − 4^ℓ (ℓ ≤ −6)에서 Characters 1.9–2.7×, Sigspatial 1.1–1.3×. 나머지 세트는 1× 안팎이라 인스턴스별 기하평균은 1에 가깝다.
- 저자가 배포한 2^ℓ 파일은 질의가 쉬워(질의당 0.35–1 ms) 1.13× / 1.02× / 0.98×다.
- 답과 호출 수는 이전 WSL candidate7 측정과 69,000질의 모두 같고, 총 시간 비도 그때(1.87× / 2.08× / 1.19×)와 같은 수준이다.

## 정확성 검증 (`verification/`)

두 구현을 새로 짠 독립 정확 오라클(17,553 인스턴스, 결정 질의 241,434), 실제 데이터 앵커(저자 δ*·번역 인증서, 5,100쌍),
임계값 근처 질의 44,000, ASan/UBSan, 기존 감사 키트, 회귀·단위 테스트, 독립 코드 리뷰로 검증했다. candidate7은 측정 경로에서 오류 0,
original은 무작위 입력에서 무한 루프·틀린 값·assert가 재현된다(논문 수치에는 영향 없음). 자세한 내용은
[`verification/REPORT.md`](verification/REPORT.md).

## 그림

| 파일 | 내용 |
|---|---|
| `fig_scatter_characters.{pdf,png}` | Characters 21,000 인스턴스 (단일 칼럼, 3.35 in) |
| `fig_scatter_sigspatial.{pdf,png}` | Sigspatial 998 인스턴스 (단일 칼럼) |
| `fig_scatter.{pdf,png}` | 두 패널 (전체 폭) |

`../figures/`와 같은 형식이다. 로그-로그 축에 인스턴스마다 점 하나를 찍고, 점선으로 y = x를 그렸다. x는 기존 구현, y는 candidate7의 LMF 시간이다.
대각선 아래의 점은 candidate7이 더 빠른 인스턴스다. 한 패널 안에서는 두 축의 범위가 같고, 패널끼리는 범위가 다르다.
`../figures/`와 달리 **두 패널 모두 같은 기계에서 쟀다**.

캡션 예시: *Running time of LMF with the original arrangement construction (x) and with the proposed
maximal-set enumeration (y) on every instance; dashed: equal time. Left: the 21,000 Characters instances of
[2, Table 4]. Right: the Sigspatial decider pairs of [2] on the full 20,199-curve set, without the 2 pairs on
which the original implementation exceeded 12 GB of memory. Both panels were measured on the same machine.*

## 측정 환경과 방식

- 서버 컨테이너: Intel Xeon @ 2.10 GHz, 2 vCPU, 7.8 GB, Ubuntu 24.04, g++ 13.3, CMake 3.28, CGAL 5.6(GMPXX 백엔드), Boost 1.83, GMP 6.3 / MPFR 4.2.
- `paper_bench`를 arm별 소스로 빌드했다(`RelWithDebInfo`, `../build.sh`와 같은 플래그). candidate7은 기본 설정(`MAXREGION_EXACT=1 MAXREGION_SLACK=0`)으로 실행했다.
- 두 arm을 같은 코어(CPU 1)에서 별도 프로세스로 연달아 실행했고, 순서는 묶음·쌍 번호의 홀짝으로 번갈아 바꿨다. 다른 코어는 비워 두었다. Characters는 100쌍 묶음 단위로, Sigspatial은 쌍 단위로 프로세스를 띄웠다. 시계는 `steady_clock`이다.
- 결정 문제는 질의 파일(1,000질의) 하나마다 두 arm을 연달아 실행했고, 순서는 파일 번호의 홀짝으로 바꿨다.
- 실행 시간: Characters 49분(09-30 15:38–16:27 KST), Sigspatial 40분(19:08–19:49), 결정 문제 52분(09-30 23:55 – 10-01 00:47). 프로세스 2,694개가 모두 정상 종료했다.

## 파일

| 경로 | 내용 |
|---|---|
| `RESULTS.md` | 값 계산: Table 4 형식 표(Characters, Sigspatial), 인스턴스별 가속비, 정확성, 초록·[BKN20]과의 대조, 호출 수 분해, LaTeX |
| `RESULTS_decider.md` | 결정 문제: Table 2 형식 표(세 벤치마크 × 4^ℓ·2^ℓ), 세트별 가속비, 이전 측정과의 비교, LaTeX |
| `raw/raw_characters_lmf_r4.tar.gz` | Characters 원시 CSV (`original/`, `candidate7/`에 묶음 `c000`–`c209`, 묶음당 100행), `rc.txt`, `log.txt` |
| `raw/raw_sigspatial_lmf_r4.tar.gz` | Sigspatial 원시 CSV (쌍 `s0001`–`s1000`, original은 998개), `rc.txt`, `log.txt` |
| `raw/raw_decider_r4.tar.gz` | 결정 문제 원시 CSV (`original/`, `candidate7/`에 질의 파일 138개, 파일당 1,000행), `rc.txt`, `log.txt` |
| `raw/raw_characters_bbcalls_split.tar.gz` | 블랙박스 호출 분해 실행 5개(A–E)의 CSV, 21,000행씩 |
| `scripts/build.sh` | 두 arm의 `paper_bench` 빌드 |
| `scripts/run_characters.sh`, `scripts/run_sigspatial.sh`, `scripts/run_decider.sh` | 측정 |
| `scripts/analyze.py`, `scripts/analyze_decider.py` | `raw/` → `RESULTS.md`, `RESULTS_decider.md` |
| `scripts/plot_scatter.py` | `raw/` → `fig_scatter*.{pdf,png}` |
| `verification/` | 정확성 검증: 보고서(`REPORT.md`), 오라클·생성기·실행·판정 스크립트(`scripts/`), 결과 표(`results/`), 원시 데이터(`raw/`) |
| `scripts/bbcalls_split/` | 카운터를 넣은 스크래치 사본을 만드는 계측 스크립트(`patch.py`, `inst_counters.h`)와 실행 스크립트(`run.sh`). 저장소 소스는 바꾸지 않는다 |

## 재현

```
bash paper_bench/r4_original_vs_candidate7/scripts/build.sh                 # ~/b_pb_original, ~/b_pb_candidate7
bash paper_bench/r4_original_vs_candidate7/scripts/run_characters.sh        # ~/r4     (약 50분)
python3 paper_data/convert_sigspatial.py                                    # paper_data/sigspatial/data (git-ignored)
bash paper_bench/r4_original_vs_candidate7/scripts/run_sigspatial.sh        # ~/r4sig  (약 40분)
bash paper_bench/r4_original_vs_candidate7/scripts/run_decider.sh          # ~/r4dec  (약 55분)
bash paper_bench/r4_original_vs_candidate7/scripts/bbcalls_split/run.sh     # ~/inst/out (2코어에서 약 30분)
# 각 작업 디렉터리를 raw/의 tar.gz로 묶은 뒤:
python3 paper_bench/r4_original_vs_candidate7/scripts/analyze.py
python3 paper_bench/r4_original_vs_candidate7/scripts/analyze_decider.py
python3 paper_bench/r4_original_vs_candidate7/scripts/plot_scatter.py
```
