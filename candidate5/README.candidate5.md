# Candidate 5 — `original` + Čech 극대 영역 열거, Epeck 대체 경로

> 정정 (2026-10-21): `original/`은 저자 코드를 수정한 사본이다(기저 사례에서 탐색 상자를 버리고 [0, f(τ_start)]를 이분 탐색한다). candidate 5도 이를 그대로 물려받는다. [`../paper_bench/authors_check/REPORT.md`](../paper_bench/authors_check/REPORT.md)를 보라.

Candidate 5는 **`original/`을 그대로** 두고, 후보 영역 생성기만 candidate 4의
Čech 파이프라인으로 바꾼 것이다.  나머지는 바뀌지 않는다.  Epeck 커널, CGAL DCEL
배열 코드, 분할 정복 드라이버, kd-tree, sub-Fréchet 판정자, `CUT_LIMIT`/`DEPTH_LIMIT`
설정값이 모두 같다.  파이프라인의 DP가 다루기에 너무 큰 성분은 **Epeck 배열**로
되돌려 넘긴다.  그래서 파이프라인이 답하지 않는 곳에서는 이 방식이 구성상 정확하다.

이전 작업에서는 이 방식을 만든 적이 없다.  Candidate 4는 파이프라인을 candidate 2의
**double** 재작성 위에 얹었다.  그래서 배열이 이미 싸진 *뒤의* 파이프라인 가치만
잴 수 있었다.  Candidate 5는 논문이 실제로 쓰는 산술을 상대로 그 가치를 잰다.

---

## 1. `original/` 대비 바뀐 것

| 파일 | 변경 |
|---|---|
| `lib/cgal_disk_arrangements/maximal_regions.h` | **candidate4에서 그대로 복사** — 파이프라인 API. 나중에(32c9cf3) 선택형(opt-in) 정확 모드의 `exact` 매개변수와 카운터 |
| `lib/cgal_disk_arrangements/maximal_regions.cpp` | **candidate4에서 그대로 복사** — Stage 0–5. 나중에(32c9cf3) 선택형 정확 모드(`MAXREGION_EXACT`) |
| `lib/cgal_disk_arrangements/test_exact_predicates.cpp` | **신규** (나중에, bc1b22b) — 정확 모드 P2/P3의 경계 사례 테스트 |
| `lib/cgal_disk_arrangements/disc_arrangement_traversal.cpp` | 디스패처 + Epeck 대체 경로. 기존 생성자에서는 모두 원래 꼭짓점 열거를 그대로 유지 |
| `lib/cgal_disk_arrangements/disc_arrangement_traversal.h` | 생성자 하나 추가(여유를 반영) |
| `lib/cgal_disk_arrangements/CMakeLists.txt` | `maximal_regions.cpp` 빌드 |
| `src/fut_n6_algorithm.cpp` | **한 줄**: `epsilon_slack`을 traversal에 넘김 |

`maximal_regions.{h,cpp}`는 candidate 5를 만들 때(05a0f01)는 candidate 4의 것과
바이트 단위로 같았다.  나중에 커밋 32c9cf3이 선택형 정확 모드(`MAXREGION_EXACT=1`,
기본값 꺼짐)를 더해서, 지금은 다르다.  이 파일들은 `disc_arrangement_traversal.h`의
`Point` / `Disc` / `Discs`에만 의존한다.  이것들은 `original/`에 이미 있으므로
따로 손볼 필요가 없었다.

아이디어, Helly-3 / 같은 반지름 술어 체계, 단계 구성은
`../candidate4/README.candidate4.md` §2에 정리되어 있으므로 여기서는 되풀이하지 않는다.

## 2. 실행 시 설정값

| 환경 변수 | 기본값 | 의미 |
|---|---|---|
| `MAXREGION` | `cech` | `cech` \| `legacy` (= `original` 그대로, 대조 방식) |
| `MAXREGION_BAND` | `1e-12` | `r²`에 대한 상대 허용 폭 |
| `MAXREGION_DP_LIMIT` | `20` | 이보다 큰 성분은 Epeck 대체 경로로 간다 |
| `MAXREGION_SLACK` | *(호출자가 정함)* | 술어 여유를 덮어쓴다. `0`이면 여유 정렬을 끈다 |
| `MAXREGION_EXACT` | `0` (꺼짐) | `1` = `cpp_rational` 대체 경로가 있는 필터링된 P2/P3. band는 무시한다 (32c9cf3에서 추가, 아래 측정 이후) |

