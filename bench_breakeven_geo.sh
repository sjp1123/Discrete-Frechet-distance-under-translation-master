#!/bin/bash
# Break-even curve: candidate (CGAL+maximal) vs original (CGAL,no-max) over real
# Geolife pairs, bucketed by n. ratio = orig/cand ; >1 means maximal is net profit.
cd /home/sj10132/FRECHET_EVOLVE2
DATA="original/test_data/benchmark/Geolife Trajectories 1.3/data"
O=./original/build/calc_frechet_distance_under_translation
C1=./candidate/build/calc_frechet_distance_under_translation
DIR=test_cases/geolife_100
TMP=$(mktemp)

algo_ms() {
  awk '
   /Arrangement computation of n/ { if (match($0,/sum = [0-9.eE+-]+/)) s+=substr($0,RSTART+6)+0 }
   /computation of n/ && !/Arrangement/ { if (match($0,/sum = [0-9.eE+-]+/)) s+=substr($0,RSTART+6)+0 }
   END{printf "%.3f", s}'
}

i=0
while read nrep f1 f2; do
  A="$DATA/$f1"; B="$DATA/$f2"; [ -f "$A" ] || continue
  o=$($O  "$A" "$B" fut_lmf 2>/dev/null | algo_ms)
  c=$($C1 "$A" "$B" fut_lmf 2>/dev/null | algo_ms)
  echo "$nrep $o $c" >> "$TMP"
  i=$((i+1)); if [ $((i % 20)) -eq 0 ]; then echo "...$i/100" >&2; fi
done < $DIR/manifest.txt

echo
echo "==== maximal-on-CGAL: candidate vs original (internal arr+frechet), by n ===="
printf "%-10s %6s %12s %12s %10s %s\n" "n-bucket" "count" "orig(ms)" "cand(ms)" "orig/cand" "verdict"
echo "---------------------------------------------------------------------------"
awk '
{ n=$1; o=$2; c=$3; b=int(n/100)*100; cnt[b]++; os[b]+=o; cs[b]+=c; to+=o; tc+=c; }
END{
  for(b=100;b<=1000;b+=100) if(cnt[b]>0){
    r=(cs[b]>0)?os[b]/cs[b]:0;
    printf "%-10s %6d %12.0f %12.0f %9.2fx  %s\n", b"-"(b+99), cnt[b], os[b], cs[b], r,
           (os[b]>cs[b]?"PROFIT":"loss");
  }
  print "---------------------------------------------------------------------------";
  r=(tc>0)?to/tc:0;
  printf "%-10s %6d %12.0f %12.0f %9.2fx  %s\n","TOTAL",0,to,tc,r,(to>tc?"PROFIT":"loss");
}' "$TMP"
rm -f "$TMP"
