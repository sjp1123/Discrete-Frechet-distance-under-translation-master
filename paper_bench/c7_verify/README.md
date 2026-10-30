# candidate7 검증 키트

`paper_bench/results/C7_report.md`의 근거가 된 하네스와 입력이다. 소스는
2026-09-26/27의 candidate6 감사에서 가져왔고, 바꾸지 않았다.

```
bash paper_bench/c7_verify/verify.sh [OUT=~/c7_verify_out]     # build if needed, run all, summarise
bash paper_bench/c7_verify/summarize.sh [OUT]                  # summary again
QUICK=1 bash paper_bench/c7_verify/verify.sh ~/c7_quick        # smoke test, counts / 10
```

하네스는 `~/b_c7v`에, paper_bench는 `~/b_pb_candidate7`에 빌드한 다음 작업을 12개씩 동시에 돌린다.
candidate7은 기본 설정으로 돌며, 이 기본 설정이 논문 설정이다. 전체 실행은 18스레드 테스트 PC에서
약 20분 걸린다. CGAL, Boost, GMP(gmpxx)가 필요하다. WSL Ubuntu 22.04, g++ 11.4, CGAL 5.4에서 시험했다.

| 파일 | 검사 내용 | 감사 지적 사항 |
|---|---|---|
| `src/test_exact_ref.cpp` | 값과 판정자를 모든 후보 평행이동에 대한 정확한 전수 탐색과 대조 (가족 1–5) | F08, F03 시드 |
| `src/fuzz_oracle.cpp` | 판정자(δ*(1 ± u))와 값을 독립적인 정확한 오라클(커플링에 대한 유리수 MEC)과 대조. 생성기 11개, depth/cut 변형, `MAXREGION_DP_LIMIT=2` | E2, F01 |
| `src/test_far.cpp` | 앞쪽 cut 원판 13개가 잘못된 원판인 가족 (깊이 제한에 의한 잘림) | F01 (B) |
| `src/test_one.cpp`, `src/test_diag.cpp`, `src/test_light.cpp` | 동률에서 하위 판정자가 내는 틀린 YES와 그로 인한 abort | F08 (F) |
| `src/probe.cpp` | 배율을 바꾼 인스턴스와 2^24 16각형에서 LMF가 종료하는지 | F04, G3-1 (D) |
| `src/e2e.cpp` | X = 2^50에서 한 원 위에 놓인 격자점 180개에 대한 LMF (폭 0인 상자) | F09 (E) |
| `cases/a/case_sig_*.txt`, `cases/a/HIT_*.txt` | 인증된 증인점이 거부된 Sigspatial 규모의 2×3 쌍. 판정자와 LMF | F02, G1-1 (A) |
| `cases/g4/` | GPS 곡선과 그 평행이동 사본: δ*+2e-7에서 틀린 NO, δ*−2.5e-7에서 멈춤(hang), \|t\| ≈ 1.4e7에서 assert | G4-2, G4-3, G4-1 (B, C, H) |
| `cases/near_threshold/` | 모든 Characters 쌍과 Sigspatial 쌍에서 candidate6 자신의 LMF 값 ± 1e-6 (정답: plus는 YES, minus는 NO) | E3 |
| `candidate7/tests/test_box_predicates.cpp`, `test_single_point_family.cpp`, `test_clustered_family.cpp` (방식의 소스에서 빌드한다. `candidate7/tests/`의 다른 테스트는 여기서 돌리지 않는다) | candidate6에 딸려 온 테스트: 상자 술어, 단일점 가족, 군집 가족 | candidate6의 README |

케이스 파일에는 `<delta>`, 이어서 `<n>`과 점 n개, 이어서 `<m>`과 점 m개가 들어 있다. verify.sh가
이를 paper_bench 곡선 파일로 바꾼다.
