# r4 — 기존 구현(original) 대 candidate7, 값 계산(LMF) 재측정

기존 구현(`original`, [BKN20] 저자 코드)과 제안 방법 최종형(`candidate7`)을 [BKN20]의 두 벤치마크에서
**같은 서버 컨테이너·같은 방식**으로 잰 결과(2026-09-30). 표는 [BKN20] Table 4 형식이고, 그림은 그 논문의 Figure 6 형식이다.
자세한 표·검증·해석은 [`RESULTS.md`](RESULTS.md)에 있다.

## 요약

| 벤치마크 | 인스턴스 | 기존 (ms/inst.) | candidate7 (ms/inst.) | 총 시간 비 | 인스턴스별 기하평균 [95 % CI] | 중앙값 | 블랙박스 호출/inst. |
|---|--:|--:|--:|--:|--:|--:|---|
| Characters ([BKN20] Table 4의 21,000쌍) | 21,000 | 109.90 | 29.72 | **3.70×** | **3.51×** [3.48, 3.54] | 3.81× | 12,245.8 → 3,271.0 |
| Sigspatial (저자 결정 문제 1,000쌍, 전체 20,199곡선) | 998 | 2,333.20 | 71.11 | 32.81×\* | **2.45×** [2.33, 2.57] | 2.29× | 12,579.3 → 3,623.5 |

- \* Sigspatial의 총 시간 비는 기존 구현이 가장 느린 1쌍(기존 총 시간의 32.6 %)과 4쌍(86.0 %)이 좌우한다. 전형적인 쌍은 기하평균·중앙값으로 본다.
- Sigspatial의 나머지 2쌍(125·432번째)은 기존 구현이 12 GB를 넘겨 실행하지 않았다(experiment_log §6.9). candidate7은 각각 0.59 s, 2.38 s에 끝낸다.
- 거리 값은 모든 쌍에서 10⁻⁷ 안에서 같다(최대 차이: Characters 9.3×10⁻⁹, Sigspatial 8.5×10⁻⁹).
- 값과 블랙박스 호출 수는 이전 WSL 측정(`../results/raw_c7_timing.tar.gz`)과 두 벤치마크의 모든 쌍에서 비트 단위로 같다.
- 초록의 Characters 4.41×(candidate5, r2)와 다른 이유는 두 가지다. 측정 기계가 다르고, candidate5에서 candidate7로 오면서 `union` 모드가 들어갔다. candidate7의 호출 수가 candidate5보다 3.97 % 많은 이유도 이 모드다. 분해 결과는 `RESULTS.md` §6–7에 있다.

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
- 실행 시간: Characters 49분(15:38–16:27 KST), Sigspatial 40분(19:08–19:49 KST). 프로세스 2,418개가 모두 정상 종료했다.

## 파일

| 경로 | 내용 |
|---|---|
| `RESULTS.md` | 전체 결과: Table 4 형식 표(Characters, Sigspatial), 인스턴스별 가속비, 정확성, 초록·[BKN20]과의 대조, 호출 수 분해, LaTeX |
| `raw/raw_characters_lmf_r4.tar.gz` | Characters 원시 CSV (`original/`, `candidate7/`에 묶음 `c000`–`c209`, 묶음당 100행), `rc.txt`, `log.txt` |
| `raw/raw_sigspatial_lmf_r4.tar.gz` | Sigspatial 원시 CSV (쌍 `s0001`–`s1000`, original은 998개), `rc.txt`, `log.txt` |
| `raw/raw_characters_bbcalls_split.tar.gz` | 블랙박스 호출 분해 실행 5개(A–E)의 CSV, 21,000행씩 |
| `scripts/build.sh` | 두 arm의 `paper_bench` 빌드 |
| `scripts/run_characters.sh`, `scripts/run_sigspatial.sh` | 측정 |
| `scripts/analyze.py` | `raw/` → `RESULTS.md` |
| `scripts/plot_scatter.py` | `raw/` → `fig_scatter*.{pdf,png}` |
| `scripts/bbcalls_split/` | 카운터를 넣은 스크래치 사본을 만드는 계측 스크립트(`patch.py`, `inst_counters.h`)와 실행 스크립트(`run.sh`). 저장소 소스는 바꾸지 않는다 |

## 재현

```
bash paper_bench/r4_original_vs_candidate7/scripts/build.sh                 # ~/b_pb_original, ~/b_pb_candidate7
bash paper_bench/r4_original_vs_candidate7/scripts/run_characters.sh        # ~/r4     (약 50분)
python3 paper_data/convert_sigspatial.py                                    # paper_data/sigspatial/data (git-ignored)
bash paper_bench/r4_original_vs_candidate7/scripts/run_sigspatial.sh        # ~/r4sig  (약 40분)
bash paper_bench/r4_original_vs_candidate7/scripts/bbcalls_split/run.sh     # ~/inst/out (2코어에서 약 30분)
# 각 작업 디렉터리를 raw/의 tar.gz로 묶은 뒤:
python3 paper_bench/r4_original_vs_candidate7/scripts/analyze.py
python3 paper_bench/r4_original_vs_candidate7/scripts/plot_scatter.py
```
