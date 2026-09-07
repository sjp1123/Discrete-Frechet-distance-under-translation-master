#!/bin/bash
# X2 K_0 contrast: A0 (b_original, Epeck) vs B0 (b_original_epick, Epick), both
# fut_lmf, both no-maximal, kernel-only difference. Per-pair round-robin, R reps
# (min), 60s censoring (§6.4). Emits CSV with wall + internal Arr/Frechet sums.
ROOT="/mnt/c/Users/shiju/Downloads/Discrete-Frechet-distance-under-translation-master/Discrete-Frechet-distance-under-translation-master"
DATA="$HOME/geodata"
MAN="$ROOT/test_cases/geolife_100/manifest.txt"
LIMIT="${1:-100}"; REPS="${2:-1}"
declare -A BIN
BIN[A0]="$HOME/b_original/calc_frechet_distance_under_translation"
BIN[B0]="$HOME/b_original_epick/calc_frechet_distance_under_translation"
ARMS="A0 B0"
OUT="$HOME/_x1/x2_perpair.csv"; mkdir -p "$HOME/_x1"
echo "pair,arm,ans,status,wall_ms,arr_ms,fre_ms" > "$OUT"

ans() { grep -E "calcDistance2 \(LMF\)" | tail -1 | grep -oE "[0-9]+\.[0-9]+" | tail -1; }
parse() {  # -> "arr fre"
  awk '/Arrangement computation of n/ { if(match($0,/sum = [0-9.eE+-]+/)) arr=substr($0,RSTART+6)+0 }
       /computation of n/ && !/Arrangement/ { if(match($0,/sum = [0-9.eE+-]+/)) fre=substr($0,RSTART+6)+0 }
       END{ printf "%.4f %.4f", arr+0, fre+0 }'
}

pi=0
while read -r nrep f1 f2; do
  [ -f "$DATA/$f1" ] && [ -f "$DATA/$f2" ] || continue
  pi=$((pi+1)); [ "$pi" -gt "$LIMIT" ] && break
  pid="${f1%.txt}_${f2%.txt}"
  declare -A best_wall best_line
  for r in $(seq 1 "$REPS"); do
    for arm in $ARMS; do
      t0=$EPOCHREALTIME
      out=$(timeout 60s "${BIN[$arm]}" "$DATA/$f1" "$DATA/$f2" fut_lmf 2>/dev/null); rc=$?
      t1=$EPOCHREALTIME
      wall=$(awk -v a="$t0" -v b="$t1" 'BEGIN{printf "%.2f",(b-a)*1000}')
      if [ "$rc" -eq 124 ]; then
        line="$pid,$arm,NA,TIMEOUT,60000,NA,NA"; w=60001
      else
        a=$(printf '%s\n' "$out" | ans); pp=$(printf '%s\n' "$out" | parse)
        line="$pid,$arm,$a,OK,$wall,${pp% *},${pp#* }"; w=$wall
      fi
      cur=${best_wall[$arm]:-99999999}
      if awk -v x="$w" -v c="$cur" 'BEGIN{exit !(x<c)}'; then best_wall[$arm]=$w; best_line[$arm]="$line"; fi
    done
  done
  for arm in $ARMS; do echo "${best_line[$arm]}"; done >> "$OUT"
  unset best_wall best_line
done < "$MAN"
echo "wrote $OUT ($(($(wc -l < "$OUT")-1)) rows)" >&2
