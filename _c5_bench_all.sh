#!/bin/bash
# All four arms on geolife_100, one round-robin, ONE machine and ONE toolchain.
#
# _c5_bench_geo.sh compares candidate5 against original only; ranking it against
# candidate2/candidate4 then had to borrow those arms' numbers from
# _c4_bench.csv, which was recorded on a different host.  This script removes
# that confound: every arm is rebuilt here and interleaved pair-by-pair, so the
# untouched-code term that sets the Amdahl ceiling is identical for all of them.
#
#   A0   b_original     Epeck + DCEL, all arrangement vertices
#   D1   b_candidate2   double list + inclusion-bitmask maximal filter
#   C4   b_candidate4   candidate2 + Cech pipeline          (MAXREGION=cech)
#   C5   b_candidate5   original   + Cech pipeline, Epeck fallback (MAXREGION=cech)
#
# Usage: _c5_bench_all.sh [PAIRS] [REPS] [OUT] [DATA] [MANIFEST]
set -u
ROOT="${ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)}"
LIMIT="${1:-100}"
REPS="${2:-3}"
OUT="${3:-$HOME/_c5/bench_all.csv}"
DATA="${4:-$HOME/geodata}"
MAN="${5:-$ROOT/test_cases/geolife_100/manifest.txt}"
TMO=60s
mkdir -p "$(dirname "$OUT")"

ARMS="A0 D1 C4 C5"
declare -A BIN
BIN[A0]="$HOME/b_original/calc_frechet_distance_under_translation"
BIN[D1]="$HOME/b_candidate2/calc_frechet_distance_under_translation"
BIN[C4]="$HOME/b_candidate4/calc_frechet_distance_under_translation"
BIN[C5]="$HOME/b_candidate5/calc_frechet_distance_under_translation"
for a in $ARMS; do
  [ -x "${BIN[$a]}" ] || { echo "missing binary for arm $a: ${BIN[$a]}" >&2; exit 1; }
done

armenv() {
  case "$1" in
    C4|C5) echo "MAXREGION=cech";;
    *)     echo "MAXREGION=unused";;
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
while read -r nrep f1 f2; do
  [ -f "$DATA/$f1" ] && [ -f "$DATA/$f2" ] || continue
  pi=$((pi+1)); [ "$pi" -gt "$LIMIT" ] && break
  pid="${f1%.txt}_${f2%.txt}"
  declare -A best line
  for r in $(seq 1 "$REPS"); do
    for arm in $(printf '%s\n' $ARMS | shuf); do
      t0=$EPOCHREALTIME
      env $(armenv "$arm") timeout "$TMO" "${BIN[$arm]}" "$DATA/$f1" "$DATA/$f2" fut_lmf >"$TMP" 2>/dev/null
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
  echo "  [$pi/$LIMIT] $pid" >&2
done < <(tr -d '\r' < "$MAN")   # the checked-out manifest may carry CRLF

echo "wrote $OUT ($(($(wc -l < "$OUT")-1)) rows)" >&2
