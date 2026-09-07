#!/bin/bash
cd /home/sj10132/FRECHET_EVOLVE2
DATA="original/test_data/benchmark/Geolife Trajectories 1.3/data"
ORIG=./original/build/calc_frechet_distance_under_translation
CAND=./candidate2/build/calc_frechet_distance_under_translation
DIR=test_cases/geolife_100
EPS=1e-7
TMP=$(mktemp)

# quick sanity on first pair
read nrep f1 f2 < $DIR/manifest.txt
echo "sanity pair: $f1 $f2 (n=$nrep)" >&2
echo -n "  orig=" >&2; $ORIG "$DATA/$f1" "$DATA/$f2" fut_lmf 2>/dev/null | grep 'LMF' | grep 'is:' | sed 's/.*is: //' >&2
echo -n "  cand=" >&2; $CAND "$DATA/$f1" "$DATA/$f2" fut_lmf 2>/dev/null | grep 'LMF' | grep 'is:' | sed 's/.*is: //' >&2

i=0
while read nrep f1 f2; do
  A="$DATA/$f1"; B="$DATA/$f2"
  [ -f "$A" ] || continue
  s=$(date +%s.%N); od=$($ORIG "$A" "$B" fut_lmf 2>/dev/null | grep 'LMF' | grep 'is:' | sed 's/.*is: //'); e=$(date +%s.%N)
  om=$(awk -v s="$s" -v e="$e" 'BEGIN{printf "%.3f",(e-s)*1000}')
  s=$(date +%s.%N); cd=$($CAND "$A" "$B" fut_lmf 2>/dev/null | grep 'LMF' | grep 'is:' | sed 's/.*is: //'); e=$(date +%s.%N)
  cm=$(awk -v s="$s" -v e="$e" 'BEGIN{printf "%.3f",(e-s)*1000}')
  echo "$nrep $om $cm $od $cd ${f1}+${f2}" >> "$TMP"
  i=$((i+1)); if [ $((i % 20)) -eq 0 ]; then echo "...$i/100 done" >&2; fi
done < $DIR/manifest.txt

echo
echo "======= Geolife 100-pair benchmark (fut_lmf, n in [100,1000]) ======="
printf "%-10s %6s %12s %12s %8s %8s\n" "n-bucket" "count" "orig(ms)" "cand2(ms)" "speedup" "mismatch"
echo "----------------------------------------------------------------------"
awk -v eps="$EPS" '
{
  n=$1; om=$2; cm=$3; od=$4; cd=$5;
  b=int(n/100)*100; cnt[b]++; osum[b]+=om; csum[b]+=cm;
  d=od-cd; if(d<0)d=-d;
  if(d>eps){ mm[b]++; mmtot++; print "  MISMATCH", $6, "n="n, "orig="od, "cand="cd, "|diff|="d > "/dev/stderr" }
  tco+=om; tcc+=cm; tcnt++;
}
END{
  for(b=100;b<=1000;b+=100) if(cnt[b]>0){
    sp=(csum[b]>0)?osum[b]/csum[b]:0;
    printf "%-10s %6d %12.0f %12.0f %7.2fx %8d\n", b"-"(b+99), cnt[b], osum[b], csum[b], sp, mm[b]+0;
  }
  print "----------------------------------------------------------------------";
  sp=(tcc>0)?tco/tcc:0;
  printf "%-10s %6d %12.0f %12.0f %7.2fx %8d\n", "TOTAL", tcnt, tco, tcc, sp, mmtot+0;
}' "$TMP"
rm -f "$TMP"
