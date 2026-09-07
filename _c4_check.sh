#!/bin/bash
# Independent verification of candidate4 vs candidate2 / original (A0):
#  (1) answer agreement within eps=1e-7 over geolife pairs (cech mode = default)
#  (2) MAXREGION=legacy reproduces candidate2 answers exactly
#  (3) MAXREGION=verify family cross-check (cech_only / legacy_only) on a pair
ROOT="/mnt/c/Users/shiju/Downloads/Discrete-Frechet-distance-under-translation-master/Discrete-Frechet-distance-under-translation-master"
DATA="$HOME/geodata"; MAN="$ROOT/test_cases/geolife_100/manifest.txt"
C4="$HOME/b_candidate4/calc_frechet_distance_under_translation"
C2="$HOME/b_candidate2/calc_frechet_distance_under_translation"
A0="$HOME/b_original/calc_frechet_distance_under_translation"
LIMIT="${1:-30}"
ans(){ grep -E "calcDistance2 \(LMF\)" | tail -1 | grep -oE "[0-9]+\.[0-9]+" | tail -1; }

echo "=== (1)+(2) answer agreement, $LIMIT pairs (eps=1e-7) ==="
printf "%-16s %-16s %-16s %-10s %-10s\n" pair c4_cech c2 "|c4-c2|" "|c4-A0|"
n=0; over_c2=0; over_a0=0; max_c2=0; max_a0=0; legacy_mismatch=0
while read -r nrep f1 f2; do
  [ -f "$DATA/$f1" ] && [ -f "$DATA/$f2" ] || continue
  n=$((n+1)); [ "$n" -gt "$LIMIT" ] && break
  a4=$( MAXREGION=cech   "$C4" "$DATA/$f1" "$DATA/$f2" fut_lmf 2>/dev/null | ans)
  a2=$(                   "$C2" "$DATA/$f1" "$DATA/$f2" fut_lmf 2>/dev/null | ans)
  aa=$(                   "$A0" "$DATA/$f1" "$DATA/$f2" fut_lmf 2>/dev/null | ans)
  al=$( MAXREGION=legacy "$C4" "$DATA/$f1" "$DATA/$f2" fut_lmf 2>/dev/null | ans)
  d2=$(awk -v x="$a4" -v y="$a2" 'BEGIN{d=x-y;if(d<0)d=-d;printf "%.3e",d}')
  da=$(awk -v x="$a4" -v y="$aa" 'BEGIN{d=x-y;if(d<0)d=-d;printf "%.3e",d}')
  awk -v d="$d2" 'BEGIN{exit !(d>1e-7)}' && over_c2=$((over_c2+1))
  awk -v d="$da" 'BEGIN{exit !(d>1e-7)}' && over_a0=$((over_a0+1))
  awk -v d="$d2" -v m="$max_c2" 'BEGIN{exit !(d>m)}' && max_c2=$d2
  awk -v d="$da" -v m="$max_a0" 'BEGIN{exit !(d>m)}' && max_a0=$da
  [ "$al" = "$a2" ] || { legacy_mismatch=$((legacy_mismatch+1)); }
  if [ $n -le 12 ]; then printf "%-16s %-16.10s %-16.10s %-10s %-10s\n" "${f1%.txt}_${f2%.txt}" "$a4" "$a2" "$d2" "$da"; fi
done < "$MAN"
echo "..."
echo "pairs=$n  c4-vs-c2 over eps=$over_c2 (max |Δ|=$max_c2)  c4-vs-A0 over eps=$over_a0 (max |Δ|=$max_a0)"
echo "MAXREGION=legacy != c2 answer: $legacy_mismatch pairs (want 0)"

echo; echo "=== (3) MAXREGION=verify family cross-check on 13962/4238 ==="
MAXREGION=verify MAXREGION_VERIFY_ABORT= "$C4" "$DATA/13962.txt" "$DATA/4238.txt" fut_lmf 2>&1 \
  | grep -iE "verify|cech_only|legacy_only|maxregion-stats|mismatch" | head -20
