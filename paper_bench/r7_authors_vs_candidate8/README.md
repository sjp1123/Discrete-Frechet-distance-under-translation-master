# r7 — 저자 코드 대 candidate8(수정판), 값 계산과 결정 문제를 한 측정에서 다시

r6의 저자 코드 대 candidate8 표에는 엄밀히 같은 측정이 아닌 부분이 두 가지 있었다.

- 값 계산 표의 candidate8은 수정 전 커밋(e264dd4)이었다.
- 결정 문제의 저자 코드는 r5 측정을 그대로 썼다.

r7은 저자 코드와 수정판 candidate8(소스 = 커밋 f51bcbd, master d5850c5)만 두고, r5의 측정 네 가지를 **한 번의 연속 측정**으로 다시 쟀다(2026-10-31 12:00–14:06).

- 값 계산: Characters 21,000쌍, Sigspatial 1,000쌍
- 결정 문제: 4^ℓ, 2^ℓ 세트 각 69,000질의

논문의 표·그림·초록 수치는 이 폴더 값을 쓴다. 자세한 표는 [`RESULTS.md`](RESULTS.md)(값 계산)와 [`RESULTS_decider.md`](RESULTS_decider.md)(결정 문제)에 있다.

## 요약

1. **값 계산(LMF)**, [BKN20] Table 4 형식

   | 벤치마크 | 인스턴스 | 저자 코드 (ms/inst.) | candidate8 (ms/inst.) | 총 시간 비 | 인스턴스별 기하평균 [95 % CI] | 중앙값 | candidate8이 빠른 인스턴스 | 블랙박스 호출/inst. |
   |---|--:|--:|--:|--:|--:|--:|--:|---|
   | Characters | 21,000 | 102.00 | 22.43 | **4.55×** | **5.09×** [5.03, 5.15] | 4.29× | 20,496 (97.6 %) | 12,387.1 → 2,677.2 (4.63×) |
   | Sigspatial | 1,000 | 1,773.38 | 53.21 | 33.33×\* | **3.24×** [3.07, 3.43] | 2.67× | 935 (93.5 %) | 13,235.2 → 3,039.1 (4.35×) |

   \* 저자 코드의 가장 긴 1쌍이 저자 코드 총 시간의 26.0 %, 4쌍이 82.2 %다. 그래서 Sigspatial은 기하평균을 앞세운다.

2. **결정 문제**, [BKN20] Table 2 형식. 계수 1 ± 4^ℓ가 주 결과다.

   | 벤치마크 | 질의 | 저자 코드 (ms/inst.) | candidate8 (ms/inst.) | 총 시간 비 | 인스턴스별 기하평균 [95 % CI] | 블랙박스 호출/inst. |
   |---|--:|--:|--:|--:|--:|---|
   | same-characters | 23,000 | 14.81 | 7.37 | **2.01×** | 1.24× [1.23, 1.25] | 1,165.1 → 272.7 (4.27×) |
   | all-characters | 23,000 | 21.54 | 9.01 | **2.39×** | 1.20× [1.19, 1.20] | 1,889.3 → 423.3 (4.46×) |
   | Sigspatial | 23,000 | 42.64 | 34.04 | **1.25×** | 1.08× [1.08, 1.09] | 1,338.1 → 319.1 (4.19×) |

   - 저자가 배포한 2^ℓ 파일은 질의당 0.3–1 ms라 1.19× / 0.98× / 1.08×다. 아래 3절을 본다.
3. **정확성**
   - 결정 문제 138,000질의에서 두 방식 모두 오답이 0이고, 서로 답이 다른 질의도 없다.
   - 값은 모든 쌍에서 10⁻⁷ 안이다(최대 차이 Characters 1.26e-8, Sigspatial 9.12e-9).
   - candidate8 − 저자 코드의 중앙값은 Characters −5.07e-9, Sigspatial −9.7e-10이다. candidate8은 저자 코드보다 조금 아래로 간다. 원인은 열거 여유다([`candidate8/README.candidate8.md`](../../candidate8/README.candidate8.md) 3.4). 논문에 함께 적는다.
   - 비정상 종료는 0개다(프로세스 2,696개).
4. **같은 코드가 같은 일을 했다**
   - 저자 코드의 블랙박스 호출 수와 값은 21,000 / 21,000쌍, 1,000 / 1,000쌍 모두 r5와 같다. Characters 합계 260,128,449회는 [BKN20] Table 4와 같다.
   - candidate8의 호출 수와 값은 모든 쌍에서 r6 2차(f51bcbd)와 같다.
   - 결정 문제 호출 수는 candidate7(r5)과 같다(272.7, 423.3, 319.1/inst.).

## 1. 측정 조건

