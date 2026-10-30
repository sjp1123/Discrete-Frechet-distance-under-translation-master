# r6 — candidate8 검증: 저자 코드, candidate7, candidate8

candidate8(커밋 e264dd4)은 candidate7에서 한 가지만 바꾼다. LMF 기저 사례의 극대 집합 열거 반지름을 δ에서 δ + s로 넓힌다(`MAXREGION_SLACK=budget`, s = 판정자 여유 9e-9 중 반올림에 쓰지 않는 부분). 이 폴더는 그 검증 결과다.

- 검증 중에 결함을 하나 찾아 고쳤다(커밋 f51bcbd, 6절).
- 실행시간은 r5와 같은 방식으로 두 번 쟀다(2026-10-29).
  - **1차**: 저자 코드, candidate7, candidate8(e264dd4)
  - **2차**: candidate7, candidate8(f51bcbd)
- 자세한 표는 [`RESULTS.md`](RESULTS.md)에 있다.

## 요약

1. **값 계산(LMF)이 유의하게 빨라진다.** 두 번의 측정이 같은 결과를 낸다.

   | 벤치마크 | candidate7 ÷ candidate8: 총 시간 | 인스턴스별 기하평균 [95 % CI] | 실행 단위 부호 검정 |
   |---|--:|--:|---|
   | Characters 21,000쌍 (1차 / 2차) | 1.247× / 1.241× | 1.404× [1.391, 1.415] / 1.398× [1.387, 1.409] | 100쌍 묶음 210개 중 192 / 202개 빠름, p = 6e-38 / 1e-49 |
   | Sigspatial 1,000쌍 (1차 / 2차) | 1.249× / 1.250× | 1.230× [1.194, 1.268] / 1.227× [1.190, 1.265] | 1,000쌍 중 649 / 655쌍 빠름, p = 3e-21 / 7e-23 |

   두 방식이 똑같은 일을 한 쌍(기저 사례 없음, 호출 수 같음)의 비는 1.01 / 0.99(1차), 1.02 / 1.00(2차)이다. 이것이 잡음 수준이다.
2. **저자 코드 대비 가속이 커진다** (1차, 같은 측정 안의 비).

   | 벤치마크 | 저자 ÷ candidate7: 총 / 기하평균 | 저자 ÷ candidate8: 총 / 기하평균 |
   |---|--:|--:|
   | Characters | 3.69× / 3.68× | **4.60× / 5.16×** |
   | Sigspatial | 28.2× / 2.84× | **35.3× / 3.50×** |

   - 저자 코드보다 느리면서 저자 코드와 다른 일을 한 쌍: Characters 213 → 16, Sigspatial 15 → 2.
   - 저자 코드보다 블랙박스 호출이 많은 쌍: Characters 199 → 0, Sigspatial 15 → 1.
   - candidate8의 목표(저자 코드보다 상자를 더 보는 쌍 없애기)는 달성됐다.
3. **결정 문제는 달라지지 않는다.** 4^ℓ 69,000질의에서 답이 모두 맞고, 블랙박스 호출 수가 같다(6,272,563 대 6,272,561 등). 시간 차이는 실행 단위(파일) 검정에서 유의하지 않거나, 두 측정에서 방향이 바뀐다. 다시 재면 사라진다(4절).
4. **수정 전 candidate8에는 정확성 결함이 있었다.** 곡선 하나가 1e8 떨어진 r4 검증 가족(far_t_1e8)에서 값 2개가 5.1e-3, 1.9e-2만큼 크게 나왔다. candidate7은 0개다. 원인을 고친 판은 r4 오라클 가족 19개 전부에서 오답이 0이다(6절). 이 수정은 논문 데이터에서 계산을 바꾸지 않는다.
5. **값은 모두 10⁻⁷ 안이지만, 저자 코드보다 조금 아래로 간다.**
   - candidate8 − 저자 코드의 중앙값은 Characters −5.1e-9, Sigspatial −1.0e-9다. candidate7은 +2.4e-9, +2.2e-9였다.
   - 가속의 상당 부분이 여기서 나오므로(3절), 논문에는 이 점을 함께 적는다.

## 1. candidate8이 바꾸는 것

[`candidate8/README.candidate8.md`](../../candidate8/README.candidate8.md)를 줄이면 이렇다.

