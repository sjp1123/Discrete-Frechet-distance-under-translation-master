#!/bin/bash
cd /home/sj10132/FRECHET_EVOLVE2
DATA="original/test_data/benchmark/Geolife Trajectories 1.3/data"
ORIG=./original/build/calc_frechet_distance_under_translation
CAND=./candidate2/build/calc_frechet_distance_under_translation
DIR=test_cases/geolife_small
EPS=1e-7
TMP=$(mktemp)

algo_ms() {
  awk '
    /Arrangement computation of n/ { if (match($0,/sum = [0-9.eE+-]+/)) { a+=substr($0,RSTART+6)+0 } }
    /computation of n/ && !/Arrangement/ { if (match($0,/sum = [0-9.eE+-]+/)) { f+=substr($0,RSTART+6)+0 } }
    END { printf "%.4f", a+f }'
}
getdist() { grep 'LMF' | grep 'is:' | sed 's/.*is: //'; }

i=0; wo=0; wc_=0
while read nrep f1 f2; do
  A="$DATA/$f1"; B="$DATA/$f2"
  [ -f "$A" ] || continue
  s=$(date +%s.%N); oout=$($ORIG "$A" "$B" fut_lmf 2>/dev/null); e=$(date +%s.%N)
  wo=$(awk -v a="$wo" -v s="$s" -v e="$e" 'BEGIN{print a+(e-s)*1000}')
  s=$(date +%s.%N); cout=$($CAND "$A" "$B" fut_lmf 2>/dev/null); e=$(date +%s.%N)
  wc_=$(awk -v a="$wc_" -v s="$s" -v e="$e" 'BEGIN{print a+(e-s)*1000}')
  oa=$(echo "$oout" | algo_ms); od=$(echo "$oout" | getdist)
  ca=$(echo "$cout" | algo_ms); cd=$(echo "$cout" | getdist)
  echo "$nrep $oa $ca $od $cd ${f1}+${f2}" >> "$TMP"
  i=$((i+1)); if [ $((i % 25)) -eq 0 ]; then echo "...$i/100" >&2; fi
done < $DIR/manifest.txt

echo
echo "===== GEOLIFE n in [10,200] : internal algorithm time (arrangement+Frechet, ms) ====="
printf "%-10s %6s %12s %12s %8s %8s\n" "n-bucket" "count" "orig(ms)" "cand2(ms)" "speedup" "mismatch"
echo "----------------------------------------------------------------------"
awk -v eps="$EPS" '
{
  n=$1; oa=$2; ca=$3; od=$4; cd=$5;
  b=(n<50)?10:(int(n/50)*50); cnt[b]++; osum[b]+=oa; csum[b]+=ca;
  d=od-cd; if(d<0)d=-d; if(d>eps){mm[b]++; mmtot++; print "  MISMATCH",$6,"n="n,"|d|="d > "/dev/stderr"}
  tco+=oa; tcc+=ca; tcnt++;
}
END{
  split("10 50 100 150 200",order," ");
  for(i=1;i<=5;i++){b=order[i]+0; if(cnt[b]>0){sp=(csum[b]>0)?osum[b]/csum[b]:0;
    lab=(b==10)?"10-49":b"-"(b+49);
    printf "%-10s %6d %12.2f %12.2f %7.2fx %8d\n", lab, cnt[b], osum[b], csum[b], sp, mm[b]+0}}
  print "----------------------------------------------------------------------";
  sp=(tcc>0)?tco/tcc:0;
  printf "%-10s %6d %12.2f %12.2f %7.2fx %8d\n","TOTAL",tcnt,tco,tcc,sp,mmtot+0;
}' "$TMP"
awk -v o="$wo" -v c="$wc_" 'BEGIN{printf "WALL-CLOCK total (100 pairs): orig=%.0f ms  cand2=%.0f ms  speedup=%.2fx\n", o, c, o/c}'
rm -f "$TMP"
