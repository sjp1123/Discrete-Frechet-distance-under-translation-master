# r5 — 저자 코드(GitLab) 대 candidate7, 값 계산(LMF)과 결정 문제

기존 구현은 [BKN20] 저자가 공개한 코드 `authors_gitlab/`이다(GitLab `anusser/frechet_distance_under_translation`, 커밋 3bbb305). 알고리즘 소스는 그대로이고, CGAL 5 빌드 이식과 측정 시계만 바꿨다. 제안 방법은 최종형 `candidate7`이다. 둘을 [BKN20]의 두 벤치마크에서 r4와 같은 방식으로 쟀다(2026-10-03 23:34 ~ 10-04 02:02 KST).

r4와 다른 것은 **기준선 하나**다. r4의 기준선 `original/`은 저자 코드에서 기저 사례를 바꾼 수정판이었다(`../authors_check/REPORT.md`). 표는 [BKN20] Table 4, Table 2 형식이고, 그림은 그 논문의 Figure 6 형식이다. 자세한 표는 [`RESULTS.md`](RESULTS.md)(값 계산)와 [`RESULTS_decider.md`](RESULTS_decider.md)(결정 문제)에 있다.

## 요약 — 값 계산 (LMF)

| 벤치마크 | 인스턴스 | 기존 (ms/inst.) | candidate7 (ms/inst.) | 총 시간 비 | 인스턴스별 기하평균 [95 % CI] | 중앙값 | 블랙박스 호출/inst. |
|---|--:|--:|--:|--:|--:|--:|---|
| Characters ([BKN20] Table 4의 21,000쌍) | 21,000 | 123.94 | 33.02 | **3.75×** | **3.72×** [3.70, 3.75] | 3.89× | 12,387.1 → 3,271.0 |
| Sigspatial (저자 결정 문제 1,000쌍, 전체 20,199곡선) | 1,000 | 2,158.58 | 74.00 | 29.17×\* | **2.83×** [2.70, 2.98] | 2.61× | 13,235.2 → 3,724.2 |

- **기존 구현의 블랙박스 호출 260,128,449회(12,387.1/쌍)는 [BKN20] Table 4와 같다.** 기준선이 논문을 낸 코드라는 근거다.
- \* Sigspatial의 총 시간 비는 기존 구현의 긴 쌍 몇 개가 좌우한다. 가장 긴 1쌍이 기존 총 시간의 26.1 %, 4쌍이 82.5 %다. 전형적인 쌍은 기하평균·중앙값으로 본다.
- Sigspatial 1,000쌍 전부를 쟀다. r4에서 기준선이 12 GB를 넘겨 빠졌던 2쌍(125·432번째)도 저자 코드는 5 GB 안에서 452.6 s, 388.2 s에 끝낸다. candidate7은 같은 쌍을 0.57 s, 2.32 s에 끝낸다.
- 제안 방법이 빠른 인스턴스: Characters 20,095 / 21,000 (95.7 %), Sigspatial 925 / 1,000 (92.5 %).
- 거리 값은 모든 쌍에서 10⁻⁷ 안에서 같다(최대 차이: Characters 1.35×10⁻⁸, Sigspatial 1.25×10⁻⁸).
- 배열 알고리즘 단계만 놓고 보면 Characters 16.5×, Sigspatial 314×다. Characters의 총 시간 비가 3.75×에 머무는 이유는 바꾸지 않은 세 단계(전처리, Lipschitz 호출, 배열 추정)가 제안 방법 시간의 76.7 %를 차지하기 때문이다.

## 요약 — 결정 문제 ([BKN20] Table 2 형식, 계수 1 ± 4^ℓ, 벤치마크마다 1,000쌍 × 23세트)

| 벤치마크 | 질의 | 기존 (ms/inst.) | candidate7 (ms/inst.) | 총 시간 비 | 인스턴스별 기하평균 [95 % CI] | 블랙박스 호출/inst. | 배열 알고리즘 단계 |
|---|--:|--:|--:|--:|--:|---|--:|
| same-characters | 23,000 | 16.13 | 8.02 | **2.01×** | 1.21× [1.20, 1.21] | 1,165.1 → 272.7 | 14.0× |
| all-characters | 23,000 | 23.41 | 9.62 | **2.43×** | 1.18× [1.17, 1.18] | 1,889.3 → 423.3 | 15.9× |
| Sigspatial | 23,000 | 45.81 | 35.53 | **1.29×** | 1.08× [1.07, 1.08] | 1,338.1 → 319.1 | 14.9× |