- LMF는 상자를 `max − min_dist ≤ ε/2`이면 버린다.
- 저자 코드는 기저 사례에서 배열 꼭짓점을 판정자 여유 δ + 9e-9로 시험하므로, 이분 탐색이 상자 최솟값보다 조금 아래에 멈춘다.
- candidate7은 열거를 정확히 δ에서 했으므로 0.2–8e-9 위에 멈췄다. 시작·끝 하한이 δ* − ε/2 근처인 쌍에서는 그 몇 e-9 때문에 상자들이 버려지지 않았다.
- candidate8은 열거를 δ + s에서 한다(Characters s ≈ 8.0e-9, Sigspatial ≈ 4.9e-9). `MAXREGION_SLACK=0`이면 candidate7과 같다.
  - 측정으로 확인했다: Characters 300쌍과 Sigspatial 40쌍에서 값과 호출 수가 비트 단위로 같다. 수정 전후 모두 그렇다.

## 2. 측정 방법 (r5와 같음)

- **기계와 빌드**: r5와 같은 서버 컨테이너(Xeon 2.1 GHz, 2 vCPU)다. 세 방식을 `r4_original_vs_candidate7/scripts/build.sh`로 같은 플래그(RelWithDebInfo, CGAL 5.6 GMPXX)로 빌드했다.
- **실행**:
  - CPU 1에 고정하고 CPU 0은 비웠다.
  - 실행 단위마다 방식을 번갈아 돌렸다. 단위는 Characters 100쌍 묶음, Sigspatial 쌍, 결정 문제 질의 파일이다.
  - 시계는 `steady_clock`이다.
  - 측정 동안 다른 작업은 돌리지 않았다.
- **1차** (`scripts/run_r6.sh`, 16:26–19:15)
  - Characters와 Sigspatial: 세 방식
  - 결정 문제 4^ℓ: candidate7과 candidate8만. 저자 코드의 결정 문제 값은 r5에 있다.
- **2차** (`scripts/run_r6b.sh`, 21:20–22:38): candidate7과 수정판 candidate8, 같은 방식.
- **절대 시간**: r5보다 약 10 % 길다(저자 코드 Characters 123.9 → 138.2 ms/쌍). 비교는 같은 측정 안의 비로만 한다.
- **유의성** (`scripts/sig_r6.py`)
  - 인스턴스별 시간비의 기하평균에 부트스트랩 95 % CI를 붙였다.
  - 검정은 부호 검정과 Wilcoxon 부호순위 검정을 썼다.
  - 같은 프로세스 안의 인스턴스는 같은 시간대를 함께 겪어 독립이 아니다. 그래서 실행 단위(묶음·쌍·파일)의 정확 부호 검정을 기준으로 삼는다.

## 3. 어디서 빨라지나 (1차, 단계별 합)

| 단계 | Characters c7 → c8 (s) | Sigspatial c7 → c8 (s) |
|---|--:|--:|
| 전처리 | 91.0 → 90.3 | 22.6 → 22.6 |
| Lipschitz 블랙박스 호출 | 341.0 → 296.8 | 24.8 → 20.2 |
| 원판 선택(배열 추정) | 170.8 → 104.7 | 26.9 → 17.5 |
| 기저 사례 | 137.8 → 101.4 | 7.8 → 5.5 |
| 나머지(미귀속) | 45.7 → 37.4 | 3.6 → 2.7 |
| **합** | **786.3 → 630.6** | **85.6 → 68.6** |

- 기저 사례만이 아니라 상자를 다루는 모든 단계가 줄어든다.
- 기저 사례 값이 조금 낮게 정해지면 `max − min_dist ≤ ε/2` 종료가 일찍 걸린다. 그만큼 상자가 덜 만들어진다.
- 블랙박스 호출은 68.7 M → 56.2 M(Characters), 3.72 M → 3.04 M(Sigspatial)이다.

## 4. candidate8이 candidate7보다 느린 경우

1차 기준으로 candidate8이 느린 인스턴스는 Characters 6,587개, Sigspatial 351개다. 대부분 잡음이다.

- **잡음의 크기**
  - 두 방식이 똑같은 일을 한 쌍에서도 18.7 %가 10 % 넘게, 1.9 %가 50 % 넘게 느리게 나온다.
  - candidate8이 가장 크게 느렸던 쌍들은 호출 수가 거의 같았다. 다시 재면 candidate7 32 / 48 / 53 ms, candidate8 41 / 33 / 60 ms처럼 차이가 없다.
  - 묶음 전체가 느려진 경우는 없었다. 묶음별 전처리 시간비의 중앙값이 0.99다.
