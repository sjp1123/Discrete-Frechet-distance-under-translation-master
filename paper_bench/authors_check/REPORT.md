# 저자 원본 코드(GitLab), 저장소 `original/`, 선행논문 — 셋의 비교 (2026-10-03)

**비교한 셋:**
- [BKN20] 논문: Bringmann, Künnemann, Nusser, *When Lipschitz Walks Your Dog*, ESA 2020, arXiv 2008.07510.
- 저자 원본 코드: GitLab `anusser/frechet_distance_under_translation`, master 커밋 `3bbb305`.
- 저장소의 `original/`.

이 문서는 결과만 보고한다. `original/`과 기존 문서는 고치지 않았다.

## 0. 요약

1. **저자 원본 = 논문을 낸 코드다.** GitLab 코드로 Characters 21,000쌍(논문 Table 4)을 돌리면 블랙박스 호출 수가 **260,128,449회(12,387.1/쌍)로 논문 Table 4와 한 자리까지 같다.** 결정 문제 4^ℓ 세트(Table 2)의 호출 수도 논문과 2 % 안이다(−2.1~+1.6 %).
2. **`original/`은 저자 원본이 아니다.** 7개 파일이 다르다.
   - 5개는 빌드·측정용이라 영향이 없다: CMake 2개, 측정 시계, EPICK 스위치, CLI 출력.
   - 2개(`frechet_under_translation.cpp`, `fut_n6_algorithm.cpp`)는 **배열 알고리즘의 기저 사례를 바꾼다.** 저자 코드는 탐색 상자 B를 넘겨 B의 네 변을 배열에 넣고, LMF 기저 사례를 [f(τ_B) − d_B/2, f(τ_B)]에서 이분 탐색한다. `original/`은 상자를 빼고 [0, f(τ_start)]에서 이분 탐색한다.
   - 이 수정은 저장소 첫 커밋 이전부터 있었다. 저장소 문서는 `original/`을 "수정 없음"이라고 적고 있다. 최상위 `src/`도 원본이 아니다.
3. **수정의 영향 (같은 기계에서 세 방식을 번갈아 측정).**
   - 결정 문제: 답은 모든 질의에서 같다. 호출 수는 `original`이 저자 코드보다 4^ℓ 세트에서 14–15 %, 2^ℓ 세트에서 1–5 % 적다.
   - Characters LMF: 시간·호출 수 차이가 1–2 %다.
   - Sigspatial LMF: 차이가 크다. `original`은 2쌍에서 12 GB를 넘겨 실패하고 나머지 998쌍도 총 1.74배 느리다. 저자 코드는 1,000쌍을 모두 5 GB 안에서 끝낸다.
   - 정확성: `original`에만 있는 오답이 있다. 기저 사례가 전역 상한 δ̃를 **올려 버려서** 값이 크게 나온다(계측으로 확인).
4. **제안 방법(candidate7)의 가속을 저자 코드 기준으로 다시 쟀다.**

   | 벤치마크 | candidate7 가속 |
   |---|---|
   | Characters LMF | 총 3.66×, 인스턴스별 기하평균 **3.67×** [3.64, 3.70] |
   | Sigspatial LMF (1,000쌍) | 총 30.1×, 기하평균 **2.82×** |
   | 결정 문제 4^ℓ | same-characters 1.95×, all-characters 2.27×, Sigspatial 1.20× (총 시간 비) |

   r4(`original` 기준)와 비교하면 다음과 같다.
   - 인스턴스별 기하평균은 같거나 커진다: Characters 3.51× → 3.67×, Sigspatial 2.45× → 2.82×.
   - 결정 문제 총 시간 비도 조금 커진다: r4 1.84×/2.18×/1.19× → 1.95×/2.27×/1.20×.
   - 총 시간 비는 Characters가 3.70× → 3.66×로 비슷하다. Sigspatial은 998쌍 기준 32.8× → 18.4×로 작아진다. `original`의 무거운 꼬리가 수정 때문에 느렸기 때문이고, 1,000쌍 전체로는 30.1×다.

   **제안 방법이 빠르다는 결론은 바뀌지 않는다. 다만 논문의 기준선은 GitLab 코드로 바꾸고, Sigspatial 총 시간 비는 다시 써야 한다.**
5. **저자 원본도 논문 서술과 세부가 다르다.** 구조는 논문 그대로다. 다른 점은 다음과 같다.
   - 기저 사례 이분 탐색 구간이 논문보다 넓다.
   - 배열에 상자 변을 넣는다.
   - 꼭짓점을 9e-9 여유를 두고 시험한다.
   - 논문에 없는 가지치기가 몇 개 있다.
   - 결함이 있다: 무한 루프 C, 원 잘림 B, assert F·H, 이분 탐색 기준선의 kd-tree 하한.

   자세한 내용은 5절이다.

## 1. 방법

- **저자 원본**: 사용자가 GitLab에서 받은 `frechet_distance_under_translation-master.zip`. zip 주석의 커밋은 `3bbb30502e201564bf9859806cda94198f8ac0d4`이고, 1,570개 파일이 모두 2020-06-27 날짜다. 해시는 `gitlab_3bbb305_sha256.txt`.
- **빌드 `gitlab`**:
  - 알고리즘 소스는 전부 GitLab 파일이다.
  - CMake 2개(CGAL 5 이식)와 측정 시계(`steady_clock`)만 `original/`에서 가져왔다. 이 셋은 빌드·측정에만 관여한다.
  - 같은 `paper_bench` 하네스, `RelWithDebInfo`, CGAL 5.6(GMPXX)으로 빌드했다(`scripts/build_gitlab_arm.sh`).
