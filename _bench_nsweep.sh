#!/bin/bash
# Global n6 sweep: original (CGAL, no-max) vs candidate (CGAL, maximal), by curve length n.
# Reports orig/candidate ratios for Arrangement, Frechet, and TOTAL (internal N6 timers).
# totX > 1  => candidate faster (maximal is a NET WIN);  < 1 => net loss.
ROOT="/mnt/c/Users/shiju/Downloads/Discrete-Frechet-distance-under-translation-master/Discrete-Frechet-distance-under-translation-master"
DIR="$ROOT/_nsweep"
O="$HOME/b_original/calc_frechet_distance_under_translation"
C1="$HOME/b_candidate/calc_frechet_distance_under_translation"
OUT="$ROOT/_nsweep.out"
TL=60   # per-invocation timeout (s)

arr_ms(){ awk '/Arrangement computation of n/{if(match($0,/sum = [0-9.eE+-]+/))a+=substr($0,RSTART+6)+0}END{printf "%.3f",a}'; }
fre_ms(){ awk '/computation of n/&&!/Arrangement/{if(match($0,/sum = [0-9.eE+-]+/))f+=substr($0,RSTART+6)+0}END{printf "%.3f",f}'; }
dist(){   awk 'match($0,/is: [0-9.eE+-]+/){v=substr($0,RSTART+4)}END{print v+0}'; }

{
printf "%-6s %-3s | %9s %9s %7s | %9s %9s %7s | %9s %9s %7s | %s\n" \
  n p O_arr C_arr arrX O_fre C_fre freX O_tot C_tot totX match
echo "-----------------------------------------------------------------------------------------------------------------"
for f in "$DIR"/n*_a.txt; do
  b="${f%_a.txt}_b.txt"; base=$(basename "$f" _a.txt)
  n=${base%%_*}; n=${n#n}; n=$((10#$n)); p=${base##*_p}
  oo=$(timeout $TL "$O"  "$f" "$b" n6 2>/dev/null); orc=$?
  cc=$(timeout $TL "$C1" "$f" "$b" n6 2>/dev/null); crc=$?
  if [ $orc -ne 0 ] || [ $crc -ne 0 ]; then
    printf "n=%-4s p=%s  TIMEOUT/err (orig_rc=%s cand_rc=%s, TL=%ss) -- stopping sweep\n" "$n" "$p" "$orc" "$crc" "$TL"
    break
  fi
  oa=$(printf '%s' "$oo"|arr_ms); ca=$(printf '%s' "$cc"|arr_ms)
  of=$(printf '%s' "$oo"|fre_ms); cf=$(printf '%s' "$cc"|fre_ms)
  od=$(printf '%s' "$oo"|dist);   cd=$(printf '%s' "$cc"|dist)
  awk -v n="$n" -v p="$p" -v oa="$oa" -v ca="$ca" -v of="$of" -v cf="$cf" -v od="$od" -v cd="$cd" 'BEGIN{
    ot=oa+of; ct=ca+cf;
    arrX=(ca>0)?oa/ca:0; freX=(cf>0)?of/cf:0; totX=(ct>0)?ot/ct:0;
    d=od-cd; if(d<0)d=-d; m=(d<1e-6)?"OK":"DIFF";
    printf "n=%-4s %-3s | %9.2f %9.2f %6.2fx | %9.2f %9.2f %6.2fx | %9.2f %9.2f %6.2fx | %s\n",
      n,p,oa,ca,arrX,of,cf,freX,ot,ct,totX,m;
  }'
done
} | tee "$OUT"
echo "saved -> $OUT" >&2
