# candidate7과 original의 정확성 검증 보고서

> **정정 (2026-10-03):** 이 문서의 기준선 `original`은 [BKN20] 저자 코드 그대로가 아니다. `original/`의 `src/frechet_under_translation.cpp`·`src/fut_n6_algorithm.cpp`가 기저 사례에서 탐색 상자를 빼도록 수정되어 있다. 저자 코드([`authors_gitlab/`](../../../authors_gitlab/), GitLab 3bbb305) 기준 수치는 [`paper_bench/authors_check/REPORT.md`](../../authors_check/REPORT.md) 4절을 본다. 아래 수치는 수정판 기준 기록으로 남겨 둔다.

2026-10-01, r4와 같은 서버 컨테이너(Xeon 2.1 GHz, 2 vCPU, Ubuntu 24.04, g++ 13.3, CGAL 5.6, Boost 1.83). 검증한 바이너리는 r4 측정에 쓴 것과 같은 빌드(`RelWithDebInfo`)이고, candidate7은 논문 설정(`MAXREGION=cech MAXREGION_EXACT=1 MAXREGION_SLACK=0`, 기본값)으로 돌렸다. 코드는 고치지 않고 결과만 보고한다.

## 0. 결론

**candidate7.** 논문이 쓰는 두 진입점, 값 계산 `calcDistance2`(LMF)와 결정 문제 `lessThan`에서 오답, 크래시, 무한 루프를 하나도 찾지 못했다.
- **새 오라클 비교:** 이번에 새로 짠 독립 정확 오라클과 17,553개 인스턴스(값 17,553개, 결정 질의 241,434개)에서 비교했다. 실제 곡선을 잘라 만든 인스턴스, 1e-3~1e8 규모의 좌표, 퇴화·동률 입력을 모두 포함한다.
- **실제 데이터:** 5,100쌍의 값을 저자 δ*와 대조하고 번역 인증서를 확인했다. δ* 바로 옆 질의 44,000개도 돌렸다.
- **기존 검증:** ASan/UBSan, 기존 감사 키트, candidate7 회귀 테스트, 저자 단위 테스트를 모두 통과했다.

남는 한계는 세 가지이고, 모두 original도 똑같이 갖는다.
1. **동률·해상도 이하 간격.** δ가 δ*와 정확히 같거나 좌표의 배정밀도 해상도보다 가까우면(δ − δ* < 16·u·(|좌표| + δ)) NO가 나올 수 있다. 1 ulp만 올려도 YES다.
2. **극단적 근-퇴화 입력에서의 느림.** 점이 10⁻⁷ 이내로 뭉친 군집 입력에서는 n, m ≤ 8인데도 값 계산이 최대 55 s 걸린다. original도 최대 50 s다.
3. **측정에 쓰지 않는 진입점의 버그.** 이분 탐색 기준선 `calcDistance`가 짧은 곡선에서 값을 크게 틀린다(0.9 대신 1.5). 저자 코드에서 물려받은 버그로 두 구현에 똑같이 있다.

**original(저자 코드).** 감사에서 문서화된 결함들이 이번에 무작위 입력에서 그대로 재현됐다. 모두 candidate7에서는 사라진다.
- **무한 루프(결함 C):**
  - 군집·재방문 인스턴스에서 결정 질의 86개
  - 실제 Sigspatial 데이터의 δ*±10⁻⁶ 질의 16개
  - 키트 재현 입력
- **틀린 값:**
  - 점을 다시 지나는 곡선: 9쌍(값 0 반환 포함, 최대 오차 3.04)
  - 곡선이 1e8 떨어진 경우: 26쌍(최대 +17 %)
  - 키트 재현 입력
- **assert 크래시(결함 F):** 재방문 인스턴스 2쌍, 키트 `one_4697`
- **틀린 NO:** 군집 7개, 1e8 평행이동 10개, 키트(작은 depth/cut 설정) 463개
- **이분 탐색 무한 반복(결함 D):** 좌표 6.7e7 이상
- **assert(결함 H):** 1.4e7 평행이동

