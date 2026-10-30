# Candidate 6 — 상자 단위로 건전하게 만든 candidate5

> 정정 (2026-10-21): `original/`은 저자 코드를 수정한 사본이다. 그 기저 사례의 [0, f(τ_start)] 탐색은 이 저장소에서 바꾼 것이다(저자 코드는 상자 안에서 [max(0, f(τ_B) − d_B/2), f(τ_B)]를 이분 탐색한다). [`../paper_bench/authors_check/REPORT.md`](../paper_bench/authors_check/REPORT.md) 참고.

Candidate 6은 **기저 사례를 고친 candidate5**다. candidate5의 극대 영역 생성기는 탐색 상자 전체를 포함하는 원판을 무시한다. 그러면 그 증인점(MEC 중심)이 그런 원판 밖에 놓일 수 있다. 그 결과 기저 사례가 YES 평행이동을 포함하는 상자에 NO라고 답할 수 있다. `original/`(저자 코드를 수정한 사본)에는 LMF 기저 사례에 이와 별개인 두 번째 결함이 있다. 두 결함 모두 구성한 입력에서 오답으로 드러난다(3절). 논문 데이터에서는 둘 다 드러나지 않았다.

기저 사례 밖은 아무것도 바뀌지 않는다. 분할 정복 구동부, 원판 선택, kd-tree, sub-Fréchet 판정자, Epeck 대체 경로(fallback), `CUT_LIMIT` / `DEPTH_LIMIT` 설정 변수가 모두 같다. `MAXREGION_FIX=none N6_RANGE=0`을 주면 같은 바이너리 안에서 candidate5의 코드 경로로 돌아간다.

---

## 1. candidate5 대비 바뀐 것

| 파일 | 변경 |
|---|---|
| `lib/cgal_disk_arrangements/maximal_regions.{h,cpp}` | `enumerate_box()`: 교집합이 상자와 만나는 극대 집합, 필터링된 정확한 Q1/Q2, 인증된 증인점. 기각된 `aware`/`lazy` 방식용 `enumerate_block()`. `enumerate()`는 그대로 |
| `lib/cgal_disk_arrangements/disc_arrangement_traversal.{h,cpp}` | 상자 / 블록 생성자, `hadOverflow()`, `hull_vertices()` |
| `src/fut_n6_algorithm.{h,cpp}` | `MAXREGION_FIX` 분기, `setBox()`, `setContainProvider()`, `calcDistanceRange()` |
| `src/frechet_under_translation.{h,cpp}` | 기저 사례에 상자(와 상자를 포함하는 원판의 제공자)를 넘긴다. LMF 범위 수정. `collectContainCenters()` |
| `tests/` | 반례 하네스 두 개와 술어 단위 테스트 |

### 1.1 상자 가족 (건전성)

이제 기저 사례는 잘리는 원판들의 부분집합 X 중 **∩X ∩ B ≠ ∅**인 것 가운데 포함 관계로 극대인 것을 열거한다. 여기서 B는 탐색 상자다. X ∪ {B}(모두 볼록)에 Helly 정리를 쓰면 이 조건은 세 술어로 줄어든다.

* P3(i,j,k): 세 원판이 한 점을 공유한다(candidate5의 술어, 변경 없음)
* Q1(i): D_i ∩ B ≠ ∅
* Q2(i,j): D_i ∩ D_j ∩ B ≠ ∅

증인점은 B의 점 가운데 max_{i∈X} |p − c_i|를 최소로 하는 점이다(MEC 중심이 B 안에 있으면 그 중심). 증인점은 B 안에 있으므로 B를 포함하는 모든 원판 안에 있다. 따라서 그런 원판은 찾을 필요가 없다.

*이것으로 충분한 이유.* p ∈ B를 YES 평행이동이라 하고, C를 잘리는 원판들이라 할 때 T = S(p) ∩ C라 하자. T ∪ {B}는 공통점 p를 가지므로 어떤 극대 X에 대해 T ⊆ X다. 이 X의 증인점 w는 S(w) ⊇ X ∪ {B를 포함하는 원판} ⊇ S(p)를 만족한다(B를 가로지르지도 포함하지도 않는 원판은 p를 포함할 수 없다). 판정자는 S에 대해 단조이므로 w는 YES다.

### 1.2 정확한 계산

정확 모드(`MAXREGION_EXACT=1`)에서는 모든 조합적 술어가 필터링된 정확한 술어다(엄밀한 오차 한계를 붙인 double 계산, `cpp_rational` 대체 경로, 제곱근 없음).

