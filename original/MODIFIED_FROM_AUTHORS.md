# 이 폴더는 [BKN20] 저자 원본 코드의 수정판이다

`original/`은 저자 원본(GitLab `anusser/frechet_distance_under_translation`, master `3bbb305`)과 **7개 파일이 다르다.**
그중 2개가 배열 알고리즘의 기저 사례를 바꾼다. 따라서 이 폴더는 "수정 없는 저자 코드"가 아니다.

**논문 기준선은 [`../authors_gitlab/`](../authors_gitlab/README.authors_gitlab.md)을 쓸 것.**

이 폴더의 소스는 그대로 둔다. 많은 스크립트가 `original/` 경로를 참조하고, 기존 실험 기록이 이 판으로 잰 것이기 때문이다.
근거와 측정은 [`../paper_bench/authors_check/REPORT.md`](../paper_bench/authors_check/REPORT.md)에 있다.

## 다른 7개 파일 (REPORT.md 2절)

전체 차이는 [`../paper_bench/authors_check/original_vs_gitlab.diff`](../paper_bench/authors_check/original_vs_gitlab.diff)에 있다.

| 파일 | 바뀐 내용 | 분류 | 결과에 미치는 영향 |
|---|---|---|---|
| `CMakeLists.txt` | `cmake_minimum_required` 2.8.8 → 3.5, `-std=c++11` → `c++14` | 빌드 이식 | 없음 |
| `lib/cgal_disk_arrangements/CMakeLists.txt` | CGAL 4식(`CGAL_LIBRARIES`) → CGAL 5 타깃 | 빌드 이식 | 없음 |
| `lib/measurement_tool/measurement_tool.h` | `high_resolution_clock` → `steady_clock` | 측정 | 시간 측정만 |
| `lib/cgal_disk_arrangements/disc_arrangement_traversal.cpp` | `USE_EPICK` 스위치 추가(X2 실험). 정의하지 않으면 Epeck 그대로 | 실험 스위치 | 없음 |
| `src/calc_frechet_distance_under_translation.cpp` | `n6` 명령에 출력 한 줄 | CLI | 없음 (`paper_bench`는 안 씀) |
| **`src/frechet_under_translation.cpp`** | 기저 사례 4곳(결정 문제 2, LMF 2)에서 탐색 상자 B 인자를 뺌 | **알고리즘** | 아래 |
| **`src/fut_n6_algorithm.cpp`** | 상자 없는 `calcDistance`·`lessThan`을 새로 짬 | **알고리즘** | 아래 |

그 밖에 `divide_and_conquer.svg`(실행 산출물)와 `.bak` 두 파일이 이 폴더에만 있다.

## 기저 사례 차이 (REPORT.md 3절)

| | 저자 코드 (상자 B) | `original/` (상자 없음) |
|---|---|---|
| 배열에 넣는 것 | 탐침 반지름에서 B와 만나는 원(CGAL로 다시 판정) + **B의 네 변** | `cut_centers`의 원 전부, 변 없음 |
| 시험하는 점 | 원–원 교점(B 밖 포함), 원의 좌우 극점, **원–변 교점, B의 꼭짓점** | 원–원 교점, 원의 좌우 극점 |
| 배열이 비면 | B 중심을 시험 | NO |
| LMF 기저 사례 이분 탐색 구간 | [max(0, f(τ_B) − d_B/2), f(τ_B)], τ_B = B의 중심 | **[0, f(τ_start)]**, τ_start = 첫 점끼리 맞춘 평행이동 |
| 결과 대입 | 조건 없이 `max =` | 조건 없이 `max =` |

## 영향

- **큰 값 오답.** [0, f(τ_start)] 구간의 탐침이 [ℓ_B, δ̃]용으로 고른 원만 쓰므로, 기저 사례가 전역 상한 δ̃를 **올려 버린다.** r4 검증 revisit 가족의 큰 값 오답 7개가 이것이다. 계측에서 `original`은 같은 쌍에서 δ̃를 42–2,491번 올리고 저자 코드는 0번이다.
- **Sigspatial 2쌍 12 GB 실패.** 쌍 125(`file-003586`/`file-002157`)와 432(`file-002502`/`file-016674`)는 `original`에서 12 GB를 넘겨 실패한다. 저자 코드는 5 GB 안에서 497.5 s, 338.0 s에 정답으로 끝낸다. 나머지 998쌍도 `original`이 총 1.74배 느리다.
- **결정 문제 호출 수 14–15 % 감소.** 답은 모든 질의에서 같지만, 상자 변이 빠져 시험점이 줄어 4^ℓ 세트의 블랙박스 호출이 저자 코드보다 14–15 % 적다(2^ℓ 세트 1–5 %). 논문 Table 2와의 −13~−16 % 차이가 이것이다.
- Characters LMF는 시간·호출 수 차이가 1–2 %다. 논문 Table 4의 260,128,449회와 `original`의 257,162,361회(−1.1 %) 차이도 이 수정 때문이다.

## `.bak` 두 파일은 GitLab 원본과 같다

| 파일 | GitLab 원본 |
|---|---|
| `src/fut_n6_algorithm.cpp.bak` | `src/fut_n6_algorithm.cpp`와 바이트 단위로 같음 |
| `lib/cgal_disk_arrangements/CMakeLists.txt.bak` | `lib/cgal_disk_arrangements/CMakeLists.txt`와 바이트 단위로 같음 |

`src/frechet_under_translation.cpp`의 원래 판은 남아 있지 않다. 원래 판은 `../authors_gitlab/src/frechet_under_translation.cpp`다.
