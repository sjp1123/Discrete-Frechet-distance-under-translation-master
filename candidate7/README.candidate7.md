# Candidate 7 — 감사에서 확인된 결함을 고친 candidate6

candidate7은 candidate6(상자로 제한한 극대 집합 기저 사례, 커밋 fb69a13의 `README`)에서 2026-09-26/27 candidate6 감사가 확인한 결함을 모두 고친 것이다. 각 결함은 독립된 검증자 두 명이 재현했다. 알고리즘은 바뀌지 않았다. 수정은 국소적이고, 소스에서 각각 `FIX (candidate7)`로 표시했다.

## 1. 수정 사항

| id | 결함 (감사 결과) | 위치 | 수정 |
|---|---|---|---|
| A | 상자 가족 증인점을 r + 9e-9까지 받아들였다. 이는 판정자 자신의 문턱값이므로, 판정자의 원래 좌표 반올림(1.4e7에서 ~1e-9)이 그 증인점을 기각할 수 있었다. Sigspatial 규모로 구성한 입력에서 1.01·δ*에서 틀린 NO가 나왔고, LMF 값이 2.7% 크게 나왔다(F02, G1-1). candidate5에도 같은 판정자 실패가 있다. | `lib/cgal_disk_arrangements/maximal_regions.cpp` `enumerate_box` | double 증인점은 r 이내로 인증될 때만 받아들인다. 그렇지 않으면 정확한 최적점(집합이 실현 가능하므로 ≤ r)을 반올림한다. 판정자는 여유 전체를 자신의 반올림에 쓴다. |
| B | 판정자의 깊이 제한 기저 사례에 잘리는 원판 목록이 13개에서 끊긴 채 전달됐다. 전수 탐색 `computeCutCenters`가 `threshold=false`를 무시했기 때문이다. 그 결과 δ < δ*(1+1.35e-6)에서 틀린 NO가 나왔고, 평행이동한 GPS 곡선에서는 절댓값으로 δ*+2e-7까지 나왔다(F01, G4-2). `original`에서 물려받은 결함이다. | `src/frechet_under_translation.cpp`, 전수 탐색 루프 두 곳 | kd-tree 분기가 이미 그러듯이, `threshold`가 설정됐을 때만 일찍 멈춘다. |
| C | 같은 분기가 잘리는 원판이 없는 상자에서 무한 루프에 빠졌다(`step()` 없는 `continue`) (F03, G4-3). 물려받은 결함이다. | `lessThanImpl` | `continue` 앞에 `search_boxes.step()`을 둔다. |
| D | max와 min이 간격 ε/2 이상인 이웃한 double이 되면 이분 탐색이 끝나지 않았다. 이 일은 LMF 값이 약 2^22(original) 또는 2^25(candidate6)부터, 그리고 거친 하한 계산에서는 약 1e7부터 생긴다(F04, G3-1). 물려받은 결함이다. | 루프 6개: `fut_n6_algorithm.cpp` ×3, `discrete_frechet_queries.h`, `discrete_frechet_light.cpp`, `frechet_under_translation.cpp` | 중점이 구간의 한쪽 끝과 같으면 멈춘다. |
| E | 상자 가족의 overflow 대체 경로가 폭이나 높이가 0인 상자에서 비정상 종료했다(SIGSEGV). 이런 상자는 한 변이 1 ulp 폭이 되면 생긴다(F09). candidate6에서 새로 생긴 결함이다. | `disc_arrangement_traversal.cpp` `build_candidates_box` | 퇴화하지 않은 상자 변만 넣는다. 즉 선분 하나를 넣고, 점 상자면 아무것도 넣지 않는다. 점 상자는 그 자신이 증인점이다. |
| F | 고정 평행이동 판정자의 free/non-free 휴리스틱은 반올림된 접두 길이와 비교한다. 정확히 같은 경우(δ가 끝점 거리와 같을 때) 이 때문에 propagation2를 거쳐 거짓 YES가 나왔고, calcDistance2가 빈 초기 상자에서 중단됐다(F08). 물려받은 결함이다. | `discrete_frechet_light.{h,cpp}` | 여러 점에 걸친 휴리스틱 단계(maxdist > 0)마다 8u·((n+2)·L + max\|x\| + δ)의 마진을 준다. 이는 접두합, 좌표, 제곱의 반올림에 대한 상한이다. 점 하나짜리 단계는 직접 시험 그 자체이므로 그대로 둔다. 거기에도 마진을 넣은 첫 판은 틀렸다. `C7_report.md` §4 참고. |
| H | 상자 변의 길이가 몇 ulp일 때 \|t\| ≈ 1.4e7에서 `intersection_interval`이 begin < 0을 반환할 수 있었고, 강제 assert에서 중단됐다(G4-1). 물려받은 결함이다. | `src/geometry_basics.cpp` | 빠른 경로의 진입/진출 매개변수를 [0, 1]로 clamp한다. 그다음은 기존 이분 탐색이 결정한다. |
| G | 기본 설정(환경 변수 없음)은 판정자 자신의 반지름에서, 증인점 허용오차 0과 느슨한 double 술어로 열거했다. 그 결과 0.5–2 % 마진에서 틀린 NO가 나왔고, LMF 값이 최대 2.1 % 크게 나왔다. Sigspatial 쌍 3개도 포함된다(F05, G2-1). | `disc_arrangement_traversal.cpp` | 이제 논문 설정이 기본값이다. `MAXREGION_EXACT`의 기본값은 1, `MAXREGION_SLACK`의 기본값은 0이다. `MAXREGION_SLACK=caller`로 예전 동작을 쓸 수 있다. |
| knobs | 알 수 없는 `MAXREGION_FIX` 값이 아무 경고 없이 건전하지 않은 candidate5 경로를 골랐다. 또 `N6_RANGE`는 맨 앞 글자가 `0`일 때만 꺼졌다(F10). | `src/fut_n6_algorithm.h`, `disc_arrangement_traversal.cpp` | 알 수 없는 값은 경고를 출력하고 기본값을 유지한다. `N6_RANGE`는 0/1, off/on, false/true, no/yes를 받는다. |

