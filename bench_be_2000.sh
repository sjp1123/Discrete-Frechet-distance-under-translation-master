#!/bin/bash
cd /home/sj10132/FRECHET_EVOLVE2
O=./original/build/calc_frechet_distance_under_translation
C1=./candidate/build/calc_frechet_distance_under_translation
N2=test_cases/bench_n2000
algo_ms() {
  awk '
   /Arrangement computation of n/ { if (match($0,/sum = [0-9.eE+-]+/)) s+=substr($0,RSTART+6)+0 }
   /computation of n/ && !/Arrangement/ { if (match($0,/sum = [0-9.eE+-]+/)) s+=substr($0,RSTART+6)+0 }
   END{printf "%.1f", s}'
}
echo "=== n=2000: candidate(CGAL+max) vs original(CGAL,no-max), internal arr+frechet ==="
to=0; tc=0
for p in 000 001 002 003 004; do
  A=$N2/pair${p}_a.txt; B=$N2/pair${p}_b.txt
  o=$($O "$A" "$B" fut_lmf 2>/dev/null | algo_ms)
  c=$($C1 "$A" "$B" fut_lmf 2>/dev/null | algo_ms)
  awk -v p=$p -v o=$o -v c=$c 'BEGIN{printf "  pair%s: orig=%9.1f ms  cand=%9.1f ms  orig/cand=%.2fx  %s\n",p,o,c,(c>0?o/c:0),(o>c?"PROFIT":"loss")}'
  to=$(awk -v a=$to -v b=$o 'BEGIN{print a+b}'); tc=$(awk -v a=$tc -v b=$c 'BEGIN{print a+b}')
done
awk -v o=$to -v c=$tc 'BEGIN{printf "  TOTAL : orig=%9.1f ms  cand=%9.1f ms  orig/cand=%.2fx  %s\n",o,c,(c>0?o/c:0),(o>c?"PROFIT":"loss")}'
