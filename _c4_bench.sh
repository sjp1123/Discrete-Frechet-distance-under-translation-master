#!/bin/bash
# Candidate 4 runtime benchmark, same protocol as _factor_perpair.sh:
# per-pair round-robin with the arm order shuffled every rep, REPS reps with the
# min taken per (pair,arm), 60 s censoring, wall + internal timers.
#
#   A0   b_original                                   Epeck + DCEL, no maximal
#   D1   b_candidate2 (default)                       the current baseline
#   C4S  b_candidate4 MAXREGION=legacy +LEGACY_SLACK  slack alignment ONLY
#   C4N  b_candidate4 MAXREGION=cech MAXREGION_SLACK=0  pipeline ONLY
#   C4   b_candidate4 (default)                       both
#
# Usage: _c4_bench.sh [PAIRS=100] [REPS=3] [ALG=fut_lmf] [OUT]
ROOT="/mnt/c/Users/shiju/Downloads/Discrete-Frechet-distance-under-translation-master/Discrete-Frechet-distance-under-translation-master"
DATA="$HOME/geodata"
MAN="$ROOT/test_cases/geolife_100/manifest.txt"
LIMIT="${1:-100}"
REPS="${2:-3}"
ALG="${3:-fut_lmf}"
OUT="${4:-$HOME/_c4/bench.csv}"
TMO=60s
mkdir -p "$(dirname "$OUT")"

ARMS="A0 D1 C4S C4N C4"
declare -A BIN
BIN[A0]="$HOME/b_original/calc_frechet_distance_under_translation"
BIN[D1]="$HOME/b_candidate2/calc_frechet_distance_under_translation"
BIN[C4S]="$HOME/b_candidate4/calc_frechet_distance_under_translation"
BIN[C4N]="$HOME/b_candidate4/calc_frechet_distance_under_translation"
BIN[C4]="$HOME/b_candidate4/calc_frechet_distance_under_translation"

for a in $ARMS; do
  [ -x "${BIN[$a]}" ] || { echo "missing binary for arm $a: ${BIN[$a]}" >&2; exit 1; }
done

# env assignments per arm (space separated NAME=VALUE)
armenv() {
  case "$1" in
    C4S) echo "MAXREGION=legacy MAXREGION_LEGACY_SLACK=1";;
    C4N) echo "MAXREGION=cech MAXREGION_SLACK=0";;
    C4)  echo "MAXREGION=cech";;
    *)   echo "";;
  esac
}

TMP=$(mktemp)
trap 'rm -f "$TMP"' EXIT

echo "pair,arm,ans,status,wall_ms,arr_ms,fre_ms,builds,queries" > "$OUT"

parse() {  # $1=file -> "arr fre builds queries"
  awk '
    /Arrangement computation of n/ { if(match($0,/sum = [0-9.eE+-]+/)) arr=substr($0,RSTART+6)+0 }
    /computation of n/ && !/Arrangement/ { if(match($0,/sum = [0-9.eE+-]+/)) fre=substr($0,RSTART+6)+0 }
    /\[sweep-stats\]/ { for(i=1;i<=NF;i++) if(split($i,kv,"=")==2) v[kv[1]]=kv[2] }
    END{ printf "%.4f %.4f %d %d", arr+0, fre+0, v["calls"]+0, v["queries"]+0 }' "$1"
}
ansof() { grep -a "calcDistance2 (LMF)" "$1" | tail -1 | grep -aoE "[0-9]+\.[0-9]+" | tail -1; }

pi=0
while read -r nrep f1 f2; do
  [ -f "$DATA/$f1" ] && [ -f "$DATA/$f2" ] || continue
  pi=$((pi+1)); [ "$pi" -gt "$LIMIT" ] && break
  pid="${f1%.txt}_${f2%.txt}"
  declare -A best line
  for r in $(seq 1 "$REPS"); do
    for arm in $(printf '%s\n' $ARMS | shuf); do
      t0=$EPOCHREALTIME
      env $(armenv "$arm") timeout "$TMO" "${BIN[$arm]}" "$DATA/$f1" "$DATA/$f2" "$ALG" >"$TMP" 2>/dev/null
      rc=$?
      t1=$EPOCHREALTIME
      if [ "$rc" -eq 124 ]; then
        [ -z "${best[$arm]}" ] && { best[$arm]=60000; line[$arm]="$pid,$arm,NA,TIMEOUT,60000,NA,NA,NA,NA"; }
        continue
      fi
      wall=$(awk -v a="$t0" -v b="$t1" 'BEGIN{printf "%.2f",(b-a)*1000}')
      cur=${best[$arm]:-99999999}
      if awk -v w="$wall" -v c="$cur" 'BEGIN{exit !(w<c)}'; then
        best[$arm]=$wall
        pp=$(parse "$TMP")
        line[$arm]="$pid,$arm,$(ansof "$TMP"),OK,$wall,$(echo "$pp" | tr ' ' ',')"
      fi
    done
  done
  for arm in $ARMS; do echo "${line[$arm]}"; done >> "$OUT"
  unset best line
  echo "  [$pi/$LIMIT] $pid" >&2
done < "$MAN"

echo "wrote $OUT ($(($(wc -l < "$OUT")-1)) rows)" >&2
cp "$OUT" "$ROOT/_c4_bench.csv" 2>/dev/null