바뀌지 않은 것: Lipschitz 탐색, 원판 선택, kd-tree, candidate6의 열거와 술어(정확한 계산임이 확인됐다. 가족 비교 117,600회와 술어 사례 약 4백만 개에서 불일치 0), `depth_limit`/`arrangement_cut_limit` 생성자 매개변수(기본값 40, 12), `-std=c++14`. CGAL ≥ 6에는 모든 방식과 마찬가지로 `-std=c++17`이 필요하다. 저자 트리의 Vim 스왑 파일 `src/.swp`는 복사하지 않았다.

## 2. 설정

환경 변수는 필요 없다. 기본값이 논문 설정이다(`MAXREGION=cech`, `MAXREGION_EXACT=1`, `MAXREGION_SLACK=0`, `MAXREGION_FIX=union`, `N6_RANGE=1`). 예전 값도 실험용으로 계속 쓸 수 있다.

## 3. 빌드

```
bash _c7_build.sh                                  # -> ~/b_candidate7
ARMS="original candidate7" bash paper_bench/build.sh   # paper_bench per arm
```

## 4. 근거와 실행시간 측정

- `paper_bench/results/C7_report.md`: 모든 감사 결과의 재현기, 정확한 기준 구현과 비교한 퍼징, δ*의 ±1e-6 안에 있는 실제 데이터 질의.
- `paper_bench/results/C7_timing.md`: Characters와 Sigspatial 전체 실행을 `original`, `candidate5`와 짝지어 비교한 결과다. 모든 답과 값, 그리고 논문 표 2와 4 형식의 실행시간을 담는다. 원자료는 `raw_c7_timing.tar.gz`에 있다. 계획, 실행기, 분석 스크립트는 `paper_bench/plan_wsl_c7.py`, `run_wsl_c7.sh`, `analyze_c7.py`다.

## 5. 병합 뒤 후속 작업

병합된 candidate7 위에 얹은 수정 두 개(`FIX (candidate7 follow-up)`로 표시)와, `tests/`로 옮겨 온 candidate6 감사 수정의 회귀 테스트다.

