# 기준선 안내 — 이 저장소의 저자 코드 사본은 수정판이다

이 저장소에는 [BKN20] 저자 코드(GitLab `anusser/frechet_distance_under_translation`, master `3bbb305`)의 사본이 세 벌 있다.

| 위치 | 상태 |
|---|---|
| [`authors_gitlab/`](authors_gitlab/README.authors_gitlab.md) | **저자 원본.** 알고리즘 소스는 GitLab 그대로이고 빌드·측정 파일 3개만 이식했다. **기준선은 이것이다.** |
| [`original/`](original/MODIFIED_FROM_AUTHORS.md) | 수정판. 7개 파일이 다르고, 그중 `src/frechet_under_translation.cpp`·`src/fut_n6_algorithm.cpp`가 기저 사례에서 탐색 상자를 뺀다. |
| 저장소 최상위 `src/`·`lib/` | 수정판. GitLab과 다른 파일은 `src/frechet_under_translation.cpp` 하나다. `original/`과 같은 호출부 수정이다([`paper_bench/authors_check/reporoot_vs_gitlab.diff`](paper_bench/authors_check/reporoot_vs_gitlab.diff)). `fut_n6_algorithm.cpp`는 GitLab 그대로라서, 상자 없는 호출이 저자의 "전역 상자" 판을 부르는 세 번째 변형이 된다. |

세 사본의 차이, 측정, 논문과의 대조는 [`paper_bench/authors_check/REPORT.md`](paper_bench/authors_check/REPORT.md)에 있다.
기준선 빌드: `ARMS=authors_gitlab bash paper_bench/r4_original_vs_candidate7/scripts/build.sh`.

루트 `README`는 저자 파일이라 고치지 않았다.

## 실험 기록을 읽을 때 주의할 점

1. 2026-10-03 이전 문서에서 `original`, A0, "기존 구현", "저자 코드"로 부른 기준선은 모두 **수정판 `original/`** 이다. "알고리즘 수정 없음"이라는 서술은 틀렸다.
2. 수정판 기준 수치 가운데 Sigspatial LMF(12 GB 실패 2쌍, 총 시간 비), 결정 문제 호출 수(논문 Table 2와 −13~−16 %), 큰 값 오답(revisit 7개)은 **수정 때문**이다. 저자 코드 기준 수치는 `authors_check/REPORT.md` 4절을 쓴다.
3. Characters LMF처럼 차이가 1–2 %인 수치와, 수정판과 저자 코드에 공통인 결함(무한 루프 C, 원 잘림 B, assert F·H, 1e8 평행이동의 큰 값)은 수정판 기록으로도 결론이 바뀌지 않는다. 기존 문서의 수치는 지우지 않고 "정정" 표시만 덧붙였다.
