# paper_data — "When Lipschitz Walks Your Dog" (BKN, ESA 2020)의 데이터셋

`paper_bench/`의 `original` 대 `candidate5` 비교에 쓰는 곡선 파일이다. 형식은 이 트리 고유의 것이다(한 줄에 `x y` 하나, 연속으로 중복된 행은 `parser.cpp`가 버린다).
논문(arXiv:2008.07510, §5)은 **Characters**(UCI Character Trajectories, 곡선 2 858개, 평균 꼭짓점 120.9개), **Sigspatial**(ACM GIS Cup
2017 표본, 곡선 20 199개, 평균 꼭짓점 247.8개), Geolife로 벤치마크한다.

저자의 내려받기/변환 스크립트는 저자 저장소(`gitlab.com/anusser/frechet_distance_under_translation`, `test_data/benchmark/`)에 있다.
`fetch_and_convert_data.py`가 UCI에서 `mixoutALL_shifted.mat`를, martinwerner.de에서 `shortest-sf.tgz`를 내려받아 변환한다.
이 디렉터리를 만든 세션은 **archive.ics.uci.edu, martinwerner.de, sigspatial.org, MPI/TUM 미러, archive.org, HuggingFace, Zenodo, Kaggle 어디에도 네트워크로 접근할 수 없었다**(egress 정책).
그래서 데이터를 아래에 적은 대로 공개 GitHub/GitLab 미러에서 모았다. 이 데이터는 나중에 대체됐다. `characters_uci/`는 사용자가 제공한 UCI 파일에서 왔고, `sigspatial/shortest-sf.tgz`는 따로 구해 커밋했다.
여기 있는 모든 것은 `convert_characters_uci.py`(characters_uci/), `convert_sigspatial.py`(sigspatial/), 그리고 기록해 둔 출처 커밋 / 체크섬으로 재현할 수 있다.

## characters/ — 곡선 전부(2 858개) — 이 브랜치에는 들어 있지 않음 (미러 순서 사본; 전체 브랜치 참고)

| 항목 | 값 |
|---|---|
| 출처 | `github.com/ShuningZhao/Character-Trajectories`의 `trajectories_train.mat`(곡선 1 429개 + 레이블)와 `trajectories_xtest.mat`(곡선 1 429개, 레이블 없음). SHA-256은 `characters/SOURCE_SHA256.txt`에 있다 |
| UCI `mixoutALL_shifted.mat`와 같은 데이터라는 근거 | 표본 2 858개, 3 × T 셀(x 속도, y 속도, 펜 압력), T ∈ [109, 205]. UCI 문서에 적힌 범위와 같다. 글자 클래스 20개이고 키 순서도 같다(a b c d e g h l m n o p q r s u v w y z) |
| 변환 | `convert_characters.py`. 저자의 `character_converter.m`을 한 줄씩 옮겼다. 압력 행을 버리고, `B = tril(ones(l,l),-1)·A`(속도의 배타적 누적합, 첫 꼭짓점은 원점)를 계산해 표본마다 `x y` 행 하나를 쓴다 |
| 결과 | 중복 제거 후 곡선당 평균 **꼭짓점 120.99개**(최소 61, 최대 183) — 논문 값은 120.9 |
| 번호 | `data/1.txt … 1429.txt` = 미러의 train 세트(글자는 `labels_train.txt`), `1430.txt … 2858.txt` = 미러의 test 세트. 미러의 순서는 UCI 순서를 **뒤섞은** 것이다. 그래서 저자의 인덱스 기반 질의 파일(`test_data/fut_*_benchmark_queries/characters_*`)을 이 데이터에서 다시 돌릴 수 없다. 대신 `paper_bench/gen_pairs.py`가 시드를 고정해 새 쌍을 뽑는다. 레이블이 있는 절반의 글자별 개수는 저자의 글자별 총수(`character_classification_data/`)의 45–60 %다. 같은 집합을 무작위로 나눈 것과 들어맞는다. |

## characters_uci/ — UCI 원본 파일, 저자 번호 체계 (이것을 쓴다)

| 항목 | 값 |
|---|---|
| 출처 | UCI 저장소의 `mixoutALL_shifted.mat`(2008-03-14에 만든 MATLAB 5.0 파일). 사용자가 제공했다. SHA-256은 `characters_uci/SOURCE_SHA256.txt`에 있다 |
| 변환 | `convert_characters_uci.py`. 저자의 `character_converter.m`을 한 줄씩 옮겼다. 출력은 `data/<i>.txt`이고, i = 1..2858은 파일 안의 순서다 |
| 동일성 확인 | 저자의 글자별 인덱스 목록(`test_data/character_classification_data/*_dataset.txt`, 2 858항목)이 모든 인덱스에서 `consts.charlabels`와 일치한다. 저자의 `characters_fut_decider_computed_distances.check`(저자의 all-characters 1 000쌍의 δ*)를 `original`이 1.2e-8 안으로 재현한다(`paper_bench/results/VERIFY_uci.md` 참고) |
| 결과 | 곡선 2 858개, 중복 제거 후 평균 꼭짓점 120.99개(최소 61, 최대 183) |
| 따라서 | 저자의 인스턴스 파일을 바꾸지 않고 그대로 쓸 수 있다. `fut_val_computation_benchmark_queries/characters_full_*`(LMF 21 000쌍)와 `fut_decider_benchmark_queries/characters_fut_decider{,_samechar}_*`(각각 판정자 인스턴스 23 × 1 000개)이다. `paper_bench/run_uci.sh`가 바로 이 파일들을 돌린다. `characters/`(미러 순서, 전체 브랜치에만 있음)는 이전 결과에만 쓰였다. |