- **측정**:
  - r4와 같은 서버 컨테이너, 2 vCPU다.
  - 세 방식(`gitlab`, `original`, `candidate7`)을 묶음·쌍·질의 파일마다 같은 코어에서 연달아 실행했다. 순서는 6가지 순열로 돌렸다.
  - Characters LMF와 결정 문제는 CPU 1에서 쟀다.
  - Sigspatial LMF와 정확성 검사는 **동시에 CPU 0에서** 돌렸다. 그래서 r4(다른 코어를 비움)보다 시간 잡음이 크다. 다만 세 방식이 같은 조건을 번갈아 겪으므로 방식 간 비는 공정하다.
- **정확성**: r4 검증의 독립 정확 오라클 인스턴스(`verification/`)를 그대로 썼다.
- **기저 사례 계측**: 기저 사례가 δ̃를 올릴 때마다 출력하는 한 줄을 스크래치 사본(`original`, `gitlab` 각각)에 넣었다.

## 2. `original/` 대 저자 원본 — 파일별

전체 차이는 `original_vs_gitlab.diff`(234줄)에 있다.

| 파일 | 바뀐 내용 | 분류 | 결과에 미치는 영향 |
|---|---|---|---|
| `CMakeLists.txt` | `cmake_minimum_required` 2.8.8 → 3.5, `-std=c++11` → `c++14` | 빌드 이식 | 없음 |
| `lib/cgal_disk_arrangements/CMakeLists.txt` | CGAL 4식(`CGAL_LIBRARIES`) → CGAL 5 타깃. 원래 파일이 `CMakeLists.txt.bak`로 남아 있음 | 빌드 이식 | 없음 |
| `lib/measurement_tool/measurement_tool.h` | `high_resolution_clock` → `steady_clock` | 측정 | 시간 측정만 (experiment_log 6.12) |
| `lib/cgal_disk_arrangements/disc_arrangement_traversal.cpp` | `USE_EPICK` 스위치 추가(X2 실험). 정의하지 않으면 Epeck 그대로 | 실험 스위치 | 없음 |
| `src/calc_frechet_distance_under_translation.cpp` | `n6` 명령에 출력 한 줄 | CLI | 없음 (`paper_bench`는 안 씀) |
| **`src/frechet_under_translation.cpp`** | 기저 사례 4곳(결정 문제 2, LMF 2)에서 탐색 상자 B 인자를 뺌 | **알고리즘** | 3절 |
| **`src/fut_n6_algorithm.cpp`** | 상자 없는 `calcDistance`·`lessThan`을 새로 짬. 원래 판이 `fut_n6_algorithm.cpp.bak`로 남아 있음(GitLab과 같음) | **알고리즘** | 3절 |

**파일 구성의 차이.**
- `original/`에만 있는 것: `divide_and_conquer.svg`(실행 산출물), 두 `.bak` 파일.
- GitLab에만 있는 것: `tools/CMakeCache.txt`·`tools/CMakeFiles`(저자 빌드 찌꺼기), `test_data/benchmark`.

**저장소 최상위 `src/`·`lib/`도 원본이 아니다.** GitLab과 비교하면 `src/frechet_under_translation.cpp` 하나가 다르다. `original/`과 같은 호출부 수정이다(`reporoot_vs_gitlab.diff`). `fut_n6_algorithm.cpp`는 GitLab 그대로라서, 최상위 사본은 상자 없는 호출이 저자의 "전역 상자" 판을 부르는 세 번째 변형이 된다.

**언제 바뀌었나.** 저장소의 첫 커밋(`3516e55`, 2026-09-07 "Snapshot")에 이미 이 상태로 들어 있다. 그 뒤 `original/`에서 바뀐 것은 측정 시계 하나다. 그런데 다음 문서들은 `original/`이 수정되지 않았다고 적고 있어 실제 코드와 맞지 않는다.
- `experiment_log.md` 1.3절: "박스 사용"
- `_x1_results/ENVIRONMENT.md`: "알고리즘 수정 없음"
- `paper_bench/results/REPRO_check.md`: "The algorithm is unmodified"

누가 왜 바꿨는지는 저장소에서 알 수 없다.

## 3. 기저 사례에서 실제로 달라지는 것

저자 코드(GitLab `frechet_under_translation.cpp`):

```cpp
// 결정 문제 기저 사례 (482–486, 513–517)
auto bounding_box = n6_alg.toBoundingBox(search_box);
bool less = n6_alg.lessThan(distance, curve1, curve2, bounding_box);
// LMF 기저 사례 (191–197, 256–262)
auto bounding_box = n6_alg.toBoundingBox(search_box);
if (n6_alg.lessThan(max, curve1, curve2, bounding_box)) {
    max = n6_alg.calcDistance(curve1, curve2, bounding_box);
```

`original/`은 같은 자리에서 `bounding_box`를 뺐다. 그래서 `lessThan(distance, curve1, curve2)`, `calcDistance(curve1, curve2)`가 불린다.