- **실제로 일이 늘어난 경우**
  - candidate8의 호출이 더 많은 쌍은 3,790개(Characters)와 144개(Sigspatial)다.
  - 늘어난 정도는 합계 +0.3 %, 쌍별 중앙값 +0.5 %, 최대 +25 %다.
  - 시간은 그 쌍들에서 Characters −1.5 %, Sigspatial +3.1 %다.
  - 원인은 기저 사례 값이 바뀌면서 상자의 하한과 처리 순서가 달라지는 것, 그리고 열거 반지름이 넓어 증인점이 조금 늘어나는 것이다.
- **반대쪽**: 호출이 줄어든 쌍은 12,962개(Characters)이고, 호출 −29.6 %, 시간 −31.0 %다.
- **결정 문제**
  - 두 방식의 호출 수와 극대 집합 통계가 같다. 결정 문제 질의는 임계 반지름에서 10⁻⁷ 이상 떨어져 있어서, 수 e-9의 s가 극대 집합을 바꾸지 않는다.
  - 파일별 시간비 c7 ÷ c8은 1차 0.975 / 0.963 / 0.962(p = 0.40 / 1.00 / 0.09), 2차 0.952 / 0.914 / 1.007이다. 방향과 크기가 측정마다 다르다.
  - 차이가 가장 컸던 파일을 다시 재 보면 시간이 같다.

    | 파일 | 실행 중 측정 c7 / c8 | 다시 잰 값 c7 / c8 (3회) |
    |---|---|---|
    | Sigspatial −9 minus (1차, 앞 120질의) | 12.7 % 느림 | 33.2 / 37.1 / 34.7 s 대 33.6 / 33.3 / 33.7 s |
    | all-characters −7 minus (2차) | 39.1 / 42.4 s | 41.0 / 41.1 / 39.2 s 대 40.9 / 39.7 / 41.5 s |

## 5. 값

| | Characters 중앙값 [최소, 최대] | Sigspatial 중앙값 [최소, 최대] |
|---|--:|--:|
| candidate7 − 저자 코드 | +2.35e-9 [−4.5e-9, +1.35e-8] | +2.21e-9 [−4.2e-9, +1.25e-8] |
| candidate8 − 저자 코드 | −5.07e-9 [−1.26e-8, +5.6e-9] | −1.0e-9 [−9.1e-9, +8.3e-9] |
| candidate8 − candidate7 | −7.99e-9 [−1.23e-8, 0] | −4.4e-9 [−9.2e-9, 0] |

- 모두 계약 10⁻⁷ 안이다.
- 처음 README에는 'candidate7보다 최대 s 아래'라고 적혀 있었는데, 틀린 주장이었다. 이분 탐색이 다른 칸에서 끝나므로 s보다 더 내려갈 수 있다(관측 최대 1.23e-8). README를 고쳤다.

## 6. 정확성

### 6.1 찾은 결함과 수정 (f51bcbd)