## sigspatial/ — 전체 집합(곡선 20 199개), 저자의 `shortest-sf.tgz`

| 항목 | 값 |
|---|---|
| 출처 | `shortest-sf.tgz`(ACM SIGSPATIAL GIS Cup 2017). 저자의 `fetch_and_convert_data.py`가 martinwerner.de에서 내려받는 파일이다. 그 호스트는 내려가 있어서 다른 경로로 구했고, 여기에 `sigspatial/shortest-sf.tgz`(54.6 MB)로 커밋했다. SHA-256은 `sigspatial/SOURCE_SHA256.txt`에 있다 |
| 변환 | `convert_sigspatial.py`. 모든 `files/file-NNNNNN.dat`에서 머리 줄 `x y k tid`를 지운다. 저자의 `tail -n +2`와 정확히 같다. 파서는 3-4열을 무시한다. `sigspatial/data/`(207 MB)는 git-ignore 대상이므로 `python3 paper_data/convert_sigspatial.py`로 다시 만든다 |
| 결과 | 곡선 20 199개, 평균 **꼭짓점 247.9개**(논문 Table 1: 247.8). `dataset.txt` = 정렬한 파일 이름 목록 |
| 동일성 확인 | 저자의 `sigspatial_fut_decider_computed_distances.check`(저자 1 000쌍의 delta*)를 세 방식 모두 모든 쌍에서 1.33e-8 안으로 재현한다. 단 `original`은 1 000쌍 중 998쌍이다. 두 쌍은 12 GB를 넘는다(experiment_log §6.9, §6.12 참고). 정정(2026-10-21): 이 실패는 `original/`의 기저 사례 수정 때문이다. 저자 코드는 두 쌍을 모두 5 GB 안에서 끝낸다([REPORT.md](../paper_bench/authors_check/REPORT.md) §4.2). 저자의 판정자 파일 23개 `sigspatial_fut_decider_<l>_<sign>.txt`는 `paper_bench/queries/sigspatial_paperq_*`로 그대로 복사했다. 이 파일들을 `paper_bench/run_sig.sh`(r1, 무효한 실행시간 측정)와 `paper_bench/run_wsl_paired.sh`(r3, 유효한 실행시간 측정; experiment_log §6.12)가 돌린다 |

## sigspatial_subset/ — **곡선 20 199개 중 101개** (부분집합!) — 이 브랜치에는 들어 있지 않음

| 항목 | 값 |
|---|---|
| 출처 | `github.com/MemoryMmy/OCJ_srajectory_similarity`(GIS Cup 2017 참가작)에 대회의 101개 파일 테스트 표본 `data/files/file-000000.dat … file-000100.dat`가 들어 있다. 커밋은 `SOURCE_COMMIT.txt`에 있다 |
| 동일성 | 논문의 `sigspatial/file-NNNNNN.dat`와 같은 파일, 같은 형식이다(EPSG:3857 미터 단위, 머리 줄 `x y k tid`, 3–4열은 파서가 무시). 예를 들어 저자의 질의 파일은 `file-000015.dat`를 참조하는데, 이 파일도 이 101개 중 하나다 |
| 변환 | 머리 줄을 지웠다. 저자의 `fetch_and_convert_data.py`가 하는 일(`tail -n +2`)과 정확히 같다 |
| 결과 | 곡선 101개, 평균 **꼭짓점 201.0개**(최소 18, 최대 659). 전체 20 199개의 평균은 247.8이다 |
| 주의 | 대체됨(2026-10-21): 이제 전체 집합이 위의 `sigspatial/`에 있다. 당시에는 전체 `shortest-sf.tgz`(궤적 20 199개, 점 ≈5 M개)를 접근 가능한 어느 호스트에서도 받을 수 없었다. 이 부분집합의 수치는 진짜 GIS Cup 곡선에서 나왔지만, 전체의 0.5 %이고 평균적으로 조금 짧다. 전체 집합을 돌리려면 `https://www.martinwerner.de/files/shortest-sf.tgz`를 내려받는다. 그다음 모든 `file-*.dat`의 머리 줄을 지워 `paper_data/sigspatial/data/`에 넣고, `dataset.txt`를 쓰고, `paper_bench/gen_pairs.py`가 그곳을 가리키게 한다. |

## 포함하지 않은 것

Geolife(2.2 GB, `**/Geolife Trajectories 1.3/`는 git-ignore 대상) — 이 트리의 이전
`geolife_100` 실행 결과는 `_c5_bench_geo.csv` / `candidate5/README.candidate5.md` §4에 있다.