| | 저자 코드 (상자 B) | `original/` (상자 없음) |
|---|---|---|
| 배열에 넣는 것 | 탐침 반지름에서 B와 만나는 원(CGAL로 다시 판정) + **B의 네 변** | `cut_centers`의 원 전부, 변 없음 |
| 시험하는 점 | 원–원 교점(B 밖 포함), 원의 좌우 극점, **원–변 교점, B의 꼭짓점** | 원–원 교점, 원의 좌우 극점 |
| 배열이 비면 | B 중심을 시험 | NO |
| LMF 기저 사례 이분 탐색 구간 | [max(0, f(τ_B) − d_B/2), f(τ_B)], τ_B = B의 중심 | **[0, f(τ_start)]**, τ_start = 첫 점끼리 맞춘 평행이동 |
| 결과 대입 | 조건 없이 `max =` | 조건 없이 `max =` |

f(τ_start)는 초기 상한 수준의 고정값이다. 탐색이 진행되어 δ̃가 줄어들수록 둘의 간격은 벌어진다. 그래서 `original/`은 [ℓ_B, δ̃]용으로 고른 원만 가지고 δ̃보다 한참 위의 반지름을 탐침한다. 그 탐침이 NO를 내면 이분 탐색이 δ̃ 위로 수렴하고, 그 값이 조건 없이 대입되어 **전역 상한 δ̃가 올라간다.**

**계측 확인.** `original`이 큰 값을 낸 revisit 5쌍을 두 사본으로 돌렸다.

| 쌍 | 정답 | `original` 값 | `original`의 δ̃ 상승 횟수 (예) | 저자 코드 값 | 저자 코드의 δ̃ 상승 |
|---|--:|--:|---|--:|--:|
| 00498 | 2.1828474 | 2.3277279 | 45회 (2.3274 → 3.2732) | 2.1828474 | 0 |
| 00602 | 2.6811102 | 2.7769087 | 42회 (2.7768 → 3.3439) | 2.6811102 | 0 |
| 00405 | 3.2163193 | 3.2179664 | 937회 (3.2180 → 6.1739) | 3.2163193 | 0 |
| 00489 | 5.3326579 | 5.3400642 | 2,491회 (5.3401 → 8.2690) | 5.3326579 | 0 |
| 00567 | 4.0380609 | 4.0393631 | 1,910회 (4.0386 → 6.5115) | 4.0380609 | 0 |

## 4. 측정

### 4.1 Characters 값 계산 (LMF) — 논문 Table 4의 21,000쌍

괄호는 쌍당 평균이다. 비는 왼쪽 ÷ 오른쪽이다. 세 방식은 묶음(100쌍)마다 CPU 1에서 번갈아 실행했다.

| | 저자 코드 (GitLab) | `original` | candidate7 | GitLab ÷ c7 | `original` ÷ c7 | GitLab ÷ `original` |
|---|--:|--:|--:|--:|--:|--:|
| **Time** | 2,342,860 ms (111.6) | 2,377,271 ms (113.2) | 639,974 ms (30.5) | **3.66** | 3.71 | 0.986 |
| **Black-box calls** | **260,128,449 (12,387.1)** | 257,162,361 (12,245.8) | 68,690,225 (3,271.0) | 3.79 | 3.74 | 1.012 |
| – Preprocessing | 77,104 ms | 78,087 ms | 78,239 ms | 0.99 | 1.00 | 0.987 |
| – Black-box calls (Lipschitz) | 255,487 ms | 255,745 ms | 266,156 ms | 0.96 | 0.96 | 0.999 |
| – Arrangement estimation | 163,695 ms | 166,406 ms | 147,152 ms | 1.11 | 1.13 | 0.984 |
| – Arrangement algorithm | 1,810,314 ms | 1,840,507 ms | 112,082 ms | 16.15 | 16.42 | 0.984 |
| &nbsp;&nbsp;∗ Construction | 1,338,859 ms | 1,398,637 ms | 67,826 ms | 19.74 | 20.62 | 0.957 |
| &nbsp;&nbsp;∗ Black-box calls | 355,015 ms | 318,370 ms | 43,319 ms | 8.20 | 7.35 | 1.115 |

- **논문 Table 4는 260,128,449회(12,387.1/쌍)이고, 저자 코드가 이 값을 정확히 다시 냈다.**
  - 논문의 시간 140.0 ms/쌍과 여기 111.6 ms/쌍의 차이는 기계 차이다.
  - experiment_log 6.8은 `original`이 1.1 % 적은 이유를 "부동소수점 경로 차이"로 설명했다. 실제 원인은 3절의 수정이다.
- 인스턴스별 가속 (부트스트랩 95 % CI):

  | 비교 | 기하평균 | 중앙값 | candidate7이 빠른 쌍 |
  |---|---|--:|--:|
  | GitLab ÷ candidate7 | **3.67×** [3.64, 3.70] | 3.83× | 20,149 / 21,000 |
  | `original` ÷ candidate7 | 3.57× [3.54, 3.60] | 3.91× | — |
  | GitLab ÷ `original` | 1.027× | — | — |

