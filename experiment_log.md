# DFDuT 극대 셀 축소 — 실험 기록

평행이동 하 이산 프레셰 거리(DFDuT, BKN/ESA 2020 기반)에서 **원 배열의 정점 전수 열거를 극대 셀 열거로 교체**하는 것이 실제로 빠른지 검증한 실험 모음.

- 기록일: 2026-08-31 (실행은 본 세션, WSL 시계 기준 2026-08-27~28)
- 핵심 질문: **극대 셀 축소가 DFDuT를 빠르게 하는가?**
- 핵심 불변식: **답이 바뀌면 버그.** 모든 비교에서 답 일치(0 mismatch) 확인.

---

## 1. 실험 환경

### 1.1 호스트 / 툴체인
| 항목 | 값 |
|---|---|
| 호스트 OS | Windows 11 Home 10.0.26200 |
| 리눅스 | WSL2 Ubuntu 24.04 (사용자 `sjp`) |
| 컴파일러 | g++ (Ubuntu 13.3.0) |
| 빌드 | CMake 3.28.3, GNU Make 4.3 |
| CGAL | 5.6 (header-only), GMP/MPFR(system), Boost 1.83.0 |

> 저장소에 원래 들어 있던 결과(`bench_*.out`, `candidate_results.json`)는 **다른 머신**(`/home/sj10132/FRECHET_EVOLVE2`, Linux)에서 생성된 것. 본 세션의 재현 실험은 위 WSL 환경에서 새로 빌드·측정.

### 1.2 빌드 방식
- **out-of-source**: 소스는 `/mnt/c`(Windows)에서 읽고, 산출물은 WSL 네이티브 FS `~/b_{original,candidate,candidate2}`에 기록 (2.4 GB 복사 회피).
- 단일 타겟 `calc_frechet_distance_under_translation`, `RelWithDebInfo`.
- **포팅 수정 (GCC 13)**: `-include cstdint -include array -include cstddef` 강제 포함 (GCC 13이 `<array>` 등을 전이적으로 포함하지 않아 `std::array` 미정의 오류 → `original/` 소스는 편집하지 않고 컴파일러 플래그로 우회). CMakeLists는 이미 포팅됨(`cmake_minimum_required 3.5`, `-std=c++14`).

### 1.3 비교 대상 3종 구현
| 이름 | 산술 | 배열/극대 방식 | 박스 |
|---|---|---|---|
| **original** | CGAL Epeck (정확) | 배열 정점 전수 열거, **극대 필터 없음** (baseline) | 사용 |
| **candidate** | CGAL Epeck (정확) | CGAL 배열 + **BFS 면-깊이 전파** 극대 추출 (`USE_CGAL_DEPTH_MAXIMAL`) | 사용 |
| **candidate2** | **double (float)** | 쌍교점 닫힌형 나열 + **포함 비트마스크** 극대 필터 (hi/lo 밴드 1e-7) | **무시(제거)** |

- `original→candidate` = **극대화 효과** 격리 (둘 다 CGAL).
- `candidate→candidate2` = **부동소수점 효과** 격리 (둘 다 극대 필터).

### 1.4 데이터
- **실측**: Geolife Trajectories 1.3.
  - `geolife_small` 매니페스트: 100쌍, n∈[10,200].
  - `geolife_100` 매니페스트: 100쌍, n∈[100,1000].
- **합성**: 부드러운 랜덤워크 `a` + (소회전 ±0.15rad, 평행이동 ±3, 점별 잡음 σ=0.6)로 만든 `b`. seed 고정.

### 1.5 측정 방법·유의
- 내부 타이머(`MEASUREMENT`): `Arrangement computation of n^6 alg`(배열 셋업), `Fréchet computation of n^6 alg`(판정자) 누적 ms. — E1~E3에 사용.
- 벽시계: bash `$EPOCHREALTIME`로 바이너리 호출만 감쌈. — E4~E5에 사용.
- **WSL 프로세스 생성 오버헤드 ~1.4s/exec**(Defender 스캔 추정) → 데이터를 `~/geodata`로 복사, 보조 프로세스(date/awk/grep) 제거, 청크 실행으로 대응.
- 바이너리별 `timeout 30s` 가드(폭주 방지).

