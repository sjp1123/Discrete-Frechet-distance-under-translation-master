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