- 값은 세 방식 모두 1.35e-8 안에서 같다.
- 저자가 함께 배포한 쌍별 LMF 시간(`experiments/characters_valcomp_full_scatter_lmf.dat`)과 이번 저자 코드 시간의 순위 상관은 0.866이다.
  - 그 파일은 쌍당 102.8 ms, 8,358회짜리 실행이다(`characters_valcomp_full_total_table.tex`).
  - 즉 논문 Table 4(140.0 ms, 12,387.1회)와는 다른 실행이다.

### 4.2 Sigspatial 값 계산 (LMF) — 저자의 결정 문제 1,000쌍, 전체 20,199곡선

CPU 0에서 쌍마다 세 방식을 번갈아 실행했다(같은 시간대에 CPU 1도 측정 중이었다). 저자 코드는 5 GB 주소 공간 제한을 걸고 돌렸다.

| | 저자 코드 (GitLab) | `original` | candidate7 |
|---|--:|--:|--:|
| 쌍 125 (`file-003586`/`file-002157`) | 497.5 s, 5110.942581 | **12 GB 초과로 실패** (experiment_log 6.9) | 0.54 s, 5110.942581 |
| 쌍 432 (`file-002502`/`file-016674`) | 338.0 s, 3886.460162 | **12 GB 초과로 실패** | 1.91 s, 3886.460162 |
| 나머지 998쌍 Time | 1,203,523 ms (1,205.9) | 2,096,778 ms (2,101.0) | 65,282 ms (65.4) |
| 나머지 998쌍 Black-box calls | 13,072,125 (13,098.3) | 12,554,186 (12,579.3) | 3,616,226 (3,623.5) |
| – Arrangement algorithm | 1,142,826 ms | 2,028,420 ms | 5,879 ms |
| &nbsp;&nbsp;∗ Construction | 1,118,541 ms | 2,006,806 ms | 3,454 ms |

- `original`이 메모리를 넘긴 2쌍은 저자 코드에서는 끝난다. 그 메모리 실패는 **수정 때문에 생긴 것**이다.
- 998쌍에서 `original`의 총 시간은 저자 코드의 1.74배다(배열 구성 1.79배). 인스턴스별로는 저자 코드가 1.13배 느리다(기하평균, 중앙값 1.04). 즉 수정이 대부분의 쌍은 조금 빠르게, 무거운 꼬리는 훨씬 느리게 만들었다.
- candidate7 가속 (저자 코드 기준):

  | 범위 | 총 시간 비 | 인스턴스별 기하평균 | 중앙값 | 호출 수 비 |
  |---|---|---|--:|--:|
  | 1,000쌍 전체 | **30.1×** | **2.82×** | 2.58× | 3.55× |
  | 998쌍 | 18.4× | 2.79× [2.66, 2.92] | — | — |

  - 998쌍 기준 candidate7이 빠른 쌍은 928쌍이다.
  - 저자 코드 총 시간의 24.7 %가 가장 무거운 1쌍, 83.1 %가 4쌍에서 나온다. 그래서 총 시간 비보다 인스턴스별 통계를 앞세워야 한다.
  - 같은 실행에서 `original` 기준은 998쌍에서 32.1×, 기하평균 2.47× [2.34, 2.60]이다.
- 값은 세 방식 모두 1.33e-8 안에서 같다.

### 4.3 결정 문제 — 논문 Table 2 형식, 계수 1 ± 4^ℓ

질의 파일은 r4와 같다(`queries/*_paperq4_*`). 저자의 같은 1,000쌍에 (δ* ∓ 10⁻⁷)(1 ∓ 4^ℓ)를 적용했고, 벤치마크마다 23세트 × 1,000질의다. 세 방식을 질의 파일마다 CPU 1에서 번갈아 실행했다. 값은 쌍당 평균이고, 비는 왼쪽 ÷ 오른쪽이다.

| 벤치마크 | 논문 Table 2 호출/질의 | 저자 코드 호출/질의 | `original` 호출/질의 | candidate7 호출/질의 | 시간 ms/질의 (GitLab / `original` / c7) | c7 가속 (GitLab 기준): 총 / 인스턴스별 기하평균 | c7 가속 (`original` 기준): 총 / 기하평균 |
|---|--:|--:|--:|--:|---|---|---|
| same-characters | 1,159.2 | **1,165.1** (+0.5 %) | 997.0 (−14 %) | 272.7 | 15.52 / 14.69 / 7.95 | **1.95×** / 1.23× [1.23, 1.24] | 1.85× / 1.19× |
| all-characters | 1,860.1 | **1,889.3** (+1.6 %) | 1,616.3 (−13 %) | 423.3 | 21.83 / 20.41 / 9.61 | **2.27×** / 1.17× [1.16, 1.17] | 2.12× / 1.11× |
| Sigspatial | 1,366.1 | **1,338.1** (−2.1 %) | 1,146.4 (−16 %) | 319.1 | 42.34 / 41.09 / 35.42 | **1.20×** / 1.11× [1.10, 1.11] | 1.16× / 1.08× |

- **답은 세 방식이 모든 질의에서 같다**(69,000질의 × 3).
- 저자 코드의 호출 수는 논문 Table 2와 2 % 안이다(−2.1~+1.6 %). 논문의 4^ℓ 질의 파일은 배포되지 않았는데, 같은 쌍과 같은 공식이라 이 정도로 맞는 것으로 보인다. r4에서 보고한 `original`의 −13~−16 %는 수정 때문이었다.
- `original`은 저자 코드보다 호출이 14–15 % 적고 시간도 3–7 % 짧다. 수정이 배열에서 상자 변을 빼 시험점이 줄었기 때문이다. 그래서 **저자 코드 기준의 candidate7 가속이 r4보다 조금 크다.**