- **결함**
  - 열거는 반올림된 원판 중심 c = fl(p − q)를 기준으로 증인점을 인증한다. 판정자는 p − fl(q + t)를 잰다.
  - 판정자 반올림 상한 `deciderRoundingBound`에는 c의 반올림(좌표당 u·|c|)이 빠져 있었다(r4 검증 #6과 같은 항).
  - candidate7은 이 오차를 여유 η − e ≈ 9e-9 안에서 흡수했다. budget을 쓰면 남는 여유가 slack/9 ≈ 1e-9로 줄어든다.
- **재현**
  - 곡선 1이 원점 근처, 곡선 2가 (1e8, −3e7)에 있는 far_t_1e8 가족에서, 최적점을 품은 상자에 기저 사례가 NO를 냈다.
  - 그 결과 값 2개가 크게 틀렸다: 10.6125 대 정답 10.6075, 9.0393 대 정답 9.0199.
  - `MAXREGION_SLACK=1e-9`만 줘도 틀리고, 0이면 맞는다.
- **수정**
  - `deciderRoundingBound`에 2u(max|B| + R)을 더했다. 상자 점 w에서 의미 있는 원판은 |c| ≤ |w| + R ≤ max|B| + R을 만족하기 때문이다.
  - 그런 좌표에서는 budget이 0이 되고, candidate7의 반지름으로 돌아간다.
- **논문 데이터에서 계산이 바뀌지 않는다** (수정 전후 전체 비교)
  - Characters 21,000쌍: 값·호출 수 동일
  - 결정 문제 69,000질의: 답·호출 수 동일
  - Sigspatial 1,000쌍: 998쌍 동일. 나머지 2쌍(δ* ≈ 3.1e4, 1.7e4)은 값이 3.6e-9, 3.3e-9 다르고 호출이 4–5회 다르다.

### 6.2 검사 결과

| 검사 | 규모 | candidate8 e264dd4 | candidate8 f51bcbd | candidate7 |
|---|--:|--:|--:|--:|
| r4 오라클 가족, 값 오답 | 17,553 인스턴스 (19가족) | far 가족 4개만 돌림: **2** | **0** | 0 |
| r4 오라클 가족, 결정 문제 틀린 NO / 틀린 YES | 241,434 질의 | far 가족: 0 / 0 | 0 / 0 | 0 / 0 |
| 실제 데이터 v ± 5e-8 (`scripts/run_tight.sh`) | 44,000 질의 | — | 0 | 0 |
| c7_verify 키트 QUICK (`ARM=candidate8`) | 56 작업 | — | 모두 통과, 오차 0 | — |
| r6 측정 전체: 값 차이 > 10⁻⁷ / 결정 문제 오답 | 22,000쌍 / 69,000질의 | 0 / 0 | 0 / 0 | 0 / 0 |

- **45초 제한 초과 7건 (두 방식 공통)**
  - cluster 가족에서 δ* 바로 아래의 NO 질의 7개가 45초 제한을 넘는다. candidate7과 수정판이 같다(6개 겹침).
  - 원판 24–58개짜리 성분이 DP 한도 20을 넘어 CGAL 배열로 대체되는 경로다.
  - r4 검증 원자료를 보면 같은 candidate7이 이 8개 질의에 36–85 s 걸렸고 정상 종료로 기록됐다. 그래서 이번에 처음 생긴 문제가 아니라 원래 느린 질의다.

## 7. 파일

| 경로 | 내용 |
|---|---|
| `RESULTS.md` | 생성된 표: `analyze_r6.py` 출력, 1차·2차 `sig_r6.py` 출력, 오라클 표 |
| `raw/r6_timing_3arm.tar.xz` | 1차 원시 CSV (`chars/`, `sig/`, `dec4/` × `gitlab`, `candidate7`, `candidate8`), rc·progress·log |
| `raw/r6b_timing_c7_c8fix.tar.xz` | 2차 원시 CSV (`candidate7`, `candidate8` = f51bcbd) |
| `raw/r6_correctness.tar.xz` | 오라클 표·실패·시간 초과 기록(e264dd4 far 가족은 CSV 포함), ±5e-8 질의 CSV, 키트 요약, 수정 전후 비교 |
| `scripts/run_r6.sh`, `run_r6b.sh` | 1차·2차 측정 |
| `scripts/run_tight.sh`, `make_tight_queries.py` | 실제 데이터 v ± 5e-8 질의 |
| `scripts/run_oracle.sh`, `check_oracle.py` | r4 오라클 가족(한 방식씩), 판정 표 |
| `scripts/analyze_r6.py`, `sig_r6.py` | 표와 유의성 검정 |

## 8. 재현

```
ARMS="authors_gitlab candidate7 candidate8" DEPS=/opt/deps bash paper_bench/r4_original_vs_candidate7/scripts/build.sh
bash paper_bench/r6_candidate8/scripts/run_r6.sh                      # W=~/r6, 약 2시간 50분
bash paper_bench/r6_candidate8/scripts/run_r6b.sh                     # W=~/r6b, 약 1시간 20분
python3 paper_bench/r6_candidate8/scripts/sig_r6.py ~/r6              # ~/r6b도 같은 방식
ARM=candidate8 bash paper_bench/r6_candidate8/scripts/run_tight.sh ~/r6v/tight
mkdir -p ~/verif && tar xzf paper_bench/r4_original_vs_candidate7/verification/raw/oracle_instances.tar.gz -C ~/verif
bash paper_bench/r6_candidate8/scripts/run_oracle.sh candidate8       # OUT=~/r6v/oracle_candidate8
QUICK=1 ARM=candidate8 B=~/b_c8v P=1 bash paper_bench/c7_verify/verify.sh ~/c8_kit_quick
```

`runner.py`(authors_check)는 이제 `candidate8` 방식을 알고, 그 방식을 `MAXREGION_SLACK=budget`으로 돌린다.
