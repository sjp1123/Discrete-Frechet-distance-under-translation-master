# 그림

## `plot_scatter.py` — 인스턴스별 실행 시간 (논문 Figure 6 형식)

```
python3 paper_bench/figures/plot_scatter.py
```

`fig_scatter_characters.{pdf,png}`, `fig_scatter_sigspatial.{pdf,png}`(한 단 너비)와
`fig_scatter.{pdf,png}`(두 패널, 전체 너비)를 출력한다. PDF의 산점도 레이어는 300 dpi로 래스터화한다.

Bringmann, Kuennemann, Nusser (ESA 2020)의 Figure 6처럼 단순하게 그린다. 로그-로그 축, 인스턴스마다 점 하나, 점선 대각선 y = x만 있고 다른 것은 없다.
원 논문의 축은 이분 탐색(x)과 LMF(y)다. 여기서 x는 원래의 배열 구성을 쓰는 LMF(기준선, `original/`)이고,
y는 극대 집합 열거를 쓰는 LMF(제안 방법, `MAXREGION_EXACT=1 MAXREGION_SLACK=0`으로 돌린 candidate5)다.
대각선 아래의 점은 제안 방법이 더 빠른 인스턴스다. 한 패널 안에서는 두 축의 범위가 같아서 대각선이 45도다.
두 패널의 범위는 서로 다르다.

> **정정 (2026-10-21):** 여기의 기준선은 저자 코드를 수정한 사본인 `original/`이다. 제안 방식도 candidate7이 아니라 candidate5다. 저자 코드 대 candidate7 그림은 `../authors_check/fig_scatter_gitlab_vs_c7*`에 있다([REPORT.md](../authors_check/REPORT.md) §7).

| 패널 | 데이터 | 기계 | 인스턴스 수 |
|---|---|---|--:|
| Characters | `results/raw_characters_uci_lmf_r2.tar.gz`(논문 Table 4의 21,000쌍) | 서버 컨테이너 (Xeon) | 21,000 |
| Sigspatial | `results/raw_sigspatial_lmf_r3.tar.gz`(저자의 판정자 1,000쌍, 전체 집합; r3) | 사용자 PC, WSL2 (Core Ultra 5 125H) | 998 |

두 패널의 절대 시간은 서로 비교할 수 없다. Sigspatial 패널은 두 방식 모두에서 측정한 998쌍을 보여 준다.
나머지 2쌍에서는 기준선이 메모리 12 GB를 넘어서 시간이 없다(제안 방법은 이 두 쌍을 0.52 s와 1.85 s에 끝낸다; `TABLES_uci.md` Table D).
여기서 실패하는 것은 수정된 `original/`뿐이다. 저자 코드는 두 쌍을 5 GB 안에서 497.5 s와 338.0 s에 끝낸다([REPORT.md](../authors_check/REPORT.md) §4.2).
인스턴스별 통계(기하평균, 중앙값, 총 시간 비)는 그림이 아니라 Table C와 D에 있다.

캡션 예시: *모든 인스턴스에서 원래의 배열 구성을 쓰는 LMF(x)와 제안한 극대 집합 열거를 쓰는 LMF(y)의 실행 시간. 점선은 두 시간이 같은 선이다.
왼쪽: [2, Table 4]의 Characters 인스턴스 21,000개. 오른쪽: 곡선 20,199개 전체 집합에서 [2]의 Sigspatial 판정자 쌍.
기준선(저자 구현을 수정한 사본)이 메모리 12 GB를 넘은 2쌍은 뺐다. 두 패널은 서로 다른 기계에서 측정했고 축 범위도 다르다.*