### 4.4 결정 문제 — 저자가 배포한 2^ℓ 출력과 대조

저자 2^ℓ 질의 파일 69개(69,000질의)를 저자 코드로 돌려, 저자가 배포한 `experiments/{same,all}-characters.txt`·`sigspatial.txt`와 비교했다.

| 벤치마크 | 저자 배포 출력 호출/질의 | 저자 코드 (이번) | `original` |
|---|--:|--:|--:|
| same-characters | 53.35 | 72.66 | 69.16 |
| all-characters | 57.89 | 62.16 | 61.40 |
| Sigspatial | 22.81 | 24.58 | 24.24 |

- 답은 저자 코드와 `original`이 모든 질의에서 같다.
- δ*에서 먼 세트는 세 값이 거의 같다. 차이는 δ*에 가까운 NO 세트(ℓ ≤ −8)에 몰린다. 예를 들어 same-characters ℓ = −10 minus는 배포 출력 274.8, 저자 코드 634.3, `original` 569.4다.
- 저자 코드조차 배포 출력과 다르다. 그런데 같은 코드가 논문 Table 4·Table 2를 거의 정확히 다시 낸다. 따라서 **배포된 `experiments/` 출력은 논문 실행과 다른 실행(다른 판이나 다른 설정)에서 나온 것으로 보는 게 맞다.** 4.1의 쌍별 시간 파일이 논문 Table 4와 다른 실행인 것과도 맞아떨어진다.
- r4와 REPRO_check가 "`original`이 저자 출력을 재현한다"고 쓴 근거(이 2^ℓ 대조)는 근거가 약했다.

### 4.5 정확성 — r4 검증 인스턴스로 저자 코드 검사

| 인스턴스 가족 | `original` | 저자 코드 (GitLab) | candidate7 |
|---|---|---|---|
| 값 계산: 일반 13가족 + far_t_1e7, far_t_sig, review (14,553쌍) | 오답 0 | 오답 0 | 오답 0 |
| 값 계산: cluster (1,200쌍) | 오답 0 | 오답 0 | 오답 0 |
| 값 계산: revisit (1,000쌍) | 오답 9 (**큰 값 7**, 0에 가까운 값 2), assert 2 | 오답 2 (0에 가까운 값 2), assert 2 | 0 |
| 값 계산: far_t_1e8 (곡선 1e8 떨어짐, 800쌍) | 큰 값 26 | 큰 값 19 | 0 |
| 결정 문제: revisit (13,880질의) | 무한 루프 48 (45 s 제한) | 무한 루프 32 (20 s 제한, gdb로 결함 C 확인) | 0 |
| 결정 문제: cluster (16,477질의) | 틀린 NO 7, 무한 루프 38 | 틀린 NO 2, 무한 루프 31 | 0 |

- revisit의 **큰 값 오답 7개는 수정 때문**이다(3절 계측). 최상위 사본 변형(전역 상자)에서는 같은 가족에서 큰 값이 3개다.
- 0에 가까운 값 2개, assert 2개(결함 F), 무한 루프(결함 C), 1e8 평행이동의 큰 값은 **저자 코드에도 있다.**
  - 1e8 평행이동의 큰 값은 저자 코드에서 δ̃ 상승 없이 생긴다. 계측한 4쌍 모두 상승 0회였다.
  - 원인은 따로 가르지 않았다. 꼭짓점 좌표를 double로 반올림한 오차가 고정 여유 9e-9보다 커지는 것(P3)이 유력하다.
- 저자 코드의 무한 루프는 gdb 스택으로 확인했다. 깊이 한도 분기에서 `computeCutCenters(search_box, false)`(471)를 반복한다.
- 실제 데이터(Characters·Sigspatial)에서는 세 방식의 값이 모두 1.4e-8 안에서 같다(4.1, 4.2).

## 5. 저자 원본(GitLab) 대 논문 서술 — 구성 요소별 대조

논문: arXiv 2008.07510 (ESA 2020) 의 Algorithm 1(결정 문제), Algorithm 2(LMF), 4–6절 본문. 코드 위치는 GitLab `src/frechet_under_translation.cpp`(FUT), `src/fut_n6_algorithm.cpp`(N6), `lib/cgal_disk_arrangements/disc_arrangement_traversal.cpp`(ARR) 기준이다. `original/`도 아래 판정이 같다. 다른 점은 3절의 기저 사례뿐이다(P1, P2가 `original/`에서는 3절 표처럼 바뀐다).

### 5.1 논문 서술대로인 부분

