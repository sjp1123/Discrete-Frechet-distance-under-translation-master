# authors_gitlab — [BKN20] 저자 원본 코드 (기준선)

이 폴더는 [BKN20] 저자가 공개한 코드다. 논문 기준선(LMF, 결정 문제)은 이 폴더로 빌드한다.

## 출처

- 저장소: <https://gitlab.com/anusser/frechet_distance_under_translation>
- 브랜치·커밋: `master`, `3bbb30502e201564bf9859806cda94198f8ac0d4` ("Add license.", André Nusser)
- 받은 파일: GitLab의 Code → zip (`frechet_distance_under_translation-master.zip`). zip 주석의 커밋이 위 해시와 같다.
- 파일 날짜: 1,570개 모두 2020-06-27

**알고리즘 소스는 GitLab 그대로다.** `src/`와 `lib/cgal_disk_arrangements/*.cpp`를 포함해 아래 3개 빌드·측정 파일 말고는 한 바이트도 바꾸지 않았다.

## 덮어쓴 파일 3개 (`original/`에서 가져옴)

| 파일 | 바뀐 내용 | 이유 |
|---|---|---|
| `CMakeLists.txt` | `cmake_minimum_required` 2.8.8 → 3.5, `-std=c++11` → `c++14` | CMake 3.28·CGAL 5.6에서 빌드되게 하는 이식 |
| `lib/cgal_disk_arrangements/CMakeLists.txt` | CGAL 4식 `CGAL_LIBRARIES` → CGAL 5 타깃 | 같은 이식 |
| `lib/measurement_tool/measurement_tool.h` | `high_resolution_clock` → `steady_clock` | 다른 방식과 같은 시계로 재기 위함. 측정에만 관여 |

셋 다 알고리즘과 무관하다. GitLab 원래 판과의 차이는 `../paper_bench/authors_check/original_vs_gitlab.diff`의 해당 부분과 같다.

## 뺀 파일

| 파일 | 이유 |
|---|---|
| `tools/CMakeCache.txt`, `tools/CMakeFiles/` | 저자 기계의 빌드 찌꺼기 |
| `src/.swp` | 편집기 임시 파일 |
| `test_data/` 중 `benchmark/`를 뺀 나머지 (`character_classification_data/`, `decider_benchmark_queries/`, `fut_decider_benchmark_queries/`, `fut_val_computation_benchmark_queries/`, `queries_small.txt`) | `../original/test_data/`의 것과 `diff -rq`로 같음을 확인했다. 중복이라 넣지 않았다 |

`test_data/benchmark/`(데이터 받기·변환 스크립트 4개)와 `experiments/`(저자 배포 결과, `../original/experiments/`와 같음)는 넣었다.
`test_data/benchmark/`는 저장소 `.gitignore`의 `**/test_data/benchmark/` 규칙에 걸리므로 `git add -f`로 넣었다.

## 해시 확인

zip을 푼 폴더 안에서:

```
unzip -q frechet_distance_under_translation-master.zip -d /tmp/gl
cd /tmp/gl/frechet_distance_under_translation-master
sha256sum -c <저장소>/paper_bench/authors_check/gitlab_3bbb305_sha256.txt   # 1,570개 모두 OK
```

이 폴더 자체는 위 "덮어쓴 파일"과 "뺀 파일"만 다르다. 나머지 파일은 같은 해시 목록으로 확인할 수 있다
(이 폴더 안에서 `sha256sum -c --ignore-missing`을 돌리면 덮어쓴 3개만 FAILED가 난다).

## 빌드

```
ARMS=authors_gitlab bash paper_bench/r4_original_vs_candidate7/scripts/build.sh   # -> ~/b_pb_authors_gitlab/paper_bench
```

## 이 판이 논문 수치를 재현한다

Characters 21,000쌍(논문 Table 4)에서 블랙박스 호출 **260,128,449회(12,387.1/쌍)** 로 논문 Table 4와 같다.
결정 문제 4^ℓ 세트(Table 2)의 호출 수도 논문과 −2.1~+1.6 % 안이다.
[`../paper_bench/authors_check/REPORT.md`](../paper_bench/authors_check/REPORT.md) 4.1절·4.3절을 본다.

저장소의 `original/`은 이 코드의 수정판이다(기저 사례에서 탐색 상자를 뺌). [`../original/MODIFIED_FROM_AUTHORS.md`](../original/MODIFIED_FROM_AUTHORS.md)와 [`../BASELINE_NOTICE.md`](../BASELINE_NOTICE.md)를 본다.