- **기계**: 서버 컨테이너(Intel Xeon @ 2.10 GHz, 2 vCPU), 커널 6.18. r5·r6과 같은 종류의 컨테이너다. 다만 2026-10-31에 새로 만들어진 컨테이너다.
- **소프트웨어**
  - Ubuntu 24.04, g++ 13.3, CMake 3.28
  - CGAL 5.6(GMPXX 백엔드), Boost 1.83(둘 다 GitHub 릴리스)
  - GMP 6.3.0 / MPFR 4.2.1(Ubuntu 패키지 `libgmp-dev`, `libgmpxx4ldbl` 2:6.3.0+dfsg-2ubuntu6.1, `libmpfr-dev` 4.2.1-1build1.1). 시스템의 `libgmp10`·`libmpfr6`와 같은 버전이다. 이 컨테이너에서는 패키지 서버에 접근할 수 없어서, 사용자가 받아 준 `.deb` 3개를 설치했다.
- **빌드**: 두 방식 모두 `r4_original_vs_candidate7/scripts/build.sh`로 같은 플래그(RelWithDebInfo)로 빌드했다. 환경은 [`raw/r7_log.txt`](raw/r7_log.txt)에 있다.
- **방식**: r5와 같다.
  - CPU 1에 고정하고 CPU 0은 비워 두었다.
  - 작업(Characters 100쌍 묶음, Sigspatial 쌍, 결정 문제 질의 파일)마다 두 방식을 별도 프로세스로 연달아 실행하고, 순서를 번갈아 바꿨다.
  - 저자 코드는 Sigspatial에서 5 GB 주소 공간 제한을 걸었다. 그래도 1,000쌍 모두 끝났다.

## 2. r5(초록)·r6과 비교

| | r5 (초록), 저자 코드 ÷ candidate7 | r6 1차, 저자 코드 ÷ candidate8(e264dd4) | **r7, 저자 코드 ÷ candidate8(f51bcbd)** |
|---|--:|--:|--:|
| Characters 총 시간 / 기하평균 | 3.75× / 3.72× | 4.60× / 5.16× | **4.55× / 5.09×** |
| Sigspatial 총 시간 / 기하평균 | 29.17× / 2.83× | 35.26× / 3.50× | **33.33× / 3.24×** |
| 결정 문제 4^ℓ (same / all / Sigspatial) | 2.01× / 2.43× / 1.29× | (저자 코드 재측정 안 함) | **2.01× / 2.39× / 1.25×** |
| 저자 코드 Characters ms/쌍 | 123.94 | 138.20 | 102.00 |

- **절대 시간은 측정마다 크게 다르다.**
  - 같은 저자 코드가 같은 호출 수로 r5 123.9, r6 138.2, r7 102.0 ms/쌍이 나왔다. r7의 기계가 r5보다 18 % 빨랐다.
  - 그래서 ms 값은 측정 하나 안에서만 비교한다. 논문의 표·그림·초록은 r7 하나에서 가져온다.
- **Characters의 비는 측정이 달라도 안정적이다**(4.60× → 4.55×). 두 방식 모두 r6보다 25 % 안팎 빨라졌다.
- **Sigspatial 기하평균은 3.50×에서 3.24×로 내려갔다.** 두 방식이 시간을 쓰는 단계가 다르기 때문이다.
  - candidate8의 Sigspatial 시간 중 36 %는 전처리(곡선 읽기와 준비)다. 저자 코드에서는 1 %다.
  - r6과 비교하면 r7에서 전처리는 0.84배로 덜 줄었고, 다른 단계는 0.69–0.80배로 더 줄었다.
  - 그래서 이 측정에서는 candidate8이 상대적으로 덜 빨라졌다.
  - 정리하면 비 자체도 기계 상태에 따라 몇 % 움직인다. 논문에는 r7 값을 쓰고, 측정 사이의 변동을 짧게 적는다.
- **결정 문제 4^ℓ는 r5의 candidate7과 같다.** 블랙박스 호출 수가 같고, 비도 3 % 안에서 같다.

## 3. 결정 문제 2^ℓ에 대해

- 2^ℓ 질의는 질의당 0.3–1 ms다. 세트 하나(1,000질의)가 1초도 걸리지 않는다.
- all-characters 2^ℓ는 0.98×로, r5 candidate7의 1.08×보다 낮다. 하지만 블랙박스 호출 수는 r5 candidate7과 같다(58.1/inst.).
- 차이는 두 방식이 같은 코드를 쓰는 단계에서 생겼다(Lipschitz 호출 0.94×, 배열 추정 0.91×). 실행시간 측정 잡음 수준이다.
- 그래서 2^ℓ는 계속 부록에 둔다.

## 4. 논문에 쓸 수치 (r7 기준)