| 논문 | 저자 코드 |
|---|---|
| 결정 문제의 초기 상자: τ ∈ D_δ(π₁−σ₁) ∩ D_δ(πₙ−σₘ) | 두 원판의 외접 사각형 교집합(FUT 377–), 두 원판이 안 겹치면 바로 NO(414) |
| 상자 큐는 FIFO | 층별 큐(`LayerQueue`)로 너비 우선. FIFO와 같은 순서 |
| d_F(π, σ+τ_B) > δ + d_B/2 이면 버림 (1-Lipschitz) | `!lessThanFixedTranslation(distance + diag_dist)` (447), diag_dist = 대각선/2 |
| d_F(π, σ+τ_B) ≤ δ 이면 YES | 456 |
| u = 2(c+c²) ≤ γ_size 이거나 깊이가 γ_depth 이면 배열 알고리즘, u = 0 이면 버림, 아니면 긴 변을 반으로 | 원의 개수 c ≤ 12(= u ≤ 312), 깊이 40에서 기저 사례(465–, 511). c = 0 이면 버림. `pushChildren`은 긴 변을 반으로 자른다 |
| 기여하는 원: 경계가 B와 만나거나 B 안에 있는 원. B를 통째로 품는 원은 제외 | `intersectAnnulus`(네 꼭짓점이 모두 원판 안이면 제외, 떨어져 있으면 제외, 나머지는 포함하는 보수적 판정) |
| 배열 A_B의 **모든** 꼭짓점을 시험한다(B 밖 포함) | ARR이 원 전체를 넣으므로 B 밖 교점도 꼭짓점이다. N6 주석: "we don't check for intersection in the box here" |
| CGAL 정확 술어·정확 구성(Epeck) | ARR `using Kernel = CGAL::Epeck` |
| 블랙박스: 가장 빠른 고정 평행이동 결정기를 이산판으로 | `DiscreteFrechetLight`(필터 포함) |
| LMF 초기 구간 [max{δ_s, δ_e}/2, min{δ_s, δ_e}] | `getInitialEstimates`: 앞·뒤 정렬 평행이동에서 ε/2 정밀도로 재고 같은 공식에 ±ε/2 (304–374) |
| δ̃ ← δ_UB, 초기 상자는 δ_UB로 같은 방식 | 147–159 |
| 우선순위 큐, 키는 국소 하한 ℓ_B (작은 것 먼저) | `std::priority_queue<…, std::greater>` (158) |
| d_F(τ_B) ≤ δ̃ 이면 고정밀로 값을 재서 δ̃·ℓ_B 갱신 | 208–212: 정밀도 ε/10으로 값, ℓ_B = max(ℓ_B, δ̃ − d_B/2) |
| 거친 정밀도로 ℓ_B 갱신 | 222–231: 정밀도 (δ̃ − ℓ_B)/10 |
| 종료: δ̃ ≤ ℓ_B(1+ε). 구현은 가법 ε = 10⁻⁷ (본문에 명시) | `max − min_dist ≤ ε/2` (238) |
| 기저 사례 크기 추정은 [ℓ_B, δ̃]의 **고리**(annulus)가 B와 만나는지로 하고, kd-tree로 찾는다 | `computeCutCenters(box, ℓ_B, δ̃)` + `intersectAnnulus(…, min_radius, max_radius)` (771–, 817–), `KdTree2::search(min, max)` |
| 기저 사례 값은 "δ̃보다 작을 때만" 관심 (본문 문장 22) | 게이트 `if (n6_alg.lessThan(max, …, B))` 를 통과할 때만 이분 탐색 (194, 259) |
| 자식 상자에 부모의 ℓ_B를 물려줌 | `pushChildren(BBQueue&, …)`이 `min_dist`를 넘김 |

### 5.2 논문 서술과 다른 부분

