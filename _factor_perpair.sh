#!/bin/bash
# Definitive factor run: arms A0,B0,D0,D1 in one per-pair round-robin (§6.2), so
# K_0=A0/B0, S_0=B0/D0, M_D=D0/D1, T=A0/D1 are all mutually consistent.
#   A0 = b_original            (Epeck, DCEL, no-max)
#   B0 = b_original_epick      (Epick, DCEL, no-max)
#   D0 = b_candidate2 off      (double, list, no-max)
#   D1 = b_candidate2 on       (double, list, maximal)
# Arm order shuffled per pair; 60s censoring; wall + internal Arr/Frechet.
ROOT="/mnt/c/Users/shiju/Downloads/Discrete-Frechet-distance-under-translation-master/Discrete-Frechet-distance-under-translation-master"
DATA="$HOME/geodata"
MAN="$ROOT/test_cases/geolife_100/manifest.txt"
LIMIT="${1:-100}"
C2="$HOME/b_candidate2/calc_frechet_distance_under_translation"
declare -A BIN ENV
BIN[A0]="$HOME/b_original/calc_frechet_distance_under_translation";        ENV[A0]=""
BIN[B0]="$HOME/b_original_epick/calc_frechet_distance_under_translation";  ENV[B0]=""
BIN[D0]="$C2"; ENV[D0]="off"
BIN[D1]="$C2"; ENV[D1]="on"
OUT="$HOME/_x1/factor_perpair.csv"; mkdir -p "$HOME/_x1"
echo "pair,arm,ans,status,wall_ms,arr_ms,fre_ms" > "$OUT"

ans() { grep -E "calcDistance2 \(LMF\)" | tail -1 | grep -oE "[0-9]+\.[0-9]+" | tail -1; }
parse() { awk '/Arrangement computation of n/ { if(match($0,/sum = [0-9.eE+-]+/)) arr=substr($0,RSTART+6)+0 }
       /computation of n/ && !/Arrangement/ { if(match($0,/sum = [0-9.eE+-]+/)) fre=substr($0,RSTART+6)+0 }
       END{ printf "%.4f %.4f", arr+0, fre+0 }'; }

pi=0
while read -r nrep f1 f2; do
  [ -f "$DATA/$f1" ] && [ -f "$DATA/$f2" ] || continue
  pi=$((pi+1)); [ "$pi" -gt "$LIMIT" ] && break
  pid="${f1%.txt}_${f2%.txt}"
  # shuffle arm order per pair
  order=$(printf "A0\nB0\nD0\nD1\n" | shuf | tr '\n' ' ')
  for arm in $order; do
    t0=$EPOCHREALTIME
    out=$(MAXIMAL_MODE="${ENV[$arm]}" timeout 60s "${BIN[$arm]}" "$DATA/$f1" "$DATA/$f2" fut_lmf 2>/dev/null); rc=$?
    t1=$EPOCHREALTIME
    wall=$(awk -v a="$t0" -v b="$t1" 'BEGIN{printf "%.2f",(b-a)*1000}')
    if [ "$rc" -eq 124 ]; then
      echo "$pid,$arm,NA,TIMEOUT,60000,NA,NA"
    else
      a=$(printf '%s\n' "$out" | ans); pp=$(printf '%s\n' "$out" | parse)
      echo "$pid,$arm,$a,OK,$wall,${pp% *},${pp#* }"
    fi
  done >> "$OUT"
done < "$MAN"
echo "wrote $OUT ($(($(wc -l < "$OUT")-1)) rows)" >&2