- **초록 결과 문장**: "저자 공개 코드와 같은 기계에서 번갈아 쟀다. 판정자 호출은 4.2–4.6배 줄었다. 값 계산은 Characters 21,000쌍에서 4.55배(기하평균 5.09배), Sigspatial 1,000쌍에서 기하평균 3.24배 빨라졌다. 결정 문제(1 ± 4^ℓ)는 1.25–2.39배 빨라졌다. 모든 답이 같았고, 값의 차이는 1.3×10⁻⁸ 이하였다."
- **Fig 2(구성별 실행시간 막대, ms/쌍)**
  - "나머지" = 총 시간 − 배열 알고리즘 단계(전처리, Lipschitz 호출, 배열 추정, 단계에 안 잡힌 시간).

  | 벤치마크 | 방식 | 배열 구성·열거 | 배열 단계 판정자 호출 | 배열 단계 기타 | 나머지 | 총 |
  |---|---|--:|--:|--:|--:|--:|
  | Characters | 저자 코드 | 58.11 | 15.22 | 5.34 | 23.32 | 102.00 |
  | Characters | candidate8 | 2.01 | 1.52 | 0.03 | 18.87 | 22.43 |
  | Sigspatial | 저자 코드 | 1,688.75 | 17.89 | 6.03 | 60.70 | 1,773.38 |
  | Sigspatial | candidate8 | 2.33 | 1.63 | 0.06 | 49.19 | 53.21 |

- **대조 검증 문장을 바꿔야 한다.**
  - 개요의 Fig 2 논증은 "나머지가 두 방식에서 같다 → 빌드 차이가 아니다"였다. candidate7에서는 맞았다(27.75 / 27.20 ms).
  - candidate8은 상자를 일찍 끝내서 Lipschitz 호출과 배열 추정도 줄인다. 그래서 나머지가 23.3 → 18.9 ms로 다르다.
  - 대신 두 방식이 같은 일을 하는 **전처리**를 근거로 쓴다: Characters 3.35 / 3.31 ms, Sigspatial 18.74 / 19.18 ms(1.01×, 0.98×).
  - 그리고 저자 코드의 호출 수가 [BKN20] Table 4와 같다는 점을 함께 든다.

## 5. 파일

| 파일 | 내용 |
|---|---|
| `RESULTS.md` | 값 계산: Table 4 형식 표(Characters, Sigspatial), 인스턴스별 가속, 정확성, r5·r6과의 대조(호출 수·값·시간 비), LaTeX |
| `RESULTS_decider.md` | 결정 문제: Table 2 형식 표(세 벤치마크 × 4^ℓ·2^ℓ), 세트별 결과, LaTeX |
| `fig_scatter.{pdf,png}`, `fig_scatter_characters.*`, `fig_scatter_sigspatial.*` | [BKN20] Figure 6 형식 산점도(x = 저자 코드, y = candidate8) |
| `raw/characters_lmf_r7.tar.gz`, `raw/sigspatial_lmf_r7.tar.gz` | 값 계산 원시 CSV(묶음·쌍 × 두 방식), 로그, 종료 코드 |
| `raw/decider_4l_r7.tar.gz`, `raw/decider_2l_r7.tar.gz` | 결정 문제 원시 CSV(질의 파일 69개 × 두 방식) |
| `raw/r7_log.txt` | 측정 시각, CPU, 커널, 패키지 버전, 저장소 커밋 |
| `scripts/run_r7.sh` | 측정 전체(`authors_check/scripts`의 `run_lmf3.sh`·`run_sig3.sh`·`run_dec3.sh`, r5의 `run_dec2.sh`를 `ARMS="gitlab candidate8"`로 실행) |
| `scripts/analyze_r7.py`, `scripts/plot_scatter.py` | `raw/` → `RESULTS*.md`, 그림 |

## 6. 재현

```
ARMS="authors_gitlab candidate8" DEPS=/opt/deps bash paper_bench/r4_original_vs_candidate7/scripts/build.sh
python3 paper_data/convert_sigspatial.py                               # paper_data/sigspatial/data (git-ignore 대상)
bash paper_bench/r7_authors_vs_candidate8/scripts/run_r7.sh             # ~/r7, 약 2시간
# ~/r7/{chars,sig,dec4,dec2}를 raw/의 tar.gz로 묶은 뒤:
python3 paper_bench/r7_authors_vs_candidate8/scripts/analyze_r7.py
python3 paper_bench/r7_authors_vs_candidate8/scripts/plot_scatter.py
```

`DEPS`를 주지 않으면 배포판 패키지(`libcgal-dev` 등)를 쓴다. `/opt/deps`는 CGAL 5.6과 Boost 1.83 릴리스를 풀어 둔 곳이다(`build.sh` 주석 참고).