- 두 방법 모두 오답이 없고, 서로 답이 다른 질의도 없다(4^ℓ와 2^ℓ 각 69,000질의).
- 기존 구현의 호출 수는 [BKN20] Table 2와 −2.1~+1.6 % 안이다. 논문의 4^ℓ 질의 파일은 배포되지 않아서, 같은 1,000쌍에 논문 공식을 적용해 만들었다.
- 제안 방법이 바꾸는 것은 배열 알고리즘 단계뿐이고, 이 단계는 14–16배 줄었다. 총 시간 비는 바꾸지 않은 배열 추정 단계의 비중에 막힌다. 그 비중은 기존 시간의 35 %(same), 30 %(all), 76 %(Sigspatial)다.
- 저자가 배포한 2^ℓ 파일은 질의가 쉬워(질의당 0.3–1 ms) 1.16× / 1.08× / 1.07×다(`RESULTS_decider.md` 1·3절).

## 앞선 측정과의 관계

| | 기준선 | 측정 조건 | Characters LMF 총 / 기하평균 | Sigspatial LMF 총 / 기하평균 | 결정 문제 4^ℓ 총 (same / all / Sig) |
|---|---|---|---|---|---|
| r4 | `original/` (수정판) | 한 코어, 다른 코어 비움 | 3.70× / 3.51× | 32.81× (998쌍) / 2.45× | 1.84× / 2.18× / 1.19× |
| authors_check | 저자 코드 | 다른 코어도 측정 중 | 3.66× / 3.67× | 30.1× / 2.82× | 1.95× / 2.27× / 1.20× |
| **r5 (이 폴더)** | **저자 코드** | **한 코어, 다른 코어 비움** | **3.75× / 3.72×** | **29.17× / 2.83×** | **2.01× / 2.43× / 1.29×** |

- 블랙박스 호출 수, 거리 값, 결정 문제의 답은 authors_check 실행과 모든 인스턴스에서 같다. 두 방식, Characters 21,000쌍, Sigspatial 1,000쌍, 결정 문제 69,000질의 전부가 그렇다. 달라진 것은 시간뿐이다.
- 시간의 절댓값은 authors_check 때보다 약 10 % 길다(저자 코드 Characters 111.6 → 123.9 ms/쌍). 컨테이너가 다시 시작돼 다른 물리 기계에 놓였을 가능성이 있다. 그래서 절대 시간은 기계마다 다르다고 보고, 비교는 이 폴더 안의 비율로 한다.
- 실행 순서의 영향은 작다. Characters 쌍별 기하평균이 저자 코드를 먼저 돌린 묶음에서 3.70×, candidate7을 먼저 돌린 묶음에서 3.74×다.

## 그림

| 파일 | 내용 |
|---|---|
| `fig_scatter_characters.{pdf,png}` | Characters 21,000 인스턴스 (단일 칼럼, 3.35 in) |
| `fig_scatter_sigspatial.{pdf,png}` | Sigspatial 1,000 인스턴스 (단일 칼럼) |
| `fig_scatter.{pdf,png}` | 두 패널 (전체 폭) |

로그-로그 축에 인스턴스마다 점 하나를 찍었다. x는 저자 코드, y는 candidate7의 LMF 시간이고, 점선은 y = x다. 대각선 아래의 점이 candidate7이 더 빠른 인스턴스다. 한 패널 안에서는 두 축의 범위가 같다.

캡션 예시: *Running time of LMF in the authors' implementation (x) and with the proposed maximal-set enumeration (y) on every instance; dashed: equal time. Left: the 21,000 Characters instances of [2, Table 4]. Right: all 1,000 Sigspatial decider pairs of [2] on the full 20,199-curve set. Both panels were measured on the same machine.*

