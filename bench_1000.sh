#!/bin/bash
cd /home/sj10132/FRECHET_EVOLVE2
ORIG=./original/build/calc_frechet_distance_under_translation
CAND=./candidate2/build/calc_frechet_distance_under_translation
DIR=test_cases/bench_1000
EPS=1e-7
TMP=$(mktemp)

i=0
while read tag n; do
  A=$DIR/${tag}_a.txt
  B=$DIR/${tag}_b.txt
  [ -f "$A" ] || continue

  s=$(date +%s.%N)
  od=$($ORIG "$A" "$B" fut_lmf 2>/dev/null | grep 'LMF' | grep 'is:' | sed 's/.*is: //')
  e=$(date +%s.%N)
  om=$(awk -v s="$s" -v e="$e" 'BEGIN{printf "%.3f",(e-s)*1000}')

  s=$(date +%s.%N)
  cd=$($CAND "$A" "$B" fut_lmf 2>/dev/null | grep 'LMF' | grep 'is:' | sed 's/.*is: //')
  e=$(date +%s.%N)
  cm=$(awk -v s="$s" -v e="$e" 'BEGIN{printf "%.3f",(e-s)*1000}')

  # bucket = floor(n/100)*100
  echo "$n $om $cm $od $cd $tag" >> "$TMP"
  i=$((i+1))
  if [ $((i % 100)) -eq 0 ]; then echo "...$i pairs done" >&2; fi
done < $DIR/manifest.txt

echo
echo "================ Per-n-bucket summary (fut_lmf) ================"
printf "%-10s %6s %12s %12s %8s %8s\n" "n-bucket" "count" "orig(ms)" "cand2(ms)" "speedup" "mismatch"
echo "---------------------------------------------------------------"
awk -v eps="$EPS" '
{
  n=$1; om=$2; cm=$3; od=$4; cd=$5;
  b=int(n/100)*100;
  cnt[b]++; osum[b]+=om; csum[b]+=cm;
  d=od-cd; if(d<0)d=-d;
  if(d>eps){ mm[b]++; mmtot++; print "MISMATCH", $6, "n="n, "orig="od, "cand="cd, "|diff|="d > "/dev/stderr" }
  tco+=om; tcc+=cm; tcnt++;
}
END{
  for(b=100;b<=1000;b+=100){
    if(cnt[b]>0){
      sp=(csum[b]>0)?osum[b]/csum[b]:0;
      printf "%-10s %6d %12.0f %12.0f %7.2fx %8d\n", b"-"(b+99), cnt[b], osum[b], csum[b], sp, mm[b]+0;
    }
  }
  print "---------------------------------------------------------------";
  sp=(tcc>0)?tco/tcc:0;
  printf "%-10s %6d %12.0f %12.0f %7.2fx %8d\n", "TOTAL", tcnt, tco, tcc, sp, mmtot+0;
}' "$TMP"

rm -f "$TMP"