---

## 2. 실험 결과

### E0. 저장소 기존 결과 (다른 머신, 참고)
`fut_lmf`, candidate2 vs original, 벽시계.

| 파일 | 규모 | original | candidate2 | 속도 | 불일치 |
|---|---|--:|--:|--:|--:|
| `bench_100.out` | 합성 100쌍 n∈[100,1000] | 62,992 ms | 50,834 ms | 1.24× | 0 |
| `bench_1000.out` | 합성 1000쌍 | 596,490 ms | 492,203 ms | 1.21× | 0 |
| `bench_geolife_100.out` | Geolife 100쌍 | 60,330 ms | 44,378 ms | 1.36× | 0 |
| `bench_n_sweep.out` | n=100~1000 | — | — | 2.45×→1.06× | OK |
| `be2000.out` | n=2000, **candidate**(cand2 아님) | 51,302 ms | 13,989 ms | 3.67× | (1쌍 0.07× 손실) |

- 관찰: 버킷별 속도향상이 **n이 커질수록 감소**(1.9×→1.14×).
- 데이터 정합성 문제: `candidate/`, `candidate2/`, `candidate3/`의 `candidate_results.json`이 **바이트 단위 동일**(구현별 재측정이 아니라 복사됨).

### E1. 3-way 분해 (내부 타이머) — `_bench_3way.out`
geolife_small 100쌍, `fut_lmf`, 배열+판정자 누적 ms.

| 구성요소 | original (CGAL,no-max) | candidate (CGAL,max) | candidate2 (float,max) |
|---|--:|--:|--:|
| Arrangement (배열) | 5,592.2 | 11,798.0 | 350.5 |
| Fréchet (판정자) | 1,905.8 | 260.9 | 857.9 |
| **TOTAL** | **7,498.0** | **12,058.9** | **1,208.4** |

**격리 효과:**
- MAXIMAL (orig→candidate): 총 **0.62×** (손해). 판정자 7.30×↓, 배열 2.1×↑.
- FLOAT (candidate→candidate2): 총 **9.98×**. 배열 **33.66×**.
- COMBINED (orig→candidate2): **6.20×**.
- 결론: 극대화를 CGAL로 하면 순손해. 이득의 원천은 float.

### E2. 전역 n6 n-sweep — `_nsweep.out`
합성 곡선, 전역 `n6` 경로(원판 N=n²), original vs candidate(둘 다 CGAL), 내부 타이머. n=6→28.

| n | arrX(배열) | freX(판정자) | **totX(총합)** |
|--:|--:|--:|--:|
| 6 | 0.55–0.64 | 5.0–5.3× | 0.57–0.66 |
| 10 | 0.58–0.59 | 15.8–19.3× | 0.60–0.63 |
| 12 | 0.66–0.70 | 25–30× | 0.69–0.74 |
| 16 | 0.43 | 16–115× | 0.45–0.48 |
| 20 | 0.35–0.36 | 35–39× | 0.38 |
| 24 | 0.35–0.36 | 33–46× | 0.38–0.39 |
| 28 | 0.31 | 47× | **0.32** |

- freX(판정자 절감)는 n과 함께 **증가**, arrX(배열)은 **악화**, totX는 **단조 감소 → 0.32로 수렴**. **break-even 없음.**
- fre 비중 ~5–8% (배열 셋업 지배). n=28 p1에서 candidate 타임아웃.
- 상한 논증: 판정자가 총시간의 ~6%인 한, 극대 계산이 공짜여도 이득 상한 ≈ 6%.

### E3. candidate2 계측 (K/m·축퇴·C_q) — `_c2_stats.out`
candidate2에 계측 3줄 추가(`degen`, `npts`, `queries`) 후 측정.

