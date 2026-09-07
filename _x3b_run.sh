#!/bin/bash
# X3-b regime sweep: M_D = D0/D1 (off/on) vs arrangement_cut_limit, fut_lmf.
# Does the maximal reduction help more as the leaf arrangements grow (break-even)?
ROOT="/mnt/c/Users/shiju/Downloads/Discrete-Frechet-distance-under-translation-master/Discrete-Frechet-distance-under-translation-master"
C2="$HOME/b_candidate2/calc_frechet_distance_under_translation"
DATA="$HOME/geodata"
MAN="$ROOT/test_cases/geolife_100/manifest.txt"
LIMIT="${1:-12}"; CUTS="${2:-12 24 48}"; TO="${3:-240}"
OUT="$HOME/_x1/x3b.csv"; mkdir -p "$HOME/_x1"
echo "cut_limit,pair,mode,status,wall_ms,arr_ms,fre_ms" > "$OUT"
parse(){ awk '/Arrangement computation of n/{if(match($0,/sum = [0-9.eE+-]+/))a=substr($0,RSTART+6)+0}
  /computation of n/&&!/Arrangement/{if(match($0,/sum = [0-9.eE+-]+/))f=substr($0,RSTART+6)+0}
  END{printf "%.4f %.4f",a+0,f+0}'; }
for cut in $CUTS; do
  pi=0
  while read -r nrep f1 f2; do
    [ -f "$DATA/$f1" ] && [ -f "$DATA/$f2" ] || continue
    pi=$((pi+1)); [ "$pi" -gt "$LIMIT" ] && break
    pid="${f1%.txt}_${f2%.txt}"
    for mode in on off; do
      t0=$EPOCHREALTIME
      out=$(CUT_LIMIT=$cut MAXIMAL_MODE=$mode timeout "${TO}s" "$C2" "$DATA/$f1" "$DATA/$f2" fut_lmf 2>/dev/null); rc=$?
      t1=$EPOCHREALTIME
      w=$(awk -v a="$t0" -v b="$t1" 'BEGIN{printf "%.2f",(b-a)*1000}')
      if [ "$rc" -eq 124 ]; then echo "$cut,$pid,$mode,TIMEOUT,240000,NA,NA"
      else pp=$(printf '%s\n' "$out" | parse); echo "$cut,$pid,$mode,OK,$w,${pp% *},${pp#* }"; fi
    done
  done < "$MAN"
  echo "  cut=$cut done" >&2
done >> "$OUT"
echo "wrote $OUT" >&2
