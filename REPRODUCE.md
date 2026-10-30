# 극대 셀 축소 실험 재현하기

`_x1_results/`의 모든 결과는 `_x1_results/ENVIRONMENT.md`에 기록된 환경에서 나왔다.
바이너리를 다시 빌드하고 핵심 수치를 다시 만들 때는 이 파일 하나에서 시작한다. 시드는 모든 곳에서 고정되어 있어서
조합적 개수는 비트 단위로 재현된다. 실제 경과 시간(wall-clock)의 비는 ENVIRONMENT.md §2에서 분석한
잡음 범위 안에서 재현된다(반복 변동계수(CV)의 중앙값 ≈ 4%).

## 0. 준비물

- WSL2 Ubuntu 24.04(또는 아무 Linux), g++ 13, CMake 3.28, CGAL 5.6, GMP/MPFR,
  Boost 1.83, Python 3(표준 라이브러리만 쓴다 — numpy/scipy는 필요 없다).
- Geolife Trajectories 1.3 데이터셋을
  `original/test_data/benchmark/Geolife Trajectories 1.3/data/` 아래에 둔다(**커밋하지 않았다** —
  2.2 GB이므로 따로 구해야 한다). 쌍 매니페스트는 `test_cases/geolife_100/`에 있다.
- 매니페스트의 파일을 네이티브 파일 시스템 디렉터리 `~/geodata`로 복사한다(/mnt/c에서
  실행할 때마다 드는 오버헤드를 피한다): `_cp_manifest_data.sh`.
- 참고(2026-10-21): 아래 스크립트 대부분은 `ROOT=/mnt/c/Users/shiju/…`를 하드코딩했다(또는
  `/home/sjp/_x1/…`를 읽는다). 다른 곳에서 돌리려면 그 경로를 고쳐야 한다. 체크아웃 위치를 스스로 찾는 것은
  `_build_one.sh`와 `reproduce.sh`뿐이다(`reproduce.sh`도 하드코딩된 스크립트를 여전히 호출한다).

## 1. 모든 방식 빌드

```
bash _build_one.sh original        # A0  (Epeck, DCEL, no maximal)  -> ~/b_original
bash _build_one.sh candidate2      # D0/D1 (double list; MAXIMAL_MODE=off/on)
bash _c4_build.sh                  # C4  (candidate4, + ctest)      -> ~/b_candidate4
# X2 kernel arm (B0 = original with Epick):
bash _x2_build_epick.sh            # -> ~/b_original_epick
```

방식은 **실행 시점**에 고른다(같은 바이너리, 설정값(knob) 하나). 그래서 요인 대비는
똑같은 코드 경로끼리 비교한다.

| 설정값 | 방식 |
|---|---|
| `MAXIMAL_MODE=off` (candidate2) | D0 |
| `MAXIMAL_MODE=on`  (candidate2, 기본값) | D1 |
| `MAXIMAL_MODE=exact` | X3 정확 열거 (검증용) |
| `MAXREGION=cech` (candidate4, 기본값) | C4 |
| `MAXREGION=legacy` | candidate4 안에서 candidate2와 같은 동작 |
| `MAXREGION=verify` | 둘 다 돌리고 집합족(family)을 교차 검사 |
| `CUT_LIMIT=<n>` | arrangement_cut_limit 값을 바꿔 가며 실행 (X3-b) |

## 2. 정확성 관문 (가장 먼저 돌린다 — 빠르다)

```
( cd ~/b_candidate4 && ctest -R maximal_regions --output-on-failure )  # unit suite
bash _oracle_check.sh        # A0/D1/C4 vs code-independent brute-force oracle
bash _c4_check.sh 30         # answers within eps; MAXREGION=legacy==c2; verify families
```

## 3. 핵심 수치

```
bash _factor_perpair.sh 100   && python3 _factor_analyze.py   # X1/X2 factor table (A0,B0,D0,D1)
bash _c4_bench.sh             && python3 _c4_analyze.py        # candidate4 (A0,D1,C4S,C4N,C4)
bash _noise.sh 10 8                                            # timing-noise characterization
```

## 4. 심화 분석 (선택, 더 느림)

```
python3 _x3_gen.py "8,12,16,20" 6 ; bash _x3a_run.sh "8 12 16 20" 6 240  # exact scaling (gamma)
python3 _x3_analyze.py ; python3 _r56_analyze.py                          # X3-a / R5 / R6
bash _x3q_run.sh 12 ; python3 _r4_analyze.py                              # C_q~ply^alpha
python3 _reanalyze.py                                                     # R1 ceiling / R2 / R3
```

## 5. 명령 하나로 (재빌드 + 관문 + 핵심 수치)

```
bash reproduce.sh
```

## 시드

- `_gen_wall.py`: `random.seed(7)` · `_gen_nsweep.py`: `random.seed(2026)` ·
  `_x3_gen.py`: 쌍마다 `random.Random(1000+idx)` · candidate4 MEC 셔플:
  `xorshift(0x9E3779B9 ^ n)` (고정).

## 알려진 한계 (ENVIRONMENT.md 참고)

- 호스트가 하나뿐이고 WSL의 주파수 governor를 제어하지 않았다 → 실제 경과 시간 비에 반복 간 잡음이
  ≈4% 있다. 여러 기계 / 베어메탈에서 다시 실행하는 것이 아직 남은 외부 단계다.
- 핵심 가속비는 geolife_100 / `fut_lmf` / `cut_limit=12` 조건의 값이다. 전역
  `n6` 경로와 더 큰 `cut_limit`은 동등(parity) / 미구현이다(candidate4 §7).

## 기준선 빌드 (2026-10-03)

논문 기준선은 `authors_gitlab/`에서 빌드한다. 이것은 [BKN20] 저자의 GitLab 코드다
(커밋 3bbb305; 알고리즘 소스는 그대로이고 CMake/CGAL 5 이식과 `steady_clock`만 바꿨다).
빌드는 `ARMS=authors_gitlab bash paper_bench/r4_original_vs_candidate7/scripts/build.sh` → `~/b_pb_authors_gitlab/paper_bench`이다.
`original/`(위의 A0)은 기저 사례에서 탐색 상자를 빼 버린 수정 사본이다.
[`BASELINE_NOTICE.md`](BASELINE_NOTICE.md)와 `paper_bench/authors_check/REPORT.md`를 참고한다.
