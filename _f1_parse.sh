#!/bin/bash
# F1 v2: run ONE pair under CGAL_PROFILE, capture full stderr, and parse the
# Lazy_kernel DAG-depth histogram to a clean interval-success vs exact-fallback
# split. Depth 0 == resolved by the double interval filter; depth >=1 == the
# lazy kernel had to build/evaluate an exact (Gmpq) DAG == "filter failure".
ROOT="/mnt/c/Users/shiju/Downloads/Discrete-Frechet-distance-under-translation-master/Discrete-Frechet-distance-under-translation-master"
BIN="$HOME/b_original_prof/calc_frechet_distance_under_translation"
DATA="$HOME/geodata"
DEST="$ROOT/_x1_results"; mkdir -p "$DEST"
# one representative geolife pair (first in manifest)
read -r nrep f1 f2 < "$ROOT/test_cases/geolife_100/manifest.txt"
RAW="$DEST/f1_raw_${f1%.txt}_${f2%.txt}.txt"
"$BIN" "$DATA/$f1" "$DATA/$f2" fut_lmf > /dev/null 2> "$RAW"
echo "pair: $f1 + $f2   raw stderr -> $RAW ($(wc -l < "$RAW") lines)"
echo
echo "=== raw 'Lazy_kernel DAG depths' lines ==="
grep -n "Lazy_kernel DAG depths" "$RAW"
echo
echo "=== Profile_counter call totals (top 15 by count) ==="
grep "Profile_counter" "$RAW" | sed -E 's/ \[with .*//' | awk '{print}' | head -15