`MAXREGION=legacy`는 같은 바이너리 안에서 원래 코드 경로를 되살린다.  아래 §4에서
보듯이 이 경로는 `original`의 답을 비트 단위로 재현하고, 측정 가능한 비용도 없다.
그래서 대비가 깨끗하다.

## 3. Epeck 대체 경로는 **부풀리지 않은** 반지름에서 돌아야 한다

`N6Alg`는 반지름 `distance`인 원판을 만들지만, 판정자에게는
`distance + epsilon_slack`을 묻는다.  그래서 파이프라인은 `distance + slack`에서
열거한다(candidate 4 §5).  대체 경로는 이 부풀림을 물려받으면 **안 된다**.

배열 꼭짓점은 정확히 원 *위에* 놓인다.  부풀린 반지름에서는 그 점의 `sqrt` 구성
오차(~1.5e-8·r)가 여유(~9e-9)보다 크다.  그래서 판정자가 그 꼭짓점을 만든 바로 그
쌍을 놓칠 수 있다.  이것이 candidate 4의 `C4S` 병리 현상이다(§6, "Caveat on `C4S`").
게다가 아무 경고 없이 일어난다.

`test_cases/bench_small`에서 대체 경로가 강제로 동작하게 하고 쟀다
(`MAXREGION_DP_LIMIT=4`, 15쌍에서 대체 경로 성분 758개).

| 대체 경로 반지름 | `original` 대비 답 |
|---|---|
| 부풀림 (`radius + slack`) | 최대 **2.7e-4** 어긋남 — 11/15쌍이 ε = 1e-7을 넘음 |
| 원래 반지름 | **비트 단위로 같음**, 15/15 |

그래서 `build_candidates_cech`는 대체 경로의 부분 벡터를 항상 `discs`에서 잘라 내고,
부풀린 사본에서는 절대 잘라 내지 않는다.  파이프라인은 부풀린 집합을 쓴다.  두 집합은
일부러 서로 다른 객체로 둔다.

## 4. 실행시간 측정 — `geolife_100` (실제 데이터)

`_c5_bench_geo.sh` + `_c5_analyze.py`로 쟀다.  **candidate 4를 잰 것과 같은
`geolife_100` manifest와 같은 곡선 파일**을 썼고, `fut_lmf`, **100쌍 × 4 방식 ×
3회 반복**이다.  반복마다 방식 순서를 다시 섞었고, (쌍, 방식)마다 최솟값을 취했다.
60 s에서 중도 절단했다(절단 0건).  쌍별 로그 비 → 기하평균, 10 000회 재표본
bootstrap 95% CI를 구했다.  `_c4_bench.sh` / `_factor_perpair.sh`와 같은 절차다.

방식: `A0` = `original`, `C5L` = candidate5 `MAXREGION=legacy`, `C5N` =
`MAXREGION=cech MAXREGION_SLACK=0`, `C5` = `MAXREGION=cech` (기본값).

> 곡선 파일은 커밋되어 있지 않다.  공식 Geolife 1.3 zip에서
> `authors_gitlab/test_data/benchmark/geolife_converter.py`로 다시 만들었다.  candidate 4의
> 데이터와 같다는 것은 세 가지로 확인했다.  `original`의 답이 `_c4_bench.csv`의
> `A0` 열과 **100쌍 모두에서 비트 단위로 같다**(정확한 Epeck 출력이다.  좌표 하나만
> 달라도 20번째 자리가 움직인다).  manifest 파일 200개가 모두 있다.  manifest의
> `n` 열이 max(curve length)와 맞는다.

| 대비 | WALL | ARRANGEMENT | INTERNAL (arr+fre) | 판정자 호출 |
|---|---|---|---|---|
| **`A0` → `C5`** | **1.69× [1.52, 1.89]** | 48.2× [36.8, 63.4] | 36.9× [28.4, 47.9] | 4.49× [3.72, 5.44] |
| 파이프라인만 (`A0/C5N`) | 1.24× [1.14, 1.34] | 22.4× | 17.0× | 2.70× |
| 파이프라인이 있을 때 여유 (`C5N/C5`) | 1.36× [1.22, 1.53] | 2.16× | 2.17× | 1.66× |
| 대조 (`A0/C5L`) | 1.00× [0.97, 1.01] | 0.98× | 0.98× | 1.000× |

**86/100**쌍에서 빨라졌다.  중앙값 1.50×, p90 3.88×다.  100쌍 전체의 절대값은
wall **68.0 s → 39.7 s**, arrangement **12.37 s → 0.61 s**, internal
**21.07 s → 1.11 s**, 판정자 호출 **4 000 135 → 1 201 618**이다.

