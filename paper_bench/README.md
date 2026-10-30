# paper_bench — 논문 데이터 세트에서 `original` 대 `candidate5`

프로세스 내 드라이버(`paper_bench.cpp`)를 **방식마다 한 번씩, 그 방식 자신의 소스로** 컴파일한다.
`CMakeLists.txt`가 방식을 하위 디렉터리로 끌어오고, 그 방식의 `trans` 객체 + `disc_arrangement_traversal`을
링크한다. 곧 그 방식의 `fut_paper_experiments`와 코드 경로와 플래그가 같다. 실행시간 측정은 논문 하니스를
따른다. `FrechetUnderTranslation` 생성 + 호출을 시계로 감싸고, 기본 매개변수(ε = 1e-7, depth 40,
cut limit 12)를 쓰며, `MEASUREMENT` 카운터/타이머를 질의마다 읽는다.

**시계.** 하니스와 두 방식의 `lib/measurement_tool/measurement_tool.h`는 `std::chrono::steady_clock`(단조)을
쓴다. 저자 코드는 `high_resolution_clock`을 썼는데, libstdc++는 이를 `system_clock`으로 정의한다. WSL2에서는
Hyper-V 시간 동기화 때 이 시계가 수백 ms에서 ~1 s까지 건너뛴다. 이 때문에 첫 WSL 측정들이 망가졌다
(음수 인스턴스 시간, `experiment_log` 6.12). 시계 변경은 측정만 건드리고 알고리즘은 건드리지 않는다.
컨테이너 측정은 이 변경 전에 했으며, 331,600행에서 시계 이상이 없다: `raw_characters_uci_lmf.tar.gz`,
`raw_characters_uci_lmf_r2.tar.gz`, `raw_characters_uci_decider.tar.gz`, `raw_characters_uci_fpsens.tar.gz`.
(`raw_characters_uci_decider_r2.tar.gz`는 실행시간 측정이 무효인 WSL 측정이다.
`raw_characters_uci_decider_r3.tar.gz`는 그것을 유효하게 다시 잰 WSL 측정이다.)

> **정정 (2026-10-21):** `original/` 자체가 저자 코드를 수정한 사본이다(기저 사례에서 탐색 상자를 뺀다). [../BASELINE_NOTICE.md](../BASELINE_NOTICE.md)와 [authors_check/REPORT.md](authors_check/REPORT.md) §2–3을 보라.

```
bash paper_bench/build.sh              # -> ~/b_pb_original/paper_bench, ~/b_pb_candidate5/paper_bench
bash paper_bench/run_uci.sh lmf        # the authors' 21,000 characters_full pairs, one measurement each
bash paper_bench/run_uci.sh decider    # the authors' 23 x 1,000 decider instances (2^l files and 4^l sets)
python3 paper_bench/analyze_uci.py     # -> results/RESULTS_uci.md; tables in results/TABLES_uci.md

# on a WSL2 / hybrid-core host: both arms back-to-back on one vCPU (r3); binaries from build.sh (or set ORIG=, C5=)
bash paper_bench/run_wsl_paired.sh lmf_sig <stream> <nstreams> <cpu>          # Sigspatial value computation, per pair
bash paper_bench/run_wsl_paired.sh decider <prefix> <data_dir> <cpu>          # 23 x 1,000 decider instances, per file
python3 paper_bench/merge_wsl_r3.py <work_dir>                               # -> results/raw_*_r3.tar.gz (re-runnable; stops rather than drop rows)
DATASET=sigspatial LMF_REP=r3 DEC_REP=r3 python3 paper_bench/analyze_uci.py  # -> results/RESULTS_sig.md
python3 paper_bench/make_tables_wsl.py     # -> TABLES_uci.md Table D (Sigspatial LMF) and Table E (decider, 3 benchmarks)
python3 paper_bench/make_repro_check.py    # -> results/REPRO_check.md (original vs the authors' outputs and the paper)
python3 paper_bench/figures/plot_scatter.py  # -> figures/fig_scatter*.{pdf,png} (the paper's Figure 6 format)
```

`run_sig.sh`는 첫 번째(r1) Sigspatial 실행 스크립트다. 그 실행시간 측정은 r3로 대체되었다.

이 브랜치에는 초록과 논문 초안이 인용하는 측정만 있다. 저자들의 Characters 인스턴스(`paper_data/characters_uci`,
`queries/characters_uci_*`, `results/*_uci_*`)와 Sigspatial 전체 세트(`paper_data/sigspatial`,
`queries/sigspatial_*`, `results/*sigspatial*`)다. 거울 순서(mirror-order) 데이터 세트와 무작위 쌍 측정은
전체(full) 브랜치에 있다.

## 측정하는 것

* **값 계산 (LMF)** — 논문의 주 알고리즘 `calcDistance2`. 쌍마다 시간, 블랙박스 호출,
  n6 배열/판정자 타이머, phase-2 타이머를 잰다.
