#!/bin/bash
# candidate5 benchmark: original (Epeck arrangement) vs original+Cech pipeline.
# Protocol copied from _c4_bench.sh: per-pair round-robin, arm order reshuffled
# every rep, REPS reps with the min wall taken per (pair,arm), timeout censoring.
#
#   A0   b_original                                    Epeck + DCEL, all vertices
#   C5L  b_candidate5 MAXREGION=legacy                 control: must equal A0
#   C5N  b_candidate5 MAXREGION=cech MAXREGION_SLACK=0 pipeline only
#   C5   b_candidate5 MAXREGION=cech                   pipeline + slack alignment
#
# Usage: c5_bench.sh <case_dir> [PAIRS] [REPS] [OUT]
set -u
DIR="${1:?case dir}"
LIMIT="${2:-100}"
REPS="${3:-3}"
OUT="${4:-$HOME/_c5/bench.csv}"
TMO=120s
mkdir -p "$(dirname "$OUT")"

ARMS="A0 C5L C5N C5"
declare -A BIN
BIN[A0]="$HOME/b_original/calc_frechet_distance_under_translation"
BIN[C5L]="$HOME/b_candidate5/calc_frechet_distance_under_translation"
BIN[C5N]="$HOME/b_candidate5/calc_frechet_distance_under_translation"
BIN[C5]="$HOME/b_candidate5/calc_frechet_distance_under_translation"

armenv() {
  case "$1" in
    C5L) echo "MAXREGION=legacy";;
    C5N) echo "MAXREGION=cech MAXREGION_SLACK=0";;
    C5)  echo "MAXREGION=cech";;
    *)   echo "MAXREGION=unused";;
  esac
}

TMP=$(mktemp); trap 'rm -f "$TMP"' EXIT
echo "pair,arm,ans,status,wall_ms,arr_ms,fre_ms,bbcalls,regions" > "$OUT"

parse() {
  awk '
    /Arrangement computation of n/ { if(match($0,/sum = [0-9.eE+-]+/)) arr=substr($0,RSTART+6)+0 }
    /computation of n/ && !/Arrangement/ { if(match($0,/sum = [0-9.eE+-]+/)) fre=substr($0,RSTART+6)+0 }
    /black-box calls:/ { bb=$NF+0 }
    /\[maxregion-stats\]/ { for(i=1;i<=NF;i++) if(split($i,kv,"=")==2) v[kv[1]]=kv[2] }
    END{ printf "%.4f,%.4f,%d,%d", arr+0, fre+0, bb+0, v["regions"]+0 }' "$1"
}
ansof() { grep -a "calcDistance2 (LMF)" "$1" | tail -1 | grep -aoE "[0-9]+\.[0-9]+" | tail -1; }

pi=0
for fa in "$DIR"/*_a.txt; do
  fb="${fa%_a.txt}_b.txt"
  [ -f "$fb" ] || continue
  pi=$((pi+1)); [ "$pi" -gt "$LIMIT" ] && break
  pid=$(basename "${fa%_a.txt}")
  declare -A best line
  for r in $(seq 1 "$REPS"); do
    for arm in $(printf '%s\n' $ARMS | shuf); do
      t0=$EPOCHREALTIME
      env $(armenv "$arm") timeout "$TMO" "${BIN[$arm]}" "$fa" "$fb" fut_lmf >"$TMP" 2>/dev/null
      rc=$?
      t1=$EPOCHREALTIME
      if [ "$rc" -ne 0 ]; then
        [ -z "${best[$arm]:-}" ] && { best[$arm]=999999; line[$arm]="$pid,$arm,NA,RC$rc,NA,NA,NA,NA,NA"; }
        continue
      fi
      wall=$(awk -v a="$t0" -v b="$t1" 'BEGIN{printf "%.2f",(b-a)*1000}')
      cur=${best[$arm]:-99999999}
      if awk -v w="$wall" -v c="$cur" 'BEGIN{exit !(w<c)}'; then
        best[$arm]=$wall
        line[$arm]="$pid,$arm,$(ansof "$TMP"),OK,$wall,$(parse "$TMP")"
      fi
    done
  done
  for arm in $ARMS; do echo "${line[$arm]}"; done >> "$OUT"
  unset best line
  echo "  [$pi] $pid" >&2
done

echo "wrote $OUT ($(($(wc -l < "$OUT")-1)) rows)" >&2