`original` 대비 답: max |Δ| **1.11e-8**, ε = 1e-7을 넘는 쌍 **0/100**.
`C5L`은 100/100쌍에서 비트 단위로 같다.

### candidate 2, candidate 4와 비교 — 같은 기계, 같은 라운드 로빈

`_c5_bench_all.sh` + `_c5_analyze_all.py`는 **네 방식을 모두 이 호스트에서 이
툴체인으로** 다시 빌드하고, 한 번의 실행 안에서 쌍마다 번갈아 돌린다.  그래서
Amdahl 상한을 정하는 항, 즉 손대지 않은 코드의 비용이 모든 방식에서 같다.
기본 점검: `A0`, `D1`, `C4`는 `_c4_bench.csv`에 기록된 답을 비트 단위로 재현한다.
따라서 이 방식들은 다시 빌드했을 뿐 같은 프로그램이다.

| 대비 | WALL | ARRANGEMENT | INTERNAL | 판정자 호출 |
|---|---|---|---|---|
| `original` → candidate 2 | 1.27× [1.21, 1.34] | 12.9× | 5.25× | 2.08× |
| `original` → candidate 4 | 1.69× [1.52, 1.88] | 43.6× | 34.0× | 4.53× |
| **`original` → candidate 5** | **1.70× [1.53, 1.89]** | 48.1× | 36.7× | 4.49× |
| candidate 2 → candidate 5 | 1.33× [1.22, 1.47] | 3.74× | 6.98× | 2.16× |
| **candidate 4 → candidate 5** | **1.01× [1.00, 1.02]** | 1.10× | 1.08× | 0.99× |

100쌍 전체의 절대값:

| 방식 | wall | arrangement | arr+fre | 나머지 (wall − internal) | 판정자 호출 |
|---|--:|--:|--:|--:|--:|
| `A0` original | 72.71 s | 13.44 s | 22.85 s | 49.86 s | 4 000 135 |
| `D1` candidate 2 | 54.40 s | 1.08 s | 6.52 s | 47.88 s | 2 002 463 |
| `C4` candidate 4 | 42.06 s | 0.67 s | 1.23 s | 40.83 s | 1 192 443 |
| `C5` candidate 5 | **41.56 s** | 0.66 s | 1.21 s | 40.35 s | 1 201 618 |

**Candidate 4와 candidate 5는 구별되지 않는다.**  wall에서 1.01×, CI
[1.00, 1.02]다.  모든 절대 합계에서 차이가 2% 안이고(wall 1.2%, arrangement 1.5%,
arr+fre 1.7%), 판정자 호출 수에서는 1% 안이다.  Čech 파이프라인이 들어가면
candidate 2의 float 재작성으로 얻는 것은 없다.  candidate 5는 손대지 않은 exact-Epeck
트리에서 출발해 같은 지점에 도달한다.  재작성이 그 자체로 *가졌던* 가치는 1행의
1.27×다.  그리고 파이프라인이 이를 대체한다(`D1` → `C5` = 그 위에 1.33×).

> internal 지표에서 candidate 5가 조금 앞서는 것(1.08×)은 실제 알고리즘 차이가
> 아니다.  candidate 4의 판정자 루프에는 질의마다 도는 `chrono` 계측과 `x3q_row`
> 호출이 있고, candidate 5에는 없다.  `C5 > C4`가 아니라 `C4 ≈ C5`로 읽어야 한다.

이 파일의 이전 판은 candidate 5의 측정을 `_c4_bench.csv`에 *기록된* candidate 4의
숫자와 비교해서 1.69× 대 1.98×로 읽었다.  그 차이는 호스트 때문에 생긴 허상이었다.
그 기록을 만든 기계는 손대지 않은 코드의 비용이 26.6 s였고, 여기서는 49.9 s다.
함께 재면 두 방식은 비긴다.  호스트를 넘나드는 비교는 어디에도 남겨 두지 않았다.
`_c5_bench_all.sh`를 써라.

### 여기서는 합성 데이터와 달리 여유 정렬이 중요하다

두 요인은 1.24× (파이프라인) × 1.36× (여유) = 1.69×로 곱해진다.  §5의 합성
`bench_100`에서는 여유 정렬이 전혀 유의하지 않다.  메커니즘은 candidate 4 §5에 있다.
실제 궤적에는 반복되는 점과 거의 반복되는 점이 있다.  그래서 정렬하지 않은
파이프라인은 판정자에게 필요한 것보다 훨씬 많은 영역을 낸다.  쌍 `13962/4238`에서는
여유가 없으면 영역 2 606개, 있으면 **65**개였고, 시간은 606 ms 대 207 ms였다.
합성 무작위 보행(random-walk) 곡선에는 이런 퇴화가 없으므로, 정렬이 합쳐 줄 것이 없다.

