#!/bin/bash
# Cross-check A0 (Epeck exact), D1 (candidate2), C4 (candidate4) against the
# independent brute-force oracle on small synthetic pairs.
ROOT="/mnt/c/Users/shiju/Downloads/Discrete-Frechet-distance-under-translation-master/Discrete-Frechet-distance-under-translation-master"
A0="$HOME/b_original/calc_frechet_distance_under_translation"
C2="$HOME/b_candidate2/calc_frechet_distance_under_translation"
C4="$HOME/b_candidate4/calc_frechet_distance_under_translation"
ans(){ grep -E "for N6 is|calcDistance2 \(LMF\)" | tail -1 | grep -oE "[0-9]+\.[0-9]+" | tail -1; }
printf "%-14s %-14s %-14s %-14s %-14s %-12s\n" pair A0 D1 C4 oracle "max|Δ|_grid"
for tag in n0006_p0 n0006_p1 n0008_p0 n0008_p1; do
  A="$ROOT/_nsweep/${tag}_a.txt"; B="$ROOT/_nsweep/${tag}_b.txt"
  [ -f "$A" ] && [ -f "$B" ] || continue
  a=$( "$A0" "$A" "$B" fut_lmf 2>/dev/null | ans)
  d=$( "$C2" "$A" "$B" fut_lmf 2>/dev/null | ans)
  c=$( MAXREGION=cech "$C4" "$A" "$B" fut_lmf 2>/dev/null | ans)
  o=$(python3 "$ROOT/_oracle.py" "$A" "$B")
  md=$(awk -v a="$a" -v d="$d" -v c="$c" -v o="$o" 'BEGIN{
    m=0; for(i=1;i<=3;i++){};
    x[1]=a-o; x[2]=d-o; x[3]=c-o; for(i=1;i<=3;i++){v=x[i]; if(v<0)v=-v; if(v>m)m=v}; printf "%.2e", m}')
  printf "%-14s %-14.9s %-14.9s %-14.9s %-14.9s %-12s\n" "$tag" "$a" "$d" "$c" "$o" "$md"
done
