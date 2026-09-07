#!/bin/bash
# 3-way decomposition, adapted to WSL out-of-source builds.
#   original  : CGAL, NO maximal filter      -> ~/b_original
#   candidate : CGAL, maximal filter          -> ~/b_candidate   (orig->cand = MAXIMAL)
#   candidate2: float, maximal filter          -> ~/b_candidate2  (cand->cand2 = FLOAT)
# awk logic is identical to the repo's bench_3way.sh; only paths + tee-to-file differ.

ROOT="/mnt/c/Users/shiju/Downloads/Discrete-Frechet-distance-under-translation-master/Discrete-Frechet-distance-under-translation-master"
cd "$ROOT" || exit 1
DATA="original/test_data/benchmark/Geolife Trajectories 1.3/data"
O="$HOME/b_original/calc_frechet_distance_under_translation"
C1="$HOME/b_candidate/calc_frechet_distance_under_translation"
C2="$HOME/b_candidate2/calc_frechet_distance_under_translation"
DIR=test_cases/geolife_small
OUT="$ROOT/_bench_3way.out"

for b in "$O" "$C1" "$C2"; do
  [ -x "$b" ] || { echo "MISSING BINARY: $b" >&2; exit 1; }
done

TMP=$(mktemp)
arr_ms() { awk '/Arrangement computation of n/ { if (match($0,/sum = [0-9.eE+-]+/)) a+=substr($0,RSTART+6)+0 } END{printf "%.4f",a}'; }
fre_ms() { awk '/computation of n/ && !/Arrangement/ { if (match($0,/sum = [0-9.eE+-]+/)) f+=substr($0,RSTART+6)+0 } END{printf "%.4f",f}'; }

i=0
while read -r nrep f1 f2; do
  A="$DATA/$f1"; B="$DATA/$f2"; [ -f "$A" ] || continue
  oo=$("$O"  "$A" "$B" fut_lmf 2>/dev/null)
  c1=$("$C1" "$A" "$B" fut_lmf 2>/dev/null)
  c2=$("$C2" "$A" "$B" fut_lmf 2>/dev/null)
  echo "ARR $(echo "$oo"|arr_ms) $(echo "$c1"|arr_ms) $(echo "$c2"|arr_ms)" >> "$TMP"
  echo "FRE $(echo "$oo"|fre_ms) $(echo "$c1"|fre_ms) $(echo "$c2"|fre_ms)" >> "$TMP"
  i=$((i+1)); if [ $((i % 25)) -eq 0 ]; then echo "...$i pairs" >&2; fi
done < "$DIR/manifest.txt"

awk -v npairs="$i" '
  $1=="ARR"{ oa+=$2; c1a+=$3; c2a+=$4 }
  $1=="FRE"{ of+=$2; c1f+=$3; c2f+=$4 }
  END{
    printf "================ 3-way decomposition (geolife_small, %d pairs) ================\n", npairs;
    printf "%-26s %12s %12s %12s\n","component(ms)","original","candidate","candidate2";
    printf "%-26s %12s %12s %12s\n","","(CGAL,no-max)","(CGAL,max)","(float,max)";
    print  "---------------------------------------------------------------------------------";
    printf "%-26s %12.1f %12.1f %12.1f\n","Arrangement (build)", oa, c1a, c2a;
    printf "%-26s %12.1f %12.1f %12.1f\n","Frechet (decider)",   of, c1f, c2f;
    printf "%-26s %12.1f %12.1f %12.1f\n","TOTAL (arr+frechet)", oa+of, c1a+c1f, c2a+c2f;
    print  "";
    print  "Isolated effects:";
    printf "  MAXIMAL filter (orig -> candidate, CGAL held): total %.1f -> %.1f = %.2fx;  Frechet %.1f -> %.1f = %.2fx\n",
           oa+of, c1a+c1f, (oa+of)/(c1a+c1f), of, c1f, of/c1f;
    printf "  FLOAT arithmetic (candidate -> candidate2, maximal held): total %.1f -> %.1f = %.2fx;  Arrangement %.1f -> %.1f = %.2fx\n",
           c1a+c1f, c2a+c2f, (c1a+c1f)/(c2a+c2f), c1a, c2a, c1a/c2a;
    printf "  COMBINED (orig -> candidate2): %.1f -> %.1f = %.2fx\n", oa+of, c2a+c2f, (oa+of)/(c2a+c2f);
  }' "$TMP" | tee "$OUT"
rm -f "$TMP"
echo "saved -> $OUT" >&2