* Q1: 중심을 B로 clamp한다(정확). 그리고 반지름 r/2로 `pred2_exact`를 호출한다(반으로 나누기는 정확하다).
* Q2: c_i c_j의 중점이 B 안에 있으면(x_i + x_j − 2x_B의 정확한 부호로 판정) P2와 같다. 그렇지 않으면 렌즈가 B와 만나는 것은 렌즈가 B의 어떤 변과 만나는 것과 동치다. 변 위에서는 (chord_i, chord_j, edge)에 1-D Helly를 쓴다. 현–변 시험 두 개는 Q1 유형이다. 두 현의 겹침 조건은 |x_i − x_j| ≤ √h_i + √h_j이고, 이를 두 번 제곱해 4차 다항식으로 만든다.
* 증인점: 집합의 모든 원판 i에 대해 |w − c_i| ≤ r + tol임이 double로 인증될 때만 받아들인다. 그렇지 않으면 최적점을 정확하게 다시 계산하고(모든 후보 — 쌍의 중점, 세 점의 외심, 모퉁이, clamp한 사영, 변 위에서 clamp한 교점 — 가 유리수다) 반올림한다. tol = 판정자 여유 가운데 파이프라인 반지름이 이미 쓰고 있지 않은 부분이다(`MAXREGION_SLACK=0`이면 9e-9).

candidate5는 P2/P3는 인증했지만 MEC 증인점은 인증하지 않았다.

### 1.3 증인점 순서 (`union`, 기본값)

상자 가족만 쓰면 YES인 판정자 질의가 13–30% 느려졌다. candidate5의 상자 밖 증인점이 다른 곳에서 YES를 찾아 탐색을 일찍 끝내는 경우가 많았기 때문이다. `union`은 candidate5의 증인점을 먼저 시험하고, 그것이 모두 NO일 때만 상자 가족을 시험한다. 건전성은 상자 가족에서 나온다. candidate5 증인점은 비용 없는 조기 종료다(YES인 증인점은 어느 것이든 실제 평행이동이다).

### 1.4 LMF 범위 수정 (`N6_RANGE`, 기본 켜짐)

`original`과 candidate5에서 LMF 기저 사례는 `N6Alg::calcDistance(curve1, curve2)`를 호출한다. 이 함수는 (논문의 [ℓ_B, δ̃]가 아니라) [0, f(τ_start)]를 이분 탐색하고, 결과를 조건 없이 `max`에 대입한다. 그래서 δ̃보다 위에서 탐침이 한 번만 틀려도 전역 상한이 올라간다. 최대 f(τ_start) ≤ 2δ*까지다. `calcDistanceRange(ℓ_B, max)`는 [ℓ_B, max]만 탐색하고 `max = min(max, result)`로 둔다. 이것만으로 Characters(1,000쌍)에서 candidate5의 LMF가 3.6% 빨라진다.

## 2. 실행 시 설정 변수

| 환경 변수 | 기본값 | 의미 |
|---|---|---|
| `MAXREGION_FIX` | `union` | `union` \| `box` \| `none` (= candidate5) \| `full` \| `aware` \| `lazy` |
| `N6_RANGE` | `1` | `0`이면 `original/`의 LMF 기저 사례 탐색으로 돌아간다(저자 코드가 아니라 이 저장소에서 수정한 것) |
| `MAXREGION_EXACT`, `MAXREGION_SLACK`, `MAXREGION_BAND`, `MAXREGION_DP_LIMIT`, `MAXREGION` | candidate5와 같음 | 논문 설정은 `MAXREGION_EXACT=1 MAXREGION_SLACK=0`이다 |

`full`: 상자를 포함하는 원판을 생성기 입력에 더한다. `aware`: 그 원판들의 껍질(hull)을 공통 Helly 블록으로 쓴다. `lazy`: candidate5를 먼저 쓰고, NO를 인증할 수 없으면 `aware`를 쓴다. 세 방식 모두 정확하지만 실행시간 때문에 기각했다(4절).

## 3. 정확성 근거 (논문 설정)

| 테스트 | candidate5 | original | candidate6 |
|---|---|---|---|
| `tests/test_single_point_family 1 20000 6 6` — 판정자, δ = r*(1+u), u ∈ [1e-4, 1] | **78 NO** (정답 YES) | 0 | 0 |
| 같은 테스트, `LMF=1` — 값 대 정확한 r* | 0 | **75개 틀림 (≤ 2×)** | 0 |
| `tests/test_clustered_family 1 20000` — v(1+u)+1e-7에서 판정자, 각각 u 8개 | **893쌍에서 1,215 NO**, u = 3e-2까지 | 값: **63개 틀림 (≤ 2×)** | 0 NO, 값 = original+`N6_RANGE` |
| 무작위 곡선, n ≤ 10, 8,000쌍 × (값 + 질의 6개) | = original | — | = original |
| `tests/test_box_predicates` — Q1/Q2 대 독립된 유리수 기준 구현, 300,000개 사례(대부분 접하는 위치에서 ±3 ulp, 오프셋 1.36e7, 거의 수직인 쌍) | — | — | 불일치 0 |
| Characters LMF 1,000쌍, Sigspatial LMF 200쌍 | — | — | candidate5 대비 \|Δ\| ≤ 1e-7 |
| Characters 판정자 2,300질의, Sigspatial 판정자 920질의 (paperq4 파일 23개 각각의 처음 100 / 40개) | 모두 정답 | 모두 정답 | 모두 정답 |