**논문 수치에는 영향이 없다.** 실제 데이터의 모든 측정 인스턴스에서 두 구현 모두 오답이 없었다.
- **값:** 저자 δ*와 1.33×10⁻⁸ 이내, 두 구현끼리 8.6×10⁻⁹ 이내였다.
- **번역 인증서:** 보고한 평행이동에서의 DFD가 값보다 최대 1.4×10⁻⁸ 크다.
- **결정 문제:** r4의 138,000 질의가 모두 정답이었다.

다만 original은 Sigspatial에서 δ* ± 10⁻⁶처럼 아주 가까운 질의 16개를 끝내지 못한다(논문의 질의 집합에는 없는 거리).

## 1. 검증 항목

| # | 항목 | 규모 (구현마다) | candidate7 | original |
|---|---|---|---|---|
| 1 | 독립 정확 오라클 (새로 작성) | 17,553 인스턴스, 값 17,553 + 결정 질의 241,434 | 값 오류 0 (최대 \|v−δ*\| 2.3e-8), 틀린 YES 0, 틀린 NO 0, 미종료·크래시 0 | 값 오류 35 (최대 3.04), 크래시 2, 틀린 NO 17, 미종료 86 |
| 2 | 실제 데이터 앵커 | 5,100쌍 (Characters 4,100 + Sigspatial 1,000/998) | 저자 δ*와 ≤1.33e-8, 인증서 ≤1.06e-8, 위반 0 | 저자 δ*와 ≤1.33e-8, 인증서 ≤1.38e-8, 위반 0 |
| 3 | 임계값 근처 실제 데이터 (v ± 1e-6) | 44,000 질의 | 오답 0, 미종료 0 | 오답 0, 미종료 16 (Sigspatial) |
| 4 | ASan + UBSan 빌드 | 30,191 인스턴스·질의 (실제 데이터 + 8개 가족) | 보고 0 | 메모리 오류 0. UBSan 520건은 모두 CGAL 5.6 헤더(배열 DCEL 다운캐스트) |
| 5 | 기존 감사 키트 (`c7_verify`, QUICK = 1/10 규모) | 53개 작업 | 전부 통과 | 결함 C, D, F, H 재현, 값 오류 16, 틀린 NO 463 (아래 3절) |
| 6 | candidate7 회귀 테스트 (`candidate7/tests`) | 15개 작업 | 전부 통과 | 해당 없음 |
| 7 | 저자 단위 테스트 `testFrechetUnderTranslation` (축소) | Sigspatial 20곡선 = 210쌍, 격자 101×101 | 통과 | 통과 |
| 8 | 독립 코드 리뷰 (서브에이전트, 읽기 전용) + 재현 | candidate7 변경분 전체, original 기저 사례 | 측정 경로 결함 없음. 저위험 지적 2건 (3절 #6, #7) | 문서화된 결함 확인. 새 결함 2건 (#1, #5) |

### 1.1 독립 정확 오라클 (`scripts/oracle.cpp`)

**정의에서 바로 짰다.** 저장소 코드는 쓰지 않았다.
- 정의: d(P, Q) = min_t DFD(P, Q + t).
- 커플링 하나를 고정하면 최적 t는 차분점 {p_i − q_j}의 최소 외접원(MEC) 중심이다. 이 중심은 점 하나, 두 점의 중점, 세 점의 외심 가운데 하나다.
- 따라서 모든 후보 중심 c에 대한 DFD(P, Q + c)의 최솟값이 정확한 답이다. 후보마다 값은 상계이고, 최적 커플링의 MEC 중심이 후보에 들어 있다.

**계산 방법.**
- 차분점을 d_00 기준으로 옮긴 뒤 long double로 걸러낸다.
- 최솟값 근처의 모든 후보를 GMP 유리수로 정확히 다시 계산한다. 결과는 δ*²의 정확한 유리수다.

**오라클 자체 점검.**
- 손으로 풀 수 있는 사례 5개에서 맞았다.
- 걸러내기 허용 오차를 바꾼 두 판을 303개 인스턴스에서 비교했다. 차이는 거의 0인 사례 하나뿐이었고(3.97e-15 대 3.86e-15), 이런 오차는 항상 위쪽으로만 난다.
- 이 정도 오차는 판정 기준(아래)에 영향이 없다.

**인스턴스 가족.** 가족마다 250~1,500개, 곡선 길이는 1~12, 차분점은 최대 144개다.
- **무작위:** uniform, grid(정수 좌표, 동률·공원), cluster(10⁻⁹~10⁻² 반경으로 뭉친 점), collinear, copy(평행이동 사본, 잡음 0~10⁻³), singleton, revisit(같은 점을 다시 지나는 곡선)
- **좌표 규모:** offset_sig(좌표 1.36e7), big(±1e5), tiny(±1e-3)
- **실제 곡선에서 잘라냄:** real_chars, real_sig, medium_real_chars, medium_real_sig(8~12점)
- **곡선 사이 거리:** far_t_1e7, far_t_1e8, far_t_sig(곡선끼리 1.4e7~1e8 떨어짐)
- **리뷰 재현:** review(코드 리뷰가 지목한 입력 3개)

**판정 기준.** u = 2⁻⁵³, M = 인스턴스의 최대 |좌표|.

| 대상 | 판정 |
|---|---|
| 값 | \|v − δ*\| ≤ 1e-7 + 4e-15·M |
| 결정, δ ≥ δ* + 16·u·(M + δ) | 반드시 YES |
| 결정, δ < δ* − (1.5e-8 + 1e-15·M) | 반드시 NO (구현의 허용 오차 9e-9와 반올림을 감안) |
| 결정, 그 사이 | 판정하지 않고 따로 센다 (동률·해상도 이하) |

결정 질의는 인스턴스마다 12~14개다.
- **YES 쪽:** δ*·{2, 1+1e-2, 1+1e-4, 1+1e-6}, δ* + 2e-8, 정확한 동률 δ* 자체, δ* + 1e-12
- **NO 쪽:** δ*·{0.5, 1−1e-2, 1−1e-4, 1−1e-6}, δ* − 5e-8, δ* − 2e-7, 동률 바로 아래

**가족별 결과는 `results/oracle_families*.txt`, 실패 목록은 `results/oracle_failures*.txt`에 있다.**

| | candidate7 | original |
|---|--:|--:|
| 값: 오류 / 크래시·미종료 | 0 / 0 | 35 / 2 |
| 결정: 반드시 YES인데 NO | 0 | 17 |
| 결정: 반드시 NO인데 YES | 0 | 0 |
| 결정: 미종료(질의당 45~120 s) | 0 | 86 |
| 동률·해상도 이하 질의에서 NO / YES | 6,485 / 26,180 | 5,825 / 26,837 |

original의 실패가 몰린 곳은 다음과 같다.
- **revisit:** 값 오류 9, 크래시 2, 미종료 48
- **cluster:** 틀린 NO 7, 미종료 38
- **far_t_1e8:** 값 오류 26, 틀린 NO 10

나머지 가족에서는 두 구현 모두 오류가 없다.

### 1.2 실제 데이터 앵커 (`results/real_data_anchors.txt`)

두 구현의 값과 반환 평행이동 t를 기록했다(`scripts/patch_pb_translation.py`로 harness에 두 열만 추가). 대상은 저자의 결정 문제 쌍 Characters all/same 1,000+1,000, Sigspatial 1,000(original은 998), `characters_full`에서 10번째마다 뽑은 2,100쌍이다.
- **저자 δ*와의 대조:** 저자 `*_computed_distances.check`와의 최대 차이는 1.33e-8이다.
- **번역 인증서:** 독립 DP(`scripts/dfdcert.cpp`)로 DFD(P, Q + t)를 계산했다. 값 v보다 항상 크거나 같고(최소 +1.3e-13), 최대 +1.38e-8을 넘지 않는다. 기준 1e-7 위반은 0이다.
- **구현 간 일치:** 두 구현의 값 차이는 최대 8.6e-9이다.
- **r4와의 일치:** 값과 호출 수가 r4 측정과 2,100/2,100 비트 단위로 같다.

### 1.3 임계값 근처 실제 데이터 (`results/near_threshold.txt`)

질의는 감사에서 만든 v ± 1e-6이다(v는 candidate6의 값이고, 정답은 + 쪽 YES, − 쪽 NO).

| 구현 | Characters 42,000 | Sigspatial 2,000 |
|---|---|---|
| candidate7 | 오답 0, 미종료 0 | 오답 0, 미종료 0 (최대 21 s) |
| original | 오답 0, 미종료 0 | 오답 0, **미종료 16** |

original이 끝내지 못한 질의 하나(`file-020126`/`file-001221`)는 candidate7이 1.7 s에 끝냈다. 같은 질의를 original로 10분 넘게 돌린 뒤 gdb로 스택을 떠 보니, depth-limit 분기(`frechet_under_translation.cpp:469` `computeCutCenters` → `cut_centers.size() == 0` → `continue`)를 반복하고 있었다. 결함 C의 무한 루프다.

### 1.4 새니타이저 (`results/sanitizers.txt`)

`-fsanitize=address,undefined`로 두 구현의 paper_bench를 빌드했다. 입력은 다음과 같다.
- Characters 값 300 / 결정 2,300
- Sigspatial 값 45 / 결정 460
- 오라클 가족 8개에서 값 200~300 / 결정 2,000~4,000

결과:
- **candidate7:** 보고 0건.
- **original:** ASan 메모리 오류 0건. UBSan 520건은 모두 CGAL 5.6의 배열 코드 헤더 3개(`Arr_construction_ss_visitor.h`, `Arrangement_2_iterators.h`, `Arrangement_on_surface_2_impl.h`)에서 DCEL 포인터를 다운캐스트하는 vptr 검사였고, 프로젝트 코드에서는 0건이다. LeakSanitizer 보고 20건(1,040블록)은 모두 UBSan이 진단을 출력하면서 쓴 demangler 메모리였다.

### 1.5 기존 키트 · 회귀 테스트 · 단위 테스트

**감사 키트 `paper_bench/c7_verify` (QUICK, 원래 규모의 1/10, `results/kit_quick_*.txt`).**
- **candidate7:** 53개 작업이 모두 통과했다.
  - exact_ref 2,200 인스턴스, fuzz_oracle 4,600 인스턴스: 오류 0
  - 감사 재현 입력 A~H: 모두 정상
- **original:** 결함 재현 결과는 3절 표에 있다.

**candidate7 회귀 테스트 (`results/candidate7_regression_tests.txt`).** 모두 통과했다.

| 테스트 | 결과 |
|---|---|
| box 술어 | 300,000건 중 불일치 0 |
| 단일점 가족 | 결정 20,000 / 값 20,000, 실패 0 |
| 군집 가족 | 20,000, 0 답 0 |
| depth-limit | −13, −8, 8 각 200, 실패 0 |
| 접선 창 | 배율 1·3·8 각 3,102 질의, NO 0 |
| 증인점 여유 | 20,000 + 20,000, 거부 0 |
| 큰 값 | 1e8·1e9에서 정확 MEC 대비 상대 오차 ≤ 1.2e-15 |
| 퇴화 상자 | 통과 |

**저자 단위 테스트 (`results/authors_unit_test_reduced.txt`).** 저자 `testFrechetUnderTranslation`을 줄여서 돌렸다. Sigspatial 100곡선 대신 20곡선(210쌍), 격자 1001² 대신 101²다. 검사 내용은 `calcDistance`와 `calcDistance2`의 일치, 번역 인증서, 격자 위 음성 검사다. 두 구현 모두 TEST_FAILED 0이었다.

## 2. 코드 리뷰 (서브에이전트) 결과의 검증

| 리뷰 지적 | 재현 결과 |
|---|---|
| **#1 양쪽:** 이분 탐색 진입점 `calcDistance`의 kd-tree 검색 하한이 음수가 되어(제곱 시 부호 소실) cut 원판을 놓친다 | **확인.** P = [(0,0),(0,−0.3),(0,1.5),(0.1,0)], Q = (0,0)×4, 정답 0.9(오라클). `calcDistance`는 두 구현 모두 1.5000000042, `calcDistance2`는 0.8999999992 / 0.8999999972 (`results/calcDistance_entry_bug.txt`). 논문과 측정에는 쓰지 않는 진입점이다 |
| **#5 양쪽:** 초기 탐색 상자 폭이 0이면 빈 상자로 보고 NO | **확인.** P = [(0,0),(2,0)], Q = [(0,0)]에서 `lessThan(1.0)`(δ = δ*)이 두 구현 모두 NO. 동률(1 ulp)에서만 생기고, 오라클 가족에서도 같은 부류가 6,000건대(1.1절 마지막 행) |
| **#6 candidate7:** A′ 반올림 상한에 원판 중심 c = fl(p − q)의 반올림이 빠져 있다 (곡선이 ~1e8 떨어지면 [δ*, δ*+~1e-8]에서 NO 가능) | **해상도 밖에서는 관찰 안 됨.** far_t_1e8 800개에서 δ* + 2e-8에 800/800 YES, 해상도 밖 틀린 NO 0. 동률 근처(δ* ~ δ*+1e-12, 1e8에서는 1 ulp ≈ 1.5e-8 이하)에서는 NO 362/800(original 184/800). #5와 같은 해상도 한계 부류다. Sigspatial(곡선끼리 ≲1e5)과는 무관 |
| **#7 candidate7:** 상자 없는 N6 경로(`fut_n6_algorithm.cpp:201`)가 Čech 열거를 쓰는데, README에 없고 MEC 증인점이 인증되지 않는다 | 측정 경로 밖이다(`calcDistance2`/`lessThan`의 기저 사례는 항상 상자가 있다). 따로 실험하지 않았다 |
| **#2~#4 original:** 결함 B, C, D, F, H와 LMF 기저 사례 결함 | C, D, F, H와 LMF 값 오류를 이번에 재현했다(3절). B는 키트의 작은 depth/cut 설정에서 틀린 NO 463개로 나타났다. 기본 설정의 감사 재현 입력 `b_g4_f01`은 original에서 YES(정답)였다 |
| **#8 candidate7:** depth limit에서 `union`이 배열을 두 번 만들 수 있다 | 성능 문제이고 정확성과는 무관하다 |

리뷰가 맞다고 확인한 부분: box family의 Helly 환원(P3/Q2/Q1), 2^m DP의 완전성, 술어의 오차 상한(P2 8u, P3 6u, 현 겹침 16u/64u), 증인점이 항상 상자 안에 있음, `calcDistanceRange`와 `union` 경로, 수정 F가 틀린 NO를 만들지 않음.

## 3. 발견 사항 (심각도 순)

| # | 구현 | 내용 | 재현 | 논문 수치 영향 |
|---|---|---|---|---|
| 1 | original | 결정 문제 무한 루프 (결함 C: depth-limit 상자에 cut 원판이 없으면 `step()` 없이 `continue`) | 오라클 cluster 38, revisit 48. 실제 Sigspatial δ*±1e-6 16. 키트 `c_g4_f03`, `exact_f5b`, fuzz 5. 예: `cluster/00075` δ = δ*−5e-8 | 없음 (논문 질의는 모두 끝남) |
| 2 | original | 값 계산이 크게 틀림. 재방문 곡선에서 5e-8(정답 2.46, 3.04) 또는 +6e-5~+0.14. 곡선이 1e8 떨어지면 +2e-6~+1.64(17 %) | `revisit/00257`, `00883`, `00498`, `far_t_1e8/00352`, 키트 `exact_f2`(0.43), `sp_lmf` 10/2,000 | 없음 (실제 데이터 5,100쌍 값 정상) |
| 3 | original | `calcDistance2`에서 assert `!init_search_box.empty()` (결함 F: 동률에서 고정 평행이동 판정기의 거짓 YES) | `revisit/00145`, `00689`, 키트 `one_4697`, `diag`(lt(e) = 1) | 없음 |
| 4 | original | 틀린 NO (δ ≥ δ*(1+1e-6), δ* + 2e-8) | cluster 7, far_t_1e8 10, 키트 작은 depth/cut 설정 463 (결함 B) | 없음 (r4 138,000 질의 정답) |
| 5 | original | 큰 좌표에서 이분 탐색 무한 반복(D), 1.4e7 평행이동에서 assert(H) | 키트 `hang_*`, `circ_g3*`, `h_g4_assert` | 없음 |
| 6 | 양쪽 | 이분 탐색 진입점 `calcDistance`의 cut 원판 누락 (리뷰 #1) | 위 2절 | 없음 (LMF만 사용) |
| 7 | 양쪽 | 동률(δ = δ*)이나 좌표 해상도 이하 간격에서 NO (리뷰 #5) | `review/00001`, `00002`, 오라클 가족의 동률 질의 | 없음 |
| 8 | 양쪽 | 근-퇴화 군집에서 매우 느림 (n, m ≤ 8에서 값 계산 최대 55 s / 50 s) | `cluster/00584`, `00163` (`hang/`) | 없음 (성능) |
| 9 | candidate7 | A′ 상한에서 중심 반올림 누락, 상자 없는 N6 경로 (리뷰 #6, #7) | 해상도 밖에서는 재현 안 됨 | 없음 |

정정: #2의 revisit 큰 값 오답 7개와 Sigspatial 12 GB 실패는 `original/` 수정 때문이며 저자 코드에는 없다(계측: original은 같은 쌍에서 전역 상한을 42–2,491번 올리고 저자 코드는 0번). 0 근처 값 2개, assert(F), 무한 루프(C), 원 잘림(B), 1e8 평행이동의 큰 값은 저자 코드에도 있다([`authors_check/REPORT.md`](../../authors_check/REPORT.md) 4.5).

## 4. 재현

```
# 빌드: ../scripts/build.sh (paper_bench, 두 구현) 후
g++ -O2 -std=c++17 scripts/oracle.cpp -lgmpxx -lgmp -o oracle
python3 scripts/gen.py inst 1 && python3 scripts/gen2.py inst2          # 인스턴스
./oracle inst/oracle_list.txt > inst/oracle_out.txt                      # 정확한 δ*
python3 scripts/make_queries.py inst                                     # 결정 질의와 정답 부류
python3 scripts/runner.py <arm> lmf|decider inst/<fam>.pairs|.q inst/<fam> res/<fam>_<lmf|dec>_<arm>.csv   # 미종료/크래시 내성
python3 scripts/check.py                                                 # 표 1.1 (VD=inst2 VR=res2로 추가 가족)
# 실제 데이터: patch_pb_translation.py로 만든 harness로 값과 t 기록 → dfdcert → real_check.py
```

스크립트 맨 위의 경로(저장소 위치, 작업 디렉터리 `/root/verif`, 바이너리 `~/b_pb_<arm>`)는 이번 실행 환경 기준이므로 실행할 곳에 맞게 바꾼다. `raw/`에 인스턴스, 오라클 출력, 모든 실행의 CSV, 새니타이저 로그가 있다. `scripts/runner.py`는 인스턴스마다 시간 제한을 두고(값 150 s, 결정 45~120 s), 끝나지 않으면 HANG으로 기록한 뒤 다음 인스턴스로 넘어간다.