| # | 논문 | 저자 코드 | 영향 |
|---|---|---|---|
| P1 | 기저 사례: δ ∈ [ℓ_B, δ̃]에서 이분 탐색으로 δ̃ 갱신 | `calcDistance(…, B)`: [max(0, f(τ_B) − d_B/2), f(τ_B)]에서 이분 탐색(N6 23–50), 결과를 조건 없이 `max =`에 대입 (196, 261). f(τ_B) ≥ δ̃이므로 구간이 논문보다 넓다 | 탐침이 늘어난다. δ̃보다 위의 탐침은 [ℓ_B, δ̃]용으로 고른 원만 쓰므로 이론상 δ̃를 올릴 수 있다. 다만 계측한 9쌍(revisit 5, far_t_1e8 4)에서 저자 코드는 한 번도 올리지 않았다. `original/`은 구간이 [0, f(τ_start)]라 실제로 올린다(3절) |
| P2 | 배열 A_B = 기여하는 원들의 배열 | ARR이 B의 **네 변**도 선분으로 넣고, 원을 탐침 반지름에서 B 기준으로 다시 거른다 | 시험점이 늘어난다(B 꼭짓점, 원–변 교점). 시험점이 많아질 뿐이라 YES 판정은 건전하다 |
| P3 | 배열 꼭짓점에서 d_F ≤ δ 를 시험 (정확한 결정기) | `lessThanFixedTranslation(δ + 9e-9)`. 꼭짓점 좌표를 double로 반올림한 오차를 덮는 여유. 논문에는 결정 문제의 이 여유가 언급되지 않는다 | δ ∈ [δ* − 9e-9, δ*)에서 YES가 나올 수 있다 |
| P4 | Algorithm 2 13–14행: d_F(τ_B) > δ̃ + d_B/2 이면 거친 값으로 ℓ_B 갱신(→ 15행에서 버림) | 그 경우 값을 재지 않고 바로 버린다(217). 대신 δ̃ < d_F(τ_B) ≤ δ̃ + d_B/2 인 경우에 거친 값으로 ℓ_B를 올린다(논문에는 없는 동작) | 효율만 다르다. 버리는 상자는 같다 |
| P5 | 8행: 상자를 꺼내자마자 δ̃ ≤ ℓ_B(1+ε) 검사 | 꺼낼 때는 ℓ_B > δ̃ 만 검사(169)하고, ε 검사는 중심 평가 뒤(238) | 수렴한 상자에서 블랙박스 호출이 한 번 더 든다 |
| P6 | 20행: 깊이 γ_depth 상자도 10–16행(중심 평가) 뒤 기저 사례 | 깊이 한도 상자는 중심 평가 없이 바로 기저 사례(179–200) | 효율만 다르다 |
| P7 | 논문에 없는 가지치기 | 초기 상자를 곡선 극값(extreme point)으로 더 좁힘(391–), 자식 상자가 D(τ_s) ∩ D(τ_e)와 안 만나면 버림(`intersectsStartEndDiscs`), 이분 탐색 기준선에서 상자 제한을 이어 씀 | 모두 타당한 가지치기다. 결과는 같고 빨라진다 |
| P8 | 깊이 γ_depth에서는 C_B 전체로 배열 알고리즘 | 결정 문제(kd-tree 없음)의 전수 선택이 13개에서 `return` (722, 결함 B) | 원이 빠져 **틀린 NO**가 날 수 있다 |
| P9 | u = 0 이면 B를 버림 | 깊이 한도 분기에서 `continue`만 하고 `step()`이 없다 (477, 결함 C) | 같은 상자를 영원히 다시 본다: **무한 루프**. 이번에 gdb로 확인 |
| P10 | 이분 탐색 기준선은 정확한 결정기를 쓴다 | 이분 탐색 경로의 kd-tree 검색 하한 `distance − d_B/2`가 음수일 때 제곱으로 부호가 사라져 상자 중심 근처의 원을 놓친다 (709) | 기준선이 **틀린 NO → 큰 값**을 낼 수 있다 (검증 보고서 #6). LMF에는 없다(`max(0, …)` 사용) |
| P11 | — | 값 평가가 이분 구간의 아래 끝(`min`)을 돌려준다 | δ̃가 실제 달성값보다 최대 5e-9 낮다. ε 안이다 |
| P12 | γ_size, γ_depth 값은 "벤치마크로 골랐다"고만 함 | 기본값 깊이 40, c ≤ 12. 13행에 "실험 후 기본값을 바꿀 것" TODO | 논문 실행 당시 값과 같은지 확인할 수 없다 |
| P13 | 결정 문제 벤치마크: (1 − 4^ℓ)δ_LB, (1 + 4^ℓ)δ_UB | 배포된 생성기·질의 파일은 (δ* ∓ precision)(1 ∓ **2^ℓ**) (`fut_create_benchmark_decider.cpp` 140, 148) | 논문 Table 2의 4^ℓ 질의 파일은 배포되지 않았다 |
| P14 | — | 큰 좌표·동률에서 이분 탐색 종료 실패(결함 D), assert(결함 F, H), 동률에서 고정 평행이동 결정기의 거짓 YES(값이 0 근처로 나오는 revisit 2쌍) | 실제 데이터에서는 나타나지 않는다 |

### 5.3 정리

- 저자 코드는 논문의 두 알고리즘을 그대로 구현했다. 논문과 다른 곳은 효율만 바꾸는 세부(P4–P7, P11), 수치 여유(P3), 실험 설정(P12, P13), 결함(P8–P10, P14)이다.
- **논문 수치(Table 2·4)는 이 코드에서 나온 것이 확실하다.** Table 4 호출 수가 한 자리까지 같다(4.1).
- 그 수치는 결함에 영향받지 않는다. 논문의 실제 데이터에서 오답·무한 루프가 없고, Binary Search 기준선의 P10은 LMF 표와 무관하다.
- 논문 서술과 가장 크게 다른 곳은 P1(기저 사례 탐색 구간)이다. 저자 코드에서는 이것이 오답으로 이어지지 않았다. `original/`의 [0, f(τ_start)]는 저자 코드와도 논문과도 다르고, 여기서 오답이 생긴다.

## 6. 앞선 결과·문서 가운데 고쳐야 할 것

| 어디 | 지금 적힌 것 | 실제 |
|---|---|---|
| 이 대화의 앞선 답변 | "LMF 기저 사례 [0, f(τ_start)] 탐색은 논문과 다른 저자 코드" | 저자 코드가 아니라 저장소 수정이다. 저자 코드는 [f(τ_B) − d_B/2, f(τ_B)] (P1) |
| r4 `README.md`·`RESULTS.md`·`RESULTS_decider.md` | 기준선 `original` = "[BKN20] 저자 코드" | 기준선은 수정판이다. 저자 코드 기준 수치는 4절 |
| r4 `RESULTS.md` Sigspatial | "기존 구현이 12 GB를 넘는 2쌍" | 수정판의 문제다. 저자 코드는 5 GB 안에서 끝낸다 |
| r4 `verification/REPORT.md` 3절 #2 | original의 큰 값 오답을 저자 코드 결함으로 분류 | revisit의 큰 값 7개는 수정 때문이다. 0 근처 값·assert·무한 루프·잘림·1e8 큰 값은 저자 코드에도 있다 |
| `experiment_log.md` 6.8 | 논문 Table 4와 1.1 % 차이는 부동소수점 경로 탓 | 수정 탓이다. 저자 코드는 260,128,449회로 같다 |
| `experiment_log.md` 1.3, `_x1_results/ENVIRONMENT.md`, `REPRO_check.md` | original = 수정 없는 저자 코드 | 2절의 두 파일이 수정되어 있다 |
| `REPRO_check.md` 2절 | 논문 Table 2와 호출 수 −13~−16 % (표본 차이 추정) | 수정 탓이다. 저자 코드는 −2.1~+1.6 % |
| 이 저장소의 모든 candidate | `original/`(수정판)에서 갈라짐 | candidate6·7은 자기 기저 사례에 상자를 따로 넘기므로(`setBox`) 수정의 영향이 그 경로에는 없다. 다른 상속 부분은 따로 검토하지 않았다 |

## 7. 권고

1. **논문의 기준선을 GitLab 코드(커밋 `3bbb305`)로 바꾼다.** 표·그림·초록 수치를 4절 값으로 쓴다.
   - "기존 구현은 [BKN20] 저자의 공개 코드(커밋 3bbb305)를 빌드 이식만 해서 그대로 썼고, Table 4의 블랙박스 호출 수 260,128,449회를 그대로 재현했다"라고 쓸 수 있다.
   - 기준선 재현을 보이는 데 이보다 강한 근거는 없다.
2. 그림은 `fig_scatter_gitlab_vs_c7*.{pdf,png}`를 쓴다. 두 패널이 같은 기계에서 쟀고, Sigspatial은 1,000쌍 전부다.
3. `original/`을 계속 둘 거라면 이름을 `original_modified`처럼 바꾸거나, 두 파일을 GitLab판으로 되돌리고 r4를 다시 돌린다. 이 보고서는 코드를 바꾸지 않았다.
4. 두 LMF 벤치마크와 결정 문제 앞부분(same-characters 일부)은 다른 코어도 측정 중인 상태에서 쟀다. 논문 본표로 쓰려면 다른 코어를 비우고 한 번 더 재는 게 좋다. 방식 간 비는 번갈아 쟀으므로 지금도 공정하다. 호출 수와 값은 기계·부하와 무관하다.
5. 논문 Figure 6 자체(LMF vs Binary Search)는 저자가 배포한 쌍별 시간으로 그린 `fig6_authors_shipped.png`가 있다. 다만 그 자료는 논문 Table 4와 다른 실행이다(4.1). 논문과 같은 실행을 원하면 저자 코드로 Binary Search 21,000쌍을 돌려야 한다(약 3시간). Binary Search 경로에는 P10 결함이 있다.

## 8. 재현

```
ZIP=<frechet_distance_under_translation-master.zip> DEPS=/opt/deps bash paper_bench/authors_check/scripts/build_gitlab_arm.sh   # ~/b_pb_gitlab
bash paper_bench/authors_check/scripts/run_lmf3.sh        # Characters 21,000쌍, 세 방식, CPU 1 (약 1시간 40분)
CPU=0 bash paper_bench/authors_check/scripts/run_sig3.sh  # Sigspatial 1,000쌍, 세 방식 (약 1시간 40분)
CPU=1 bash paper_bench/authors_check/scripts/run_dec3.sh  # 결정 문제 4^ℓ 69 파일, 세 방식 (약 1시간 50분)
bash paper_bench/authors_check/scripts/run_dec_gitlab.sh paperq   # 결정 문제 2^ℓ, 저자 코드 (약 2분)
python3 paper_bench/authors_check/scripts/cmp_dec.py paperq        # 4.4 표
# 정확성: verification/의 인스턴스로  python3 scripts/runner.py gitlab lmf|decider <list> <dir> <out.csv> 600 <cpu>,
#         판정은 scripts/check_lmf.py, scripts/check_dec.py
python3 paper_bench/authors_check/scripts/plot_scatter.py ~/arms/gitlab
```

| 파일 | 내용 |
|---|---|
| `original_vs_gitlab.diff`, `reporoot_vs_gitlab.diff` | 2절의 차이 전체 |
| `gitlab_3bbb305_sha256.txt` | 저자 원본 1,570개 파일 해시 |
| `raw/characters_lmf_3way.tar.gz` | 4.1 원시 CSV (`gitlab/`, `original/`, `candidate7/`, 묶음 c000–c209) |
| `raw/sigspatial_lmf_3way.tar.gz` | 4.2 원시 CSV (쌍 s0001–s1000) |
| `raw/decider_4l_3way.tar.gz` | 4.3 원시 CSV (질의 파일 69개 × 세 방식) |
| `raw/decider_2l_gitlab.tar.gz` | 4.4 저자 코드 원시 CSV |
| `raw/verification_families_gitlab.tar.gz` | 4.5 저자 코드 결과(`gres/`, `gres2/`) |
| `raw/repo_root_variant_revisit.tar.gz` | 최상위 사본 변형의 revisit 결과 |
| `fig_scatter_gitlab_vs_c7*.{pdf,png}` | 저자 코드(x) 대 candidate7(y), Figure 6 형식 |
| `fig6_authors_shipped.{pdf,png}` | 저자가 배포한 쌍별 시간으로 그린 LMF vs Binary Search |