| id | 문제 | 위치 | 수정 |
|---|---|---|---|
| A′ | 수정 A는 반올림한 정확한 증인점이 r + 9e-9, 즉 여유 전체를 통과하면 그대로 썼다. 그래서 판정자 자신의 반올림에 남는 것이 없었다. 그 검사마저 실패하면 코드는 횟수만 세고(`mec_fail`) 증인점을 내보냈다. double 증인점은 좌표가 아무리 커도 r 이내면 받아들였다. | `maximal_regions.cpp` `enumerate_box`, `disc_arrangement_traversal.cpp`, `fut_n6_algorithm.{h,cpp}` | N6은 상자 점에서 판정자 반올림의 상한 2u(max\|p\| + R) + 6uR(p는 곡선 1의 점, R = δ + slack)을 넘긴다. 증인점 허용오차는 slack − bound가 되며, 음수일 수 있다. 두 수용 검사 모두 이 값을 쓴다. double 증인점은 min(r, r + tol) 이내, 반올림한 정확한 증인점은 r + tol 이내여야 한다. 실패한 영역은 증인점을 유지하고, 그 원판들은 상자 배열 대체 경로로도 간다. 그래서 판정자는 그 집합에 대해 original의 후보도 시험한다. |
| E′ | E 수정은 점 상자의 점을 넣었지만, 여전히 성분의 원들로 배열을 만들었다(테스트의 원판 24개에서 후보 601개). | `disc_arrangement_traversal.cpp` | 점 상자는 자기 자신만 낸다. 배열 코드는 `append_box_arrangement_vertices`로 옮겼고, 두 대체 경로 원인이 이를 공유한다. |

max|p| + R이 약 4e7(9e-9 / 2u)보다 작은 동안 허용오차는 ≥ 0이다. 그때 두 검사는 병합된 판의 검사와 비트 단위로 같다. Sigspatial은 max|p| ≈ 1.4e7이다.

근거(명령은 `tests/CMakeLists.txt`와 각 파일 머리말에 있다):

- 논문 부분집합(Characters LMF 1,000쌍, Sigspatial LMF 200, Characters 판정자 2,300질의, Sigspatial 판정자 920): 값, 답, `bbcalls`가 병합된 candidate7과 같고, 시간은 잡음 범위 안에서 일치한다.
- `paper_bench/c7_verify/verify.sh` (QUICK=1): 모든 작업이 통과한다. A/B/C/H 재현기는 병합된 판과 같은 답과 값을 낸다.
- `test_witness_margin`(설정마다 원판 세 개짜리 사례 20,000개): 다시 돌린 판정자가 기각한 증인점은 0개다. 병합된 빌드와 후속 빌드 모두 그렇다. 대체 경로는 두 곡선이 모두 6e7 또는 1.2e8에 있을 때만 작동한다(최적점이 상자 안에 있으면 20,000개 중 20,000개, 변 위에 있으면 253개와 1,021개). 1.36e7이나 3e7에서는 한 번도 작동하지 않는다.
- `test_large_values` 1e8: 동일하다. 1e9에서는 대체 경로가 영역 46개에서 작동하고, 값 300개 중 8개가 1 ulp 움직인다. 정확한 MEC 반지름 대비 최대 상대 오차는 1.2e-15에 머문다.
- `test_tangency_windows`, 원래 좌표 규모 1, 3, 8(far 0/1, 각 3,102질의): NO 0개.
- `test_degenerate_box`: 점 상자의 후보 601 → 1개. 다른 상자는 그대로다.
- `test_box_predicates`, `test_single_point_family`(판정자와 LMF), `test_clustered_family`, `test_depth_limit`(−8, −13, 8): 시간 말고는 출력이 같다.

병합된 candidate7이 내보내고 판정자가 기각하는 증인점을 찾은 테스트는 없다. 따라서 A′는 관찰된 실패의 수정이 아니라, 논문 데이터 범위를 넘는 좌표에 대한 안전장치다.

| 테스트 | 대상 | 실행 |
|---|---|---|
| `test_depth_limit` | B, C | `test_depth_limit 1 200 -13` (m < 0: cluster-major 순서, `TEST_ALARM`은 질의당 5 s 알람을 덮어쓴다) |
| `test_large_values` | D | `test_large_values 1 300 1e8` → `S seed value exact_MEC rel_err` |
| `test_degenerate_box` | E, E′ | 인자 없음 |
| `test_tangency_windows` | A | `test_tangency_windows 1 150 [far] [scale]` |
| `test_witness_margin` | A′ | `test_witness_margin <count> [edge] [offset] [same_city]`; `_nofix`는 여유 전체를 넘긴다 |
