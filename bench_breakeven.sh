#!/bin/bash
# candidate (CGAL + maximal) vs original (CGAL, no maximal): when does adding the
# maximal filter on top of CGAL become a NET PROFIT? Measure internal algorithm
# time (arrangement build + Frechet decider) for both, ratio = orig/cand (>1 = profit).
cd /home/sj10132/FRECHET_EVOLVE2
O=./original/build/calc_frechet_distance_under_translation
C1=./candidate/build/calc_frechet_distance_under_translation

algo_ms() {  # arrangement + frechet internal sums
  awk '
   /Arrangement computation of n/ { if (match($0,/sum = [0-9.eE+-]+/)) s+=substr($0,RSTART+6)+0 }
   /computation of n/ && !/Arrangement/ { if (match($0,/sum = [0-9.eE+-]+/)) s+=substr($0,RSTART+6)+0 }
   END{printf "%.1f", s}'
}

row() {  # $1=label  $2..=list of "A|B" pairs
  local label="$1"; shift
  local osum=0 csum=0
  for pr in "$@"; do
    A="${pr%%|*}"; B="${pr##*|}"
    [ -f "$A" ] || continue
    o=$($O "$A" "$B" fut_lmf 2>/dev/null | algo_ms)
    c=$($C1 "$A" "$B" fut_lmf 2>/dev/null | algo_ms)
    osum=$(awk -v a=$osum -v b=$o 'BEGIN{print a+b}')
    csum=$(awk -v a=$csum -v b=$c 'BEGIN{print a+b}')
  done
  awk -v l="$label" -v o=$osum -v c=$csum 'BEGIN{
    printf "%-8s original=%9.1f ms   candidate=%9.1f ms   orig/cand=%.2fx  %s\n",
      l, o, c, (c>0?o/c:0), (o>c?"<-- maximal PROFIT":"maximal loss");
  }'
}

S=test_cases/bench_n_sweep
N2=test_cases/bench_n2000
echo "=== maximal-on-CGAL profitability (internal arr+frechet time) ==="
row "n=400"  "$S/n0400_p0_a.txt|$S/n0400_p0_b.txt" "$S/n0400_p1_a.txt|$S/n0400_p1_b.txt"
row "n=600"  "$S/n0600_p0_a.txt|$S/n0600_p0_b.txt" "$S/n0600_p1_a.txt|$S/n0600_p1_b.txt"
row "n=800"  "$S/n0800_p0_a.txt|$S/n0800_p0_b.txt" "$S/n0800_p1_a.txt|$S/n0800_p1_b.txt"
row "n=1000" "$S/n1000_p0_a.txt|$S/n1000_p0_b.txt" "$S/n1000_p1_a.txt|$S/n1000_p1_b.txt"
row "n=2000" "$N2/pair000_a.txt|$N2/pair000_b.txt" "$N2/pair002_a.txt|$N2/pair002_b.txt"
