#!/bin/bash
# Correctness gates for candidate5.
#   1. C5 (default)       vs A0 — answers within eps
#   2. C5 with DP_LIMIT=4 vs A0 — forces the Epeck fallback to fire (overflow>0)
#   3. C5L (legacy)       vs A0 — must be bit-identical
# Usage: c5_check.sh <case_dir> [N]
set -u
DIR="${1:?case dir}"
N="${2:-20}"
A0="$HOME/b_original/calc_frechet_distance_under_translation"
C5="$HOME/b_candidate5/calc_frechet_distance_under_translation"
ansof() { grep -a "calcDistance2 (LMF)" | tail -1 | grep -aoE "[0-9]+\.[0-9]+" | tail -1; }

printf "%-10s %-24s %-24s %-24s %-24s %s\n" pair A0 C5 "C5(dp=4)" C5L overflow
i=0; bad=0; badl=0; ovf_total=0
for fa in "$DIR"/*_a.txt; do
  fb="${fa%_a.txt}_b.txt"; [ -f "$fb" ] || continue
  i=$((i+1)); [ "$i" -gt "$N" ] && break
  pid=$(basename "${fa%_a.txt}")
  o=$("$A0" "$fa" "$fb" fut_lmf 2>/dev/null)
  a=$(echo "$o" | ansof)
  o5=$(MAXREGION=cech "$C5" "$fa" "$fb" fut_lmf 2>/dev/null)
  b=$(echo "$o5" | ansof)
  o4=$(MAXREGION=cech MAXREGION_DP_LIMIT=4 "$C5" "$fa" "$fb" fut_lmf 2>/dev/null)
  c=$(echo "$o4" | ansof)
  ovf=$(echo "$o4" | grep -ao "overflow=[0-9]*" | tail -1 | cut -d= -f2)
  l=$(MAXREGION=legacy "$C5" "$fa" "$fb" fut_lmf 2>/dev/null | ansof)
  ovf_total=$((ovf_total + ${ovf:-0}))
  printf "%-10s %-24s %-24s %-24s %-24s %s\n" "$pid" "$a" "$b" "$c" "$l" "${ovf:-?}"
  awk -v x="$a" -v y="$b" -v z="$c" 'BEGIN{e=1e-7; if((x-y)^2>e*e || (x-z)^2>e*e) exit 1}' || bad=$((bad+1))
  [ "$a" = "$l" ] || badl=$((badl+1))
done
echo
echo "pairs=$((i-1))  over-eps(1e-7) vs A0 = $bad   C5L-not-bit-identical = $badl   total overflow components = $ovf_total"
