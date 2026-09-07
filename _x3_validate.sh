#!/bin/bash
# Validate --maximal=exact on the n6 path (its intended path): answer must match
# on/off (decider uses band-on set), and the CSV counts must be sane
# (K_exact_distinct <= K_band_distinct <= K_band; degeneracy detected).
ROOT="/mnt/c/Users/shiju/Downloads/Discrete-Frechet-distance-under-translation-master/Discrete-Frechet-distance-under-translation-master"
C2="$HOME/b_candidate2/calc_frechet_distance_under_translation"
NS="$ROOT/_nsweep"
OUT="$HOME/_x1"; mkdir -p "$OUT"
ansof(){ grep -E "for N6 is" | tail -1 | grep -oE "[0-9]+\.[0-9]+" | tail -1; }

echo "=== answer match (n6 synthetic, general position) ==="
for base in n0008_p0 n0012_p0 n0016_p0; do
  a_on=$( MAXIMAL_MODE=on    "$C2" "$NS/${base}_a.txt" "$NS/${base}_b.txt" n6 2>/dev/null | ansof)
  a_off=$(MAXIMAL_MODE=off   "$C2" "$NS/${base}_a.txt" "$NS/${base}_b.txt" n6 2>/dev/null | ansof)
  X3_DUMP="$OUT/x3_val.csv" X3_PATH="n6:$base" \
  a_ex=$( MAXIMAL_MODE=exact "$C2" "$NS/${base}_a.txt" "$NS/${base}_b.txt" n6 2>/dev/null | ansof)
  m="OK"; [ "$a_on" = "$a_ex" ] && [ "$a_off" = "$a_ex" ] || m="** MISMATCH **"
  echo "  $base: on=$a_on off=$a_off exact=$a_ex  $m"
done

echo "=== CSV counts (fresh) ==="
rm -f "$OUT/x3_val.csv"
for base in n0008_p0 n0012_p0 n0016_p0; do
  X3_DUMP="$OUT/x3_val.csv" X3_PATH="n6:$base" \
    MAXIMAL_MODE=exact "$C2" "$NS/${base}_a.txt" "$NS/${base}_b.txt" n6 >/dev/null 2>&1
done
head -1 "$OUT/x3_val.csv"
awk -F, 'NR>1{print "  "$2" N="$3" V_pairs="$4" m="$5" K_band="$6" K_band_distinct="$7" K_exact="$8" K_exact_distinct="$9" degen_geom="$11" t_exact_ms="$12}
  NR>1{ if($9>$7) bad++; if($7>$6) bad++ }
  END{ print (bad? "  ** invariant violated (K_exact_distinct>K_band_distinct or K_band_distinct>K_band) **":"  invariants OK: K_exact_distinct<=K_band_distinct<=K_band") }' "$OUT/x3_val.csv"
