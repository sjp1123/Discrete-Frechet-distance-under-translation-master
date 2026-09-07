#!/bin/bash
# X1 per-pair runner (main path fut_lmf, arm contrast via MAXIMAL_MODE).
# Round-robin over modes within each pair (§6.2), R reps, min over reps.
# Emits one CSV row per (pair,mode): the min-rep wall + internal timers + counts.
# Internal timers isolate the arr+predicate loop that maximal actually affects.
ROOT="/mnt/c/Users/shiju/Downloads/Discrete-Frechet-distance-under-translation-master/Discrete-Frechet-distance-under-translation-master"
C2="$HOME/b_candidate2/calc_frechet_distance_under_translation"
DATA="$HOME/geodata"
MAN="$ROOT/test_cases/geolife_100/manifest.txt"
LIMIT="${1:-100}"
REPS="${2:-2}"           # first rep is warmup-ish; we take min over all reps
ALG="${3:-fut_lmf}"
MODES="on off mask-only"
OUT="$HOME/_x1/perpair.csv"; mkdir -p "$HOME/_x1"
echo "pair,mode,ans,wall_ms,arr_ms,fre_ms,builds,m,K,queries" > "$OUT"

parse() {  # stdin: one process output -> "arr fre builds m K queries" (space sep)
  awk '
    /Arrangement computation of n/ { if(match($0,/sum = [0-9.eE+-]+/)) arr=substr($0,RSTART+6)+0 }
    /computation of n/ && !/Arrangement/ { if(match($0,/sum = [0-9.eE+-]+/)) fre=substr($0,RSTART+6)+0 }
    /\[sweep-stats\]/ { for(i=1;i<=NF;i++){ if(split($i,kv,"=")==2) v[kv[1]]=kv[2] }
      builds=v["calls"]; m=v["before"]; K=v["after"]; q=v["queries"] }
    END{ printf "%.4f %.4f %d %d %d %d", arr+0, fre+0, builds+0, m+0, K+0, q+0 }'
}
ansof() { grep -E "calcDistance2 \(LMF\)|for N6 is" | tail -1 | grep -oE "[0-9]+\.[0-9]+" | tail -1; }

pi=0
while read -r nrep f1 f2; do
  [ -f "$DATA/$f1" ] && [ -f "$DATA/$f2" ] || continue
  pi=$((pi+1)); [ "$pi" -gt "$LIMIT" ] && break
  pid="${f1%.txt}_${f2%.txt}"
  declare -A best_wall best_line
  for r in $(seq 1 "$REPS"); do
    for mode in $MODES; do
      t0=$EPOCHREALTIME
      out=$(MAXIMAL_MODE="$mode" "$C2" "$DATA/$f1" "$DATA/$f2" "$ALG" 2>/dev/null)
      t1=$EPOCHREALTIME
      wall=$(awk -v a="$t0" -v b="$t1" 'BEGIN{printf "%.2f",(b-a)*1000}')
      ans=$(printf '%s\n' "$out" | ansof)
      pp=$(printf '%s\n' "$out" | parse)
      # keep the rep with smallest wall for this mode
      cur=${best_wall[$mode]:-99999999}
      if awk -v w="$wall" -v c="$cur" 'BEGIN{exit !(w<c)}'; then
        best_wall[$mode]=$wall
        best_line[$mode]="$pid,$mode,$ans,$wall,$pp"
      fi
    done
  done
  for mode in $MODES; do
    # normalize spaces to commas in the parse tail
    echo "${best_line[$mode]}" | awk -F, '{n=split($0,a,","); printf "%s,%s,%s,%s", a[1],a[2],a[3],a[4]; m=split($5,b," "); for(i=1;i<=m;i++) printf ",%s", b[i]; print ""}'
  done >> "$OUT"
  unset best_wall best_line
done < "$MAN"
echo "wrote $OUT ($(($(wc -l < "$OUT")-1)) rows)" >&2