## 5. 실행시간 측정 — 합성 `bench_100`

절차는 같다.  `test_cases/bench_100`(합성 100쌍, n ∈ [100, 1000])에서 120 s 중도
절단(절단 0건)으로 쟀다.  결과는 `_c5_bench.csv` / `_c5_analysis.txt`로 커밋했다.
geolife 측정은 `_c5_bench_geo.csv` / `_c5_analysis_geo.txt`다.

| 대비 | WALL | ARRANGEMENT | INTERNAL (arr+fre) | 판정자 호출 |
|---|---|---|---|---|
| **`A0` → `C5`** | **1.50× [1.42, 1.58]** | 32.8× [28.9, 37.5] | 8.52× [7.62, 9.60] | 3.65× [3.36, 3.99] |
| 파이프라인만 (`A0/C5N`) | 1.47× [1.40, 1.55] | 30.2× | 7.87× | 3.40× |
| 파이프라인이 있을 때 여유 (`C5N/C5`) | 1.02× [0.98, 1.06] | 1.09× | 1.08× | 1.07× |
| 대조 (`A0/C5L`) | 1.01× [0.99, 1.03] | 0.99× | 1.00× | 1.000× |

**95/100**쌍에서 빨라졌다.  중앙값 1.48×, p90 1.99×다.  100쌍 전체의 절대값은
wall **59.4 s → 39.8 s**, arrangement 단계 **5.03 s → 0.17 s**, internal
**20.5 s → 2.34 s**, 판정자 호출 **1 055 717 → 287 713**이다.

처음부터 끝까지 두 번 돌렸다.  1.508×와 1.498×로, `_x1_results/ENVIRONMENT.md` §2에서
측정한 반복 간 잡음(~4%) 안에서 재현된다.

### 이 측정에서 파이프라인만으로 효과가 나는 이유

Candidate 4에서는 파이프라인만으로는 효과가 없었고(`D1/C4N` = 0.98×), 1.44× 전부가
여유 정렬에서 나왔다.  합성 곡선에서는 반대다.  파이프라인만으로 1.47×이고 여유
정렬은 유의하지 않다.

두 결과는 서로 모순되지 않는다.  이유는 파이프라인이 무엇을 대체하느냐에 있다.
candidate 2의 double 리스트에서는 arrangement 단계가 이미 100쌍에 ~625 ms까지 줄어
있었다(X1 §3, 방식 D1.  115 ms는 D0로, 같은 리스트에서 극대 필터를 끈 것이다).
그래서 더 빠른 생성기가 얻을 것이 남지 않았고, 탐색 궤적만 바뀔 여지가 있었다.
`original`에서는 Epeck 배열이 내부 루프의 주된 비용 *그 자체*다.  그래서 이것을
없애면 바로 이득이 난다.  그다음 두 데이터 집합이 가르는 것은 여유 정렬에 얼마가
*남느냐*다.  geolife에서는 실제 궤적의 퇴화 덕분에 1.36×가 더 생기고(§4), 합성
무작위 보행에서는 아무것도 없다.

전체 실행 기준 수치가 arrangement 이득보다 훨씬 작은 것은 R1에 기록한 흔한 Amdahl
이유 때문이다.  N6 루프는 wall에서 차지하는 몫이 작다.

## 6. 정확성

`original`(정확한 Epeck) 대비, 각 측정의 100쌍에 대해:

| 방식 | max \|Δ\| geolife | max \|Δ\| 합성 | ε = 1e-7을 넘는 쌍 | `C5L` 비트 단위 일치 |
|---|---|---|---|---|
| `C5L` | 0.00e+00 | 0.00e+00 | 0/100 | **100/100** |
| `C5N` | 1.09e-08 | 7.13e-09 | 0/100 | — |
| `C5`  | 1.11e-08 | 1.15e-08 | 0/100 | — |

`_c5_check.sh <case_dir> [N]`가 점검 항목을 돌린다.  기본값 대 `original`, §3의 강제
대체 경로, `C5L` 비트 일치 점검이다.

## 7. 동작 한계

* **`fut_lmf` 경로에서는 대체 경로가 쉬고 있다.**  `CUT_LIMIT = 12`,
  `MAXREGION_DP_LIMIT = 20`이면 모든 build에서 `overflow = 0`이다(벤치마크 20쌍에서
  확인).  따라서 §4/§5의 숫자는 파이프라인만의 결과다.  Epeck는 측정된 경로의 일부가
  아니라 안전망으로 들고 있는 것이다.