## 측정 환경과 방식

- **기계**: 서버 컨테이너, Intel Xeon @ 2.10 GHz, 2 vCPU, 7.8 GB.
- **소프트웨어**: Ubuntu 24.04, g++ 13.3, CMake 3.28, CGAL 5.6(GMPXX 백엔드), Boost 1.83, GMP 6.3 / MPFR 4.2.
- **빌드**: `paper_bench`를 각 방식의 소스로 빌드했다: `ARMS="authors_gitlab candidate7" bash paper_bench/r4_original_vs_candidate7/scripts/build.sh`. 플래그는 `RelWithDebInfo`이고, candidate7은 논문 설정 `MAXREGION=cech MAXREGION_EXACT=1 MAXREGION_SLACK=0`(= 기본값)으로 돌렸다.
- **실행 방식**:
  - 두 방식을 같은 코어(CPU 1)에서 별도 프로세스로 연달아 실행했고, 다른 코어는 비워 두었다.
  - 실행 단위는 Characters가 100쌍 묶음, Sigspatial이 쌍, 결정 문제가 질의 파일(1,000질의)이다.
  - 순서는 단위 번호의 홀짝으로 번갈아 바꿨다(짝수: 저자 코드 먼저).
  - 시계는 `steady_clock`이다.
  - Sigspatial의 저자 코드에는 5 GB 주소 공간 제한을 걸었다.
- **실행 시간**: `start 2026-10-03T23:33:59+09:00` / `all done 2026-10-04T02:02:00+09:00`. 프로세스 2,696개가 모두 정상 종료했다.

## 파일

| 경로 | 내용 |
|---|---|
| `RESULTS.md` | 값 계산: Table 4 형식 표(Characters, Sigspatial), 인스턴스별 가속, 정확성, LaTeX |
| `RESULTS_decider.md` | 결정 문제: Table 2 형식 표(세 벤치마크 × 4^ℓ·2^ℓ), 세트별 결과, LaTeX |
| `raw/characters_lmf_r5.tar.gz` | Characters 원시 CSV (`gitlab/`, `candidate7/`에 묶음 c000–c209, 묶음당 100행), `rc.txt`, `log.txt` |
| `raw/sigspatial_lmf_r5.tar.gz` | Sigspatial 원시 CSV (쌍 s0001–s1000), 쌍 목록, `rc.txt`, `log.txt` |
| `raw/decider_4l_r5.tar.gz`, `raw/decider_2l_r5.tar.gz` | 결정 문제 원시 CSV (질의 파일 69개 × 두 방식) |
| `raw/r5_log.txt` | 전체 실행 시작·끝 시각 |
| `scripts/run_r5.sh` | 측정 전체: `authors_check/scripts`의 `run_lmf3.sh`·`run_sig3.sh`·`run_dec3.sh`를 `ARMS="gitlab candidate7"`로 실행 |
| `scripts/run_dec2.sh` | 결정 문제 2^ℓ (`run_dec3.sh`와 같고 질의 파일만 다름) |
| `scripts/analyze_r5.py` | `raw/` → `RESULTS.md`, `RESULTS_decider.md` |
| `scripts/plot_scatter.py` | `raw/` → `fig_scatter*.{pdf,png}` |

`raw/`의 방식 이름 `gitlab`은 `authors_gitlab/`(저자 코드)이다.

## 재현

```
ARMS="authors_gitlab candidate7" bash paper_bench/r4_original_vs_candidate7/scripts/build.sh   # ~/b_pb_authors_gitlab, ~/b_pb_candidate7
python3 paper_data/convert_sigspatial.py                                                        # paper_data/sigspatial/data (git-ignored)
bash paper_bench/r5_authors_vs_candidate7/scripts/run_r5.sh                                     # ~/r5 (약 2시간 30분, CPU 1, 다른 코어는 비울 것)
# ~/r5/{chars,sig,dec4,dec2}를 raw/의 tar.gz로 묶은 뒤:
python3 paper_bench/r5_authors_vs_candidate7/scripts/analyze_r5.py
python3 paper_bench/r5_authors_vs_candidate7/scripts/plot_scatter.py
```