* **결정 문제** — 논문의 판정자 벤치마크 `lessThan(δ)`. 벤치마크마다 질의 세트가 23개다.
  δ = (δ\*−ε)(1−b^l), l=−10…−1(답은 *no*여야 한다)과 δ = (δ\*+ε)(1+b^l),
  l=−10…2(답은 *yes*여야 한다)를 쓴다. 저자들이 배포한 파일(`paperq`)은 b = 2, 논문 본문(`paperq4`)은
  b = 4다. 시간, 블랙박스 호출, phase-1 단계 타이머를 잰다.

## 데이터 세트 (`../paper_data/README.md` 참조)

| 세트 | 곡선 | 쌍 | 비고 |
|---|--:|--:|---|
| `characters_uci` (LMF) | 2 858 | 21 000 | 저자들의 `characters_full_<s1>_<s2>.txt`, 210 파일 x 100 (논문 표 4) |
| `characters_uci_all` / `_same` (판정자) | 2 858 | 각 1 000 | 저자들의 판정자 쌍 파일, 각 23세트 |
| `sigspatial` (판정자, LMF) | 20 199 | 1 000 | 저자들의 `sigspatial_fut_decider_*` 파일 (GIS Cup 전체 세트, `paper_data/sigspatial`) |

## 정확성 관문

* LMF: 모든 쌍에서 |value(original) − value(candidate5)| ≤ ε = 1e-7 (이 소스 트리의 관례다. 둘 다 같은
  양의 ε-근사이므로 비트 단위 일치는 기대하지 않는다).
* 판정자: **두** 방식 모두 모든 답이 기대한 답과 같고, 두 방식의 답이 서로 일치한다.

## 파일

* `queries/<set>_{paperq,paperq4}_<l>_<plus|minus>.txt` — 판정자 세트. `*_computed_distances.check` — 쌍별 δ*.
* `results/raw_*.tar.gz` — 방식별, 인스턴스별 행(`<set>_..._<arm>_<rep>.csv`). 컨테이너: Characters LMF r1과 r2
  (r2 = 연달아 다시 잰 측정, 표 C와 초록에 쓰임), Characters 판정자 r1. WSL(사용자 PC): Sigspatial r1과
  판정자 r2는 실행시간 측정이 무효다(시계 건너뜀). r3는 짝지은 재측정이며 유일하게 유효한 WSL 실행시간 측정이다.
  `raw_sigspatial_lmf_r3.tar.gz`에는 `sigspatial_lmf_rss_r3.txt`(최대 RSS)와
  `sigspatial_lmf_original_oom_12gb.txt`(12 GB를 넘는 두 쌍에서 기준선이 강제 종료된 시각; 정정(2026-10-21):
  수정된 `original/`만의 실패이며, 저자 코드는 두 쌍 모두 5 GB 안에서 끝낸다, [authors_check/REPORT.md](authors_check/REPORT.md) §4.2)도 들어 있다.
* `results/TABLES_uci.md` — 논문 형식의 표(A-C 컨테이너, D-E WSL r3). `RESULTS_uci.md`, `RESULTS_uci_r2.md`,
  `RESULTS_sig.md` — 세트별 요약. `REPRO_check.md`, `VERIFY_uci.md` — 재현 검사와 데이터 동일성 검사.

## candidate7 (감사 수정을 넣은 candidate6; `../candidate7/README.candidate7.md`)

```
ARMS="original candidate5 candidate7" bash paper_bench/build.sh           # ~/b_pb_<arm>
python3 paper_bench/plan_wsl_c7.py <plan_dir> 8                           # jobs + a cost-balanced split over 8 streams
bash paper_bench/run_wsl_c7.sh <plan_dir> ~/wsl_c7                         # 3 arms back-to-back per job on one pinned vCPU
(cd ~/wsl_c7 && tar czf <repo>/paper_bench/results/raw_c7_timing.tar.gz lmf_chars lmf_sig decider rss rc.txt log.txt)
python3 paper_bench/analyze_c7.py                                          # -> results/C7_timing.md
bash paper_bench/c7_verify/verify.sh                                       # verification -> ~/c7_verify_out (summarize.sh); written up in results/C7_report.md
```

* `results/C7_report.md` — candidate7에 대한 감사의 재현 사례, 정확한 기준값 퍼징, 임계값 근처 실제 데이터.
* `results/C7_timing.md` — Characters LMF 전체(21,000), Sigspatial LMF(1,000), paperq4 판정자 세트(3 × 23,000)의
  모든 답과 값, 그리고 짝지은 실행시간 측정(original, candidate7, candidate5). 원시 행은 `raw_c7_timing.tar.gz`에 있다.
* `results/PAPER_tables_c7.md` — `C7_timing.md`로 만든 본 논문의 표(한국어 캡션).
