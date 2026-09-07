#!/bin/bash
cd /home/sj10132/FRECHET_EVOLVE2
DATA="original/test_data/benchmark/Geolife Trajectories 1.3/data"
O=./original/build/calc_frechet_distance_under_translation
C1=./candidate/build/calc_frechet_distance_under_translation
C2=./candidate2/build/calc_frechet_distance_under_translation
C3=./candidate3/build/calc_frechet_distance_under_translation
DIR=test_cases/geolife_small
TMP=$(mktemp)

arr_ms() { awk '/Arrangement computation of n/ { if (match($0,/sum = [0-9.eE+-]+/)) a+=substr($0,RSTART+6)+0 } END{printf "%.4f",a}'; }
getd()  { grep 'LMF' | grep 'is:' | sed 's/.*is: //'; }
wall()  { local s e; s=$(date +%s.%N); "$1" "$2" "$3" fut_lmf >/dev/null 2>&1; e=$(date +%s.%N); awk -v s=$s -v e=$e 'BEGIN{printf "%.3f",(e-s)*1000}'; }

i=0
while read nrep f1 f2; do
  A="$DATA/$f1"; B="$DATA/$f2"; [ -f "$A" ] || continue
  oo=$($O "$A" "$B" fut_lmf 2>/dev/null); od=$(echo "$oo" | getd)
  a1=$($C1 "$A" "$B" fut_lmf 2>/dev/null | arr_ms)
  a2=$($C2 "$A" "$B" fut_lmf 2>/dev/null | arr_ms)
  c3o=$($C3 "$A" "$B" fut_lmf 2>/dev/null); a3=$(echo "$c3o" | arr_ms); d3=$(echo "$c3o" | getd)
  w1=$(wall $C1 "$A" "$B"); w2=$(wall $C2 "$A" "$B"); w3=$(wall $C3 "$A" "$B")
  echo "ARR $a1 $a2 $a3" >> "$TMP"
  echo "WALL $w1 $w2 $w3" >> "$TMP"
  awk -v o="$od" -v c="$d3" 'BEGIN{d=o-c;if(d<0)d=-d; if(d>1e-7) print "MISMATCH c3" > "/dev/stderr"}'
  i=$((i+1)); if [ $((i % 25)) -eq 0 ]; then echo "...$i/100" >&2; fi
done < $DIR/manifest.txt

awk '
  $1=="ARR" { c1a+=$2; c2a+=$3; c3a+=$4 }
  $1=="WALL"{ c1w+=$2; c2w+=$3; c3w+=$4 }
  END{
    print "===== Geolife n[10,200], 100 pairs:  candidate(CGAL full) vs cand2(float+band) vs cand3(exact+band) =====";
    printf "%-26s %14s %14s %14s\n","", "candidate", "candidate2", "candidate3";
    printf "%-26s %14s %14s %14s\n","", "(CGAL arrangement)","(float+band)","(exact+band)";
    print  "-----------------------------------------------------------------------------------";
    printf "%-26s %14.1f %14.1f %14.1f\n","Arrangement build (ms)", c1a, c2a, c3a;
    printf "%-26s %14.1f %14.1f %14.1f\n","Wall clock total (ms)",  c1w, c2w, c3w;
    print  "";
    printf "Arrangement-build speed vs candidate2(float):  candidate=%.2fx slower,  candidate3=%.2fx slower\n",
           c1a/c2a, c3a/c2a;
  }' "$TMP"
rm -f "$TMP"
