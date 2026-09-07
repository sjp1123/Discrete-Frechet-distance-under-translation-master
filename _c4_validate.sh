#!/bin/bash
# Candidate 4 validation: Čech maximal-region pipeline vs candidate2's
# arrangement-vertex path.
#
#   arm c2       candidate2, default settings                      (baseline)
#   arm c4legacy candidate4, MAXREGION=legacy  -> must equal c2 exactly
#   arm c4       candidate4, MAXREGION=cech    -> the pipeline under test
#
# Usage: _c4_validate.sh [PAIRS=20] [REPS=2] [ALG=fut_lmf]
ROOT="/mnt/c/Users/shiju/Downloads/Discrete-Frechet-distance-under-translation-master/Discrete-Frechet-distance-under-translation-master"
C2="$HOME/b_candidate2/calc_frechet_distance_under_translation"
C4="$HOME/b_candidate4/calc_frechet_distance_under_translation"
DATA="$HOME/geodata"
MAN="$ROOT/test_cases/geolife_100/manifest.txt"
LIMIT="${1:-20}"
REPS="${2:-2}"
ALG="${3:-fut_lmf}"
OUT="$HOME/_c4/perpair.csv"; mkdir -p "$HOME/_c4"

for b in "$C2" "$C4"; do
  [ -x "$b" ] || { echo "missing binary: $b" >&2; exit 1; }
done

echo "pair,arm,ans,wall_ms,arr_ms,fre_ms,builds,m,K,queries,regions,mec_fail" > "$OUT"

parse() {  # stdin -> "arr fre builds m K queries regions mec_fail"
  awk '
    /Arrangement computation of n/ { if(match($0,/sum = [0-9.eE+-]+/)) arr=substr($0,RSTART+6)+0 }
    /computation of n/ && !/Arrangement/ { if(match($0,/sum = [0-9.eE+-]+/)) fre=substr($0,RSTART+6)+0 }
    /\[sweep-stats\]/ { for(i=1;i<=NF;i++){ if(split($i,kv,"=")==2) v[kv[1]]=kv[2] }
      builds=v["calls"]; m=v["before"]; K=v["after"]; q=v["queries"] }
    /\[maxregion-stats\]/ { for(i=1;i<=NF;i++){ if(split($i,kw,"=")==2) w[kw[1]]=kw[2] }
      reg=w["regions"]; mf=w["mec_fail"] }
    END{ printf "%.4f %.4f %d %d %d %d %d %d", arr+0, fre+0, builds+0, m+0, K+0, q+0, reg+0, mf+0 }'
}
ansof() { grep -E "calcDistance2 \(LMF\)|for N6 is" | tail -1 | grep -oE "[0-9]+\.[0-9]+" | tail -1; }

run_arm() {  # $1=arm  -> echoes "ans<TAB>wall<TAB>parsed"
  local arm="$1" bin env
  case "$arm" in
    c2)       bin="$C2"; env="";;
    c4legacy) bin="$C4"; env="legacy";;
    c4)       bin="$C4"; env="cech";;
  esac
  local t0=$EPOCHREALTIME
  local out
  out=$(MAXREGION="$env" "$bin" "$DATA/$f1" "$DATA/$f2" "$ALG" 2>/dev/null)
  local t1=$EPOCHREALTIME
  local wall; wall=$(awk -v a="$t0" -v b="$t1" 'BEGIN{printf "%.2f",(b-a)*1000}')
  printf '%s\t%s\t%s' "$(printf '%s\n' "$out" | ansof)" "$wall" "$(printf '%s\n' "$out" | parse)"
}

pi=0
while read -r nrep f1 f2; do
  [ -f "$DATA/$f1" ] && [ -f "$DATA/$f2" ] || continue
  pi=$((pi+1)); [ "$pi" -gt "$LIMIT" ] && break
  pid="${f1%.txt}_${f2%.txt}"
  declare -A best_wall best_line
  for r in $(seq 1 "$REPS"); do
    for arm in c2 c4legacy c4; do
      IFS=$'\t' read -r ans wall pp <<< "$(run_arm "$arm")"
      cur=${best_wall[$arm]:-99999999}
      if awk -v w="$wall" -v c="$cur" 'BEGIN{exit !(w<c)}'; then
        best_wall[$arm]=$wall
        best_line[$arm]="$pid,$arm,$ans,$wall,$(echo "$pp" | tr ' ' ',')"
      fi
    done
  done
  for arm in c2 c4legacy c4; do echo "${best_line[$arm]}"; done >> "$OUT"
  unset best_wall best_line
done < "$MAN"

echo "wrote $OUT ($(($(wc -l < "$OUT")-1)) rows)" >&2
cp "$OUT" "$ROOT/_c4_perpair.csv" 2>/dev/null
