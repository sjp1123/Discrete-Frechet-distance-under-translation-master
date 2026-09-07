#!/bin/bash
cd /home/sj10132/FRECHET_EVOLVE2
ORIG=./original/build/calc_frechet_distance_under_translation
CAND=./candidate2/build/calc_frechet_distance_under_translation
DIR=test_cases/bench_n_sweep
EPS=1e-7

run_time() {  # $1=binary $2=a $3=b  -> prints "ms|dist"
  local out start end ms dist
  start=$(date +%s.%N)
  out=$("$1" "$2" "$3" fut_lmf 2>/dev/null)
  end=$(date +%s.%N)
  ms=$(awk -v s="$start" -v e="$end" 'BEGIN{printf "%.1f",(e-s)*1000}')
  dist=$(echo "$out" | grep 'LMF' | grep 'is:' | sed 's/.*is: //')
  echo "${ms}|${dist}"
}

printf "%-12s %12s %12s %8s %8s\n" "case" "original(ms)" "cand2(ms)" "ratio" "match"
echo "---------------------------------------------------------------"
declare -A ot ct
for n in 0100 0200 0400 0600 0800 1000; do
  osum=0; csum=0
  for p in 0 1; do
    A=$DIR/n${n}_p${p}_a.txt
    B=$DIR/n${n}_p${p}_b.txt
    [ -f "$A" ] || continue
    r1=$(run_time "$ORIG" "$A" "$B"); om=${r1%%|*}; od=${r1##*|}
    r2=$(run_time "$CAND" "$A" "$B"); cm=${r2%%|*}; cd=${r2##*|}
    match=$(awk -v o="$od" -v c="$cd" -v eps="$EPS" 'BEGIN{d=o-c;if(d<0)d=-d;print (d<=eps)?"OK":"DIFF"}')
    ratio=$(awk -v o="$om" -v c="$cm" 'BEGIN{printf "%.2fx",(c>0)?o/c:0}')
    printf "%-12s %12s %12s %8s %8s\n" "n${n}_p${p}" "$om" "$cm" "$ratio" "$match"
    osum=$(awk -v a="$osum" -v b="$om" 'BEGIN{printf "%.1f",a+b}')
    csum=$(awk -v a="$csum" -v b="$cm" 'BEGIN{printf "%.1f",a+b}')
  done
  ratio=$(awk -v o="$osum" -v c="$csum" 'BEGIN{printf "%.2fx",(c>0)?o/c:0}')
  printf "%-12s %12s %12s %8s   (n=%s total)\n" "  SUBTOTAL" "$osum" "$csum" "$ratio" "$n"
  echo "---------------------------------------------------------------"
done