* **전역 `n6` 진입점에서는 성능이 같다.**  여기서는 원판 집합이 n₁·n₂ 곱 전체이므로
  모든 성분이 넘치고, 모든 일이 Epeck 대체 경로로 간다.  `_nsweep` n = 6…20에서
  잰 결과, 답은 `original`과 **비트 단위로 같고** wall은 **0.84×–1.07×, 중앙값 ≈ 1.00×**다.
  결론은 candidate 4 §6과 같고 처방도 같다.  `n6`을 참조용 이외로 쓰려면 먼저 설계
  노트의 §5(Bron–Kerbosch + clique별 DP)를 구현해야 한다.
* `CUT_LIMIT`가 `MAXREGION_DP_LIMIT`보다 크면 성분이 DP 한도를 넘을 수 있다.  일단
  넘으면 파이프라인이 통째로 꺼지고 사전 처리(pre-pass) 오버헤드만 남는다.
  candidate 4 §7을 보라.  그 내용이 그대로 적용된다.

## 8. 재현 방법

```
bash _c5_build.sh                                   # -> ~/b_candidate5
bash _build_one.sh original                         # -> ~/b_original  (baseline)
bash _c5_check.sh  test_cases/bench_small 15        # correctness gates

# geolife_100 (real data).  The curve files are NOT committed: run
# authors_gitlab/test_data/benchmark/fetch_and_convert_data.py, then _cp_manifest_data.sh
# to stage the manifest's files in a native-FS ~/geodata (_cp_manifest_data.sh
# still hard-codes its source as original/test_data/benchmark/...; edit SRC).
bash _c5_bench_geo.sh 100 3 ~/_c5/bench_geo.csv     # A0 vs C5L/C5N/C5
python3 _c5_analyze.py ~/_c5/bench_geo.csv          # -> _c5_analysis_geo.txt

# ranking against candidate2 / candidate4 -- all four arms, one round-robin.
# Build them here too, with the SAME toolchain flags, or the comparison is
# confounded by the host:
bash _build_one.sh candidate2 ; bash _c4_build.sh
bash _c5_bench_all.sh 100 3 ~/_c5/bench_all.csv
python3 _c5_analyze_all.py ~/_c5/bench_all.csv      # -> _c5_analysis_all.txt

# synthetic bench_100 (committed data, no download needed)
bash _c5_bench.sh  test_cases/bench_100 100 3 ~/_c5/bench.csv
python3 _c5_analyze.py ~/_c5/bench.csv              # -> _c5_analysis.txt
```

커밋된 출력은 모두 쌍별 원자료다.  `_c5_bench_all.csv` /
`_c5_analysis_all.txt`(네 방식 순위), `_c5_bench_geo.csv` /
`_c5_analysis_geo.txt`(geolife 요인 분해), `_c5_bench.csv` /
`_c5_analysis.txt`(합성).

**§4 숫자에 쓴 환경** — WSL2 Ubuntu, g++ 15.2, CGAL **6.1.1**,
CMake 4.2.3, `RelWithDebInfo`.  이것은 `_x1_results/ENVIRONMENT.md`의 g++ 13 / CGAL 5.6
툴체인이 *아니다*.  CGAL ≥ 6은 C++17을 요구하는데 이 트리는 C++14를 요청한다.  그래서
`-std=c++14`를 `-std=c++17`로 올렸다.  이 변경은 두 방식에 똑같이 적용했고,
`_c5_bench_all.sh`용으로 다시 빌드한 candidate 2와 candidate 4에도 적용했다.  따라서
어떤 대비도 영향을 받지 않는다.  커밋된 `CMakeLists.txt`는 다른 방식과 맞추려고
C++14로 두었다.  여기의 절대 시간은 g++ 13 / CGAL 5.6 호스트에서 `_c4_bench.csv`나
`_x1_results/`에 기록한 숫자와 비교할 수 **없다**.  같은 100쌍에서 `original`은
그쪽에서 39.5 s, 여기서 72.7 s 걸린다.  방식 사이의 주장은 반드시 함께 잰 방식들에서
나와야 한다.

**데이터.**  §4는 `geolife_100`에서 쟀다.  이전의 모든 방식과 같은 manifest, 같은
곡선 파일이므로, 공유하는 `A0` 기준선을 통해 candidate 2, candidate 4와 직접 비교할
수 있다.  §5는 커밋된 합성 `bench_100`에서 쟀다.  저장소에 있는 `bench_100.out`(다른
기계)은 그 데이터에서 `original` → candidate 2를 1.24×로 기록했다.