| 지표 | 주 경로 `fut_lmf` (N≤12, 40 geolife쌍) | 전역 `n6` (N=mn, n=8/12/16) |
|---|--:|--:|
| m/호출 (dedup) | ~98 | ~6,300 |
| K/호출 (생존) | ~26 | ~3,150 |
| **K/m** | **0.264** | **0.500** |
| **축퇴율 (mask_hi≠mask_lo)** | **0.898** | **0.976** |
| **C_q (질의당)** | **3,904 ns** | **259 ns** |
| 셋업 합 (gen/mask/filter) | 116 ms (5.1/74.5/26.5) | 2,169 ms (3.1/291.8/**1,861.4**) |
| 판정자 합 | **316 ms** | 38 ms |
| 지배 요소 | **판정자 73%** | **셋업 98%** |

- 두 경로가 정반대: 주 경로는 판정자 지배(긴 곡선 → C_q 3.9µs), 전역은 셋업 지배(필터 O(N²K) 폭발).
- 축퇴 90%/98% — 근-축퇴가 기본값. 주 경로에선 필터가 여전히 3.8× 축소.

### E4. end-to-end 벽시계 3-way (합성) — `_wall3.out`
`fut_lmf` **전체 실행시간**, 합성 n=100/200/400/800 각 3쌍, 2회 중 최소.

| n | O (기존) | C1 (극대,CGAL) | C2 (극대+float) | O/C1 | O/C2 | 불일치 |
|--:|--:|--:|--:|--:|--:|:--:|
| 100 | 293 | 453 | 140 | 0.65× | 2.09× | 0/0 |
| 200 | 440 | 425 | 267 | 1.04× | 1.65× | 0/0 |
| 400 | 652 | 888 | 493 | 0.73× | 1.32× | 0/0 |
| 800 | 2,436 | 2,396 | 2,055 | 1.02× | 1.19× | 0/0 |
| **합계** | **3,821** | **4,162** | **2,955** | **0.92×** | **1.29×** | 0/0 |

- **극대만 추가(O/C1) = 0.92×** — end-to-end로는 wash/약간 손해.
- 분해: 극대 ×0.92 × float ×1.41 = 1.29×. **순이득은 전부 float.**

### E5. end-to-end 벽시계 3-way (실 Geolife) — `_wall_geo.raw`
`fut_lmf` 전체 실행시간, `geolife_100` 매니페스트, REPS=1.

| | O (기존) | C1 (극대,CGAL) | C2 (극대+float) |
|---|--:|--:|--:|
| 합계 (44 clean쌍) | 25,101 ms | 24,312 ms | 15,854 ms |
| **비율** | — | **O/C1 = 1.03×** | **O/C2 = 1.58×** |
| 불일치 | — | 0 | 0 |

- 표본 수렴: 10쌍 1.234 → 24쌍 1.066 → 44쌍 **1.032** (10쌍의 1.23은 노이즈).
- **병리적 쌍 1개**: `10167.txt+4547.txt` — candidate가 **30초 타임아웃(ERR124)**, original 543ms, candidate2 408ms.
- 결론: 극대만 추가는 실데이터에서도 **wash(1.03×)**, 합성(0.92×)과 일치. 이득은 float(1.58×).

### E6. 병리적 쌍 진단 — `_pair_diag.txt`
`10167.txt + 4547.txt` 세 구현 비교.

| | 배열 빌드 수 | 빌드당 극대 후보 | 벽시계 | 답 |
|---|--:|--:|--:|:--:|
| original | ~107 | (전정점) | 1초 | 0.0538022643… ✓ |
| candidate2 (float) | 81 | 20–40 | <1초 | 0.0538022643… ✓ |
| candidate (CGAL) | **205+ (안 끝남)** | **거의 전부 1** (187/205) | **>150초 timeout** | 미완 |

- 이 쌍의 축퇴: 곡선에 정지 구간(4547.txt는 한 점 ×9회), candidate2 계측 degen = **89.6%**.
- 진단: candidate의 CGAL 깊이-BFS 극대가 **극대 셀을 과소반환(1개)** → 분기한정이 대표점 부족으로 수렴 못 함 → 축퇴된 원 위 CGAL exact 배열을 **무한히 재구성**(205+, ~0.7초/빌드) → timeout. **속도만이 아니라 강건성·정확성 실패.**
- candidate2(float)는 동일 입력을 81빌드·<1초·정답으로 처리(축퇴 면역).

---

## 3. 정확성 규약
- 값 계산은 설계상 ε-근사(기본 ε=1e-7). 결정 문제는 한쪽으로 ~9e-9 슬랙(`epsilon_slack = epsilon_sub − epsilon_sub/10`), 진짜 yes를 no로 만들지 않음.
- 비교 기준은 "동일 yes/no"가 아니라 **답 차이 < 1e-6**(no→yes 전환만 허용). 본 세션 모든 실험 **0 mismatch**.

---

## 4. 결론 요약
1. **극대 셀 축소만 추가하면 end-to-end는 wash** — 합성 O/C1=0.92×, 실 Geolife O/C1=1.03×. 표본 늘릴수록 1.0으로 수렴.
2. **실제 end-to-end 이득의 원천은 float 재작성** — O/C2 ≈ 1.3~1.6×. (내부 배열 셋업 33×↓)
3. **극대화의 이득은 "내부 arr+판정자 루프"에 한정**되며(E3: 주 경로 판정자 73% 지배 → 판정자 절감이 거기선 유효), 그 루프가 `fut_lmf` 총시간의 소수라 end-to-end로는 희석됨.
4. **CGAL 깊이-BFS 극대(candidate)는 실데이터 축퇴에서 폭주/미수렴** — 폐기 권장. 극대화를 계속한다면 **반드시 float 경로 위에서**.
5. 근-축퇴(90%+)가 실데이터의 기본값 — 밴드 정확성(`band = max(r²·1e-7, k·r·ε_coord)`) 보정은 정확성상 필요.

---

## 5. 미해결 / 다음 단계 (해석·미실험)
- **Epick 대조 (미실험)**: BKN은 `Epeck`(정확 술어+정확 구성) 사용. `Epick`(정확 술어+double 구성)으로 바꾸면 강건성 유지한 채 상당 속도를 얻을 가능성 — 우리 "float 이득"이 우리 것인지 커널 것인지 판별 필요.
- **밴드 정확성 논증**: 현재 mismatch 0은 실증일 뿐 보장 아님.
- **극대화가 이기는 점근 체제**: 판정자가 배열을 추월하는 큰 배열/무제한 경로(`depth_limit=40`) 탐색.
- 원 논문(arXiv:2008.07510)이 산술 선택(exact vs float)을 명시적으로 논하는지 미확인.

---

## 부록. 생성/실행 스크립트
| 스크립트 | 역할 |
|---|---|
| `_build_one.sh` | 단일 타겟 out-of-source 빌드 (GCC13 force-include) |
| `_bench_3way_wsl.sh` | E1 3-way 분해 |
| `_gen_nsweep.py` / `_bench_nsweep.sh` | E2 전역 n6 스윕 |
| `_c2_stats.sh` | E3 candidate2 계측 (K/m·축퇴·C_q) |
| `_gen_wall.py` / `_bench_wall3.sh` | E4 합성 벽시계 |
| `_geo_chunk.sh` / `_cp_manifest_data.sh` | E5 실 Geolife 벽시계 (재개형 청크) |
| `_diag_pair.sh` | E6 병리적 쌍 진단 |

*(candidate2 소스에 계측 3줄 `// INSTRUMENTATION` 추가 상태 — 되돌리기 대기 중)*

## 6. 논문 데이터 재현 실험 — `paper_bench/` (2026-09-16 ~ 09-21)

이 브랜치에는 초록이 인용하는 6.6~6.8만 담았다. 거울 데이터(characters/, sigspatial_subset/)와 임의 1,000쌍 실험(6.1~6.5, 그림 스크립트)은 전체 브랜치 `claude/loving-ramanujan-ge8xd7`에 있다. 절 번호는 전체 브랜치와 같게 두어 표·결과 파일의 참조(§6.6, §6.8)가 그대로 맞는다.

### 6.6 저자와 동일한 데이터: UCI 원본 파일 + 저자 인스턴스 파일 — `paper_bench/run_uci.sh`

사용자가 UCI 원본 `mixoutALL_shifted.mat`을 제공 → `paper_data/convert_characters_uci.py`
(저자 `character_converter.m` 그대로) → `paper_data/characters_uci` (2,858곡선, 평균 120.99 정점).
**동일성 검증**: 저자 글자별 인덱스 목록 2,858/2,858 일치; 저자 δ\* 검증 파일 2,000쌍을 original로
재계산해 최대 오차 1.3×10⁻⁸ (`results/VERIFY_uci.md`). 따라서 저자의 질의 파일이 그대로 적용된다.
측정은 저자 하네스처럼 **인스턴스당 1회**, 단일 코어(LMF 코어 2, 결정 문제 코어 3 병렬).

**LMF** (`characters_full_*` 210파일 = 21,000쌍, 논문 Table 4의 집합):

| | original | candidate5_noslack | candidate5 (slack) | 논문 Table 4 (original) |
|---|--:|--:|--:|--:|
| ms/인스턴스 (21,000) | 100.60 | 24.06 (**4.18×**, geomean 3.84 [3.81, 3.87]) | 18.94 (5.31×) | 140.0 |
| 블랙박스 호출/인스턴스 | 12,246 | 3,144 | 2,600 | 12,387 |
| construction 비중 | 59.5 % | 6.7 % | 6.2 % | 52.3 % |
| 값 불일치 (>1e-7) | — | **0 / 21,000** | 1 / 21,000 (2.4×10⁻⁶ 큼, slack 누락) | |
| 같은 글자 2,000쌍 ms | 77.47 | 18.79 (4.12×) | 13.72 (5.65×) | |

**결정 문제** (저자 1,000쌍 × 23세트, 인자 (1 ± 4^ℓ)):

| 데이터셋 | original ms | candidate5 ms | 가속 | 호출/인스턴스 | 논문 Table 2 | 오답/불일치 |
|---|--:|--:|--:|---|---|--:|
| all-characters | 16.77 | 7.46 | **2.25×** | 1,616 → 386 | 27.3 ms, 1,860 | 0 / 0 |
| same-characters | 11.74 | 6.35 | **1.85×** | 997 → 247 | 18.7 ms, 1,159 | 0 / 0 |

- 저장소에 들어 있는 저자 질의 파일은 **2^ℓ** 인자(`paperq`)다. 그대로 돌리면 original 0.51 / 0.93 ms,
  호출 61 / 69로 논문 Table 2의 1/50 → 논문 수치는 본문대로 4^ℓ 인스턴스에서 나온 것. 표·그림은 4^ℓ(`paperq4`).
- 저자 all-characters 목록에 중복 쌍 2개(712–2493, 432–266)가 있어 행 인덱스로 대응시켰다.
- 결론은 무작위 1,000쌍(6.2~6.5)과 같다: LMF 4.2× (이전 4.0×), 결정 2.25× / 1.85× (이전 2.22× / 1.91×).
- Sigspatial 전체 세트(`shortest-sf.tgz`)는 원 서버가 내려가 보류.
- 원시 결과: `results/raw_characters_uci_{lmf,decider}.tar.gz`, 요약: `results/RESULTS_uci.md`,
  그림: `figures/fig_breakdown_uci.*`, `figures/fig_decider_bars_uci.*`.

### 6.7 술어의 정확 모드(MAXREGION_EXACT=1)와 재측정 — `candidate5/lib/cgal_disk_arrangements/maximal_regions.cpp`

초록 2.3의 "부호가 확정되지 않을 때만 유리수 재계산" 서술이 candidate4의 하이브리드 술어에만 있고
candidate5(표의 결과를 낸 파이프라인)에는 없던 문제를 해결. P2(두 원판)·P3(세 원판) 판정을
배정밀도로 계산하고 비교식의 반올림 오차 상한(단위 반올림 u = 2⁻⁵³ 기준, 보수적 상수)이 부호를
확정하지 못할 때만 boost `cpp_rational`로 정확 반지름에서 재계산. band는 0으로 취급.
경계 사례 테스트(`test_exact_predicates.cpp`, 결정 경계 ±3 ulp의 무작위 삼중·쌍 각 40만 개):
유리수로 넘어간 799,978건, 확정했으나 틀린 건 0.

컨테이너가 재시작되어(같은 CPU 모델명, 속도는 약 35 % 느림) original / candidate5_noslack /
candidate5_exact 세 arm을 같은 인스턴스에서 코어 2 단독으로 연속 재측정(`_r2`):

| arm | ms/인스턴스 | 총 시간 | 호출/인스턴스 | 불일치 | vs original (합 / geomean / 빠른 쌍) |
|---|--:|--:|--:|--:|---|
| original | 135.35 | 2,842.3 s | 12,245.8 | — | — |
| candidate5_noslack | 30.75 | 645.8 s | 3,143.6 | 0/21,000 | 4.40× / 4.07× / 20,853 |
| candidate5_exact | 30.66 | 643.9 s | 3,146.1 | 0/21,000 (최대 8.8e-9) | 4.41× / 4.07× / 20,904 |

- 필터 비용: exact / noslack 시간비 1.003 (측정 오차 이내).
- 재계산 횟수: P2 0 / 197,226,956, P3 74 / 289,116,753 (r0·r2 두 실행에서 동일, 결정적).
- 단계별(exact): 전처리 82,581 / Lipschitz 호출 282,761 / 배열 추정 162,130 / 배열 알고리즘 77,596
  (구성 42,777 + 호출 33,978) ms; 기존 87,237 / 289,267 / 190,210 / 2,233,926 (1,708,959 + 367,421).
- 표·그림(`TABLES_uci.md` 표 B·C, `fig_breakdown_uci`)은 r2·exact로 교체. 결정 문제(표 A)는 이전
  인스턴스의 r1 값 그대로이며 절대 시간은 표 간 비교 불가.
- 원시: `results/raw_characters_uci_lmf_r2.tar.gz`(r2 세 arm + exact r0), 요약 `results/RESULTS_uci_r2.md`.

### 6.8 블랙박스 호출 수는 부동소수점 경로에 따라 달라진다 — 논문 Table 4와의 1.2 % 차이

같은 `original` 코드·같은 UCI 인스턴스에서 컴파일 옵션만 바꾼 두 바이너리
(RelWithDebInfo `-O2` = 본 실험 기본, Release `-O3 -march=native`)로 LMF 목록의 처음 300쌍(유효 299)을
각각 코어 1·2에서 실행:

| | -O2 (기본) | -O3 -march=native |
|---|--:|--:|
| 블랙박스 호출 합 (299쌍) | 3,414,459 | 3,417,802 (+0.10 %) |
| 호출 수가 다른 쌍 | 46 / 299 | |
| 한 쌍에서의 최대 변동 | 9,193 → 11,798 (+28 %, `1465–8`) | |
| 거리 값 최대 차이 | 1.4×10⁻¹⁴ | |

- 원인: 분기한정 탐색의 가지치기·상자 분할 판정이 배정밀도 비교(`std::pow`, FMA 축약 여부 등)에
  의존하므로, 경계에 걸린 판정이 뒤집히면 이후 상자 트리와 배열 구성 횟수가 달라진다. 답은 ε=10⁻⁷
  보증 안에서 같다(값 차이 10⁻¹⁴).
- 따라서 호출 수는 알고리즘의 결정적 함수가 아니라 실행 환경(컴파일러·플래그·CPU·CGAL 버전)에 따라
  달라지는 통계량이고, 논문 Table 4의 12,387.1 대 본 실험 12,245.8(−1.1 %)은 이 변동 범위로 설명된다.
  저자 코드에는 "TODO: change the default arrangement parameters … after experiments" 주석이 있어
  논문 실행 당시 γ 기본값이 달랐을 가능성도 남는다(논문 본문은 γ 값을 밝히지 않음).
- 원시: `results/raw_characters_uci_fpsens.tar.gz` (쌍 목록 + 두 CSV).

### 6.9 Sigspatial 전체 집합(20,199곡선)에서의 재현 — `paper_bench/run_sig.sh` (2026-09-24, 사용자 PC WSL)

**데이터**: 사용자가 확보한 저자의 `shortest-sf.tgz`(54.6 MB, SHA-256 `33d3262ef389e543…`, `paper_data/sigspatial/`에 커밋)를
`paper_data/convert_sigspatial.py`(저자 `fetch_and_convert_data.py`의 `tail -n +2`와 동일)로 변환 → 20,199곡선, 평균 247.9정점
(논문 Table 1: 247.8). 저자의 결정 문제 파일 `sigspatial_fut_decider_*`(1,000쌍 × 23세트, 2^ℓ)를 그대로 복사해 `queries/sigspatial_paperq_*`로 쓰고,
4^ℓ 세트(`sigspatial_paperq4_*`)는 저자의 δ* 파일에서 `paper_bench gen`과 같은 식으로 생성했다(gen 자체는 original의 δ* 재계산이 아래의
OOM으로 죽어 쓸 수 없었음). **동일성**: 세 arm 모두 1,000쌍(original은 998쌍)의 계산값이 저자 δ*와 최대 1.33×10⁻⁸ 이내로 일치.

**환경**: Windows 11 호스트(15.5 GB) 위 WSL2 Ubuntu 22.04, g++ 11.4, CGAL 5.4, 18코어. 기본 WSL 메모리 한도 7.5 GB.
저자 하네스처럼 인스턴스당 1회 측정, arm마다 코어 하나에 고정(`taskset`). 결정 문제의 두 arm과 LMF는 서로 다른 코어에서 동시에 돌렸다.
하네스에 행 단위 `csv.flush()`를 추가했다(프로세스가 나중 쌍에서 죽어도 앞 행이 남도록; 측정에는 영향 없음).

**LMF (값 계산, 저자 결정 문제의 1,000쌍)** — proposed = candidate5 `MAXREGION_EXACT=1 MAXREGION_SLACK=0`, 998쌍 공통:

| arm | ms/인스턴스 | 총 시간 | 호출/인스턴스 | Construction(열거) | 불일치 | vs original (합 / geomean / 중앙값) |
|---|--:|--:|--:|--:|--:|---|
| original | 2,323.0 | 2,318 s | 12,579 | 2,230,595 ms (96.2 %) | — | — |
| candidate5_exact | 70.7 | 70.5 s | 3,569 | 2,502 ms (3.5 %) | 0/998 (최대 8.6e-9) | **32.9× / 2.37× [2.26, 2.49] / 2.27×** |
| candidate5_noslack | 55.4 | 55.3 s | 2,841 | 1,646 ms (3.0 %) | 0/998 | 41.9× / 3.04× / 2.49× |

- 합의 32.9×는 소수의 병적 쌍이 만든다. original이 60 s를 넘는 쌍 5개(최대 795 s, 모두 Construction 97~99.8 %),
  10 s 초과 8개, 1 s 초과 18개. 같은 쌍을 candidate5는 1.1~1.7 s에 끝냈다. 가장 느린 10쌍을 빼면 146.4 → 60.4 ms(2.42×),
  중앙값 71.0 → 30.0 ms(2.37×). Characters(4.41×)보다 전형적 이득은 작고 꼬리 이득은 훨씬 크다.
- **original이 메모리로 죽는 쌍 2개**: 125번(`file-003586`/`file-002157`)과 432번(`file-002502`/`file-016674`)에서 RSS 7.4 GB에 이르러
  WSL OOM killer에 죽었다(120 s, 193 s 시점). candidate5는 두 쌍을 각각 1 s 안팎에 처리했다. 위 표는 이 2쌍을 제외한 998쌍이다.
  **OOM 쌍 재측정**: WSL 한도를 12 GB로 올려 다시 돌려도 두 쌍 모두 RSS 11.8 GB에서 죽었다(125번 312 s, 432번 203 s 시점).
  즉 original은 이 두 쌍에 12 GB 이상이 필요하고, candidate5(exact)는 각각 0.59 s / 1.97 s, 값은 저자 δ*와 7.5e-10 / 3.6e-9 이내로 일치한다.
  쌍별 시간·최대 RSS는 `raw_sigspatial_lmf.tar.gz`의 `sigspatial_lmf_original_perpair_status.txt`에 있다.
- exact arm이 noslack보다 느리고 호출이 많다(70.7 vs 55.4 ms, 3,569 vs 2,841회). 정확 모드는 band=0이라 접하는 원판이 별도 극대
  집합으로 갈라지고, 유리수 재계산도 Characters(74회)보다 훨씬 잦다(P2 4,765 / 9.39M, P3 9,789 / 13.7M; 좌표가 EPSG:3857 미터 단위
  ~1.4×10⁷이라 필터의 오차 상한이 커짐). 정확성은 두 arm 모두 유지된다.
- 단계별(998쌍 합, ms): original 전처리 20,176 / Lipschitz 호출 16,289 / 배열 추정 27,722 / 배열 알고리즘 2,251,388(구성 2,230,595 + 호출 15,283);
  exact 19,164 / 18,273 / 25,881 / 4,234(열거 2,502 + 호출 1,622).

**결정 문제 (저자 23세트 × 1,000쌍)** — 두 arm 모두 오답 0, 서로 불일치 0:

| 세트 | original ms/질의 | candidate5 ms/질의 | 가속 | 호출/질의 | original 구성 비중 |
|---|--:|--:|--:|---|--:|
| 2^ℓ (저자 파일) | 0.369 | 0.393 | 0.94× | 24.2 → 22.8 | 3.8 % |
| 4^ℓ (논문 본문) | 41.74 | 34.51 | 1.21× | 1,146 → 293 | 14.3 % |

- 결정 문제에서는 이득이 작다. Sigspatial 결정 질의는 original도 배열 구성이 4^ℓ에서 14 %, 2^ℓ에서 4 %뿐이고 나머지가 Lipschitz 탐색이라,
  배열 단계를 없애도 줄일 몫이 그만큼이다(4^ℓ에서 구성 137 s → 4.6 s, 호출 35 s → 3.4 s; 나머지 788 s vs 786 s).
  2^ℓ 세트는 질의당 0.4 ms라 후보 열거의 고정 비용이 드러나 6 % 느리다. 가장 어려운 세트(ℓ=−10 minus)는 331.6 → 261.4 ms(1.27×).
- 논문 Table 2의 Sigspatial 값과 직접 비교는 하지 않았다(2^ℓ/4^ℓ 문제는 §6.6과 같음).

**결론**: Characters와 달리 Sigspatial에서는 original의 배열 구성이 값 계산 시간의 96 %를 차지하고 일부 쌍에서 수백 초·7 GB 이상을 쓴다.
제안 방법은 같은 답을 내면서 모든 쌍을 2 s 이내·수십 MB에 끝낸다. 초록의 Characters 수치(4.41×)에 더해 "Sigspatial 1,000쌍에서 합 32.9×,
중앙값 2.4×, original이 메모리 부족으로 실패한 2쌍 포함 전부 성공"이 본문에 넣을 수 있는 결과다.
원시: `results/raw_sigspatial_lmf.tar.gz`, `results/raw_sigspatial_decider.tar.gz`; 요약 `results/RESULTS_sig.md`; 표 `results/TABLES_uci.md` 표 D.