실제 데이터에서 유리수 대체 경로가 쓰인 횟수는 다음과 같다. Q2는 379,726번 중 33번(Sigspatial LMF, 40쌍), 465,220번 중 0번(Sigspatial 판정자), 1,257,224번 중 1번(Characters LMF, 200쌍)이다. 증인점은 0번이다. 증인점이 r을 넘는 최대량은 모든 곳에서 0이다(Q1/Q2의 첫 판, 즉 느슨한 double 판은 Sigspatial에서 1.15e-8까지 넘어 9e-9 여유보다 컸다 — 지금 술어를 정확하게 만든 이유다).

## 4. 측정한 실행시간

컨테이너, 2 vCPU. 한 번에 프로세스 하나만 돌리고 코어 하나에 고정했다(앞서 두 스트림으로 돌린 측정은 편향됐다. 방식 순환 때문에 box 방식이 항상 `original`과 함께 스케줄됐다). 방식은 묶음마다 번갈아 돌렸다. g++ 13.3, CGAL 5.6, Boost 1.83, CMake 3.28, `RelWithDebInfo`, `MAXREGION_EXACT=1 MAXREGION_SLACK=0`. 부분집합은 커밋된 질의 파일의 앞쪽 행들이며, 3절에서 설명한 대로다.

| 벤치마크 | candidate5 | `box` | **`union` (기본값)** |
|---|---|---|---|
| Characters LMF, 1,000쌍 (`characters_uci_lmf_pairs.txt`에서 21번째마다) | 27.87 s | +5.5%, 기하평균 +6.7% [4.8, 8.6] | **+6.4%, 기하평균 +1.5% [−0.4, 3.4]** |
| Sigspatial LMF, 200쌍 (`sigspatial_pairs.txt`에서 5번째마다) | 10.61 s | +3.1% | **−1.1%**, 기하평균 −2.9% [−6.9, 0.8] |
| Characters 판정자, 2,300질의 | 15.47 s | +5.3% | **+2.2%** |
| Sigspatial 판정자, 920질의 | 64.22 s | +2.4% | **+1.4%** |

같은 호스트와 부분집합에서 `original` 대 candidate5는 Characters LMF 4.07×, Characters 판정자 2.06×, Sigspatial 판정자 1.115×다. **이 값은 컨테이너에서 부분집합으로 잰 것이다. 어떤 비든 인용하기 전에 논문 호스트에서 전체 질의 집합으로(`paper_bench/run_wsl_paired.sh`) 다시 재야 한다.**

기각한 대안, Characters LMF 처음 20쌍 (candidate5: 0.55 s): `full` 65.0 s, `aware` 2.09 s, `lazy` 2.10 s. 상자를 포함하는 원판들의 중심 껍질(hull)은 평균 51개 꼭짓점이었다. 그래서 Helly 블록 때문에 P3 호출이 0.28 M에서 21 M으로 늘었다. `lazy`의 인증은 기저 사례의 77%에서 실패했다.

## 5. 정밀도 예산

판정자는 반지름 + 9e-9까지 받아들인다(절댓값, `N6Alg` 여유 = 1e-8 − 1e-9). 관찰된 반지름은 Characters δ* 1.4–106(기저 사례 ≤ 98.5), Sigspatial(EPSG:3857 미터) δ* 664–48,550, 중앙값 13,180(기저 사례 ≤ 3.85e4)이다. 증인점 보장은 double로 나타낸 중심 c = π_i − σ_j에 대한 것이다. 판정자 자체는 원래 좌표로 계산한다. Sigspatial 좌표는 ≈ 1.36e7(ulp 1.9e-9)이고, 이는 여유의 5분의 1이다. 이 항은 `original`을 포함한 모든 방식에 공통이다.

어떤 실행에서도 쓰이지 않은 것: 상자 가족의 overflow 대체 경로(성분과 상자 변들의 배열). `overflow = 0`이 내내 유지됐다.

## 6. 재현 방법

```
bash _c6_build.sh                                          # -> ~/b_candidate5 (sic: overwrites candidate5's build)
ARMS="original candidate5 candidate6" bash paper_bench/build.sh
cmake -S candidate6/tests -B ~/b_c6_tests -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DCMAKE_CXX_FLAGS="-include cstdint -include array -include cstddef"
cmake --build ~/b_c6_tests -j
cd ~/b_c6_tests && export MAXREGION_EXACT=1 MAXREGION_SLACK=0
./test_box_predicates                                      # mismatches=0
./test_single_point_family 1 20000 6 6                     # fails=0
MAXREGION_FIX=none N6_RANGE=0 ./test_single_point_family 1 20000 6 6   # fails=78 (candidate5)
./test_clustered_family 1 20000                            # no 0 in the answer columns
```
