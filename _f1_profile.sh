#!/bin/bash
# X2 / F1 (experiment_design_X1_X2.md §3.2): measure CGAL lazy-kernel interval
# filter behavior on the Epeck baseline (original). Build with -DCGAL_PROFILE,
# run on a few geolife pairs, dump whatever profile counters CGAL emits at exit.
# The counter names are version-specific, so this first pass just SURFACES them.
set -o pipefail
ROOT="/mnt/c/Users/shiju/Downloads/Discrete-Frechet-distance-under-translation-master/Discrete-Frechet-distance-under-translation-master"
SRC="$ROOT/original"
BUILD="$HOME/b_original_prof"
TGT=calc_frechet_distance_under_translation
DATA="$HOME/geodata"
MAN="$ROOT/test_cases/geolife_100/manifest.txt"
OUT="$HOME/_x1/f1_profile.txt"; mkdir -p "$HOME/_x1"

echo "### F1 configure $(date)"
cmake -S "$SRC" -B "$BUILD" -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_CXX_FLAGS="-include cstdint -include array -include cstddef -DCGAL_PROFILE" \
  > "$HOME/_x1/f1_cfg.log" 2>&1
echo "configure rc=$?"
echo "### F1 build $(date)"
cmake --build "$BUILD" -j"$(nproc)" --target "$TGT" > "$HOME/_x1/f1_build.log" 2>&1
echo "build rc=$?  artifact:"; ls -la "$BUILD/$TGT" 2>&1

BIN="$BUILD/$TGT"
[ -x "$BIN" ] || { echo "no binary, aborting"; exit 1; }

# Run a handful of pairs; CGAL_PROFILE counters print to stderr at process exit.
: > "$OUT"
n=0
while read -r nrep f1 f2; do
  [ -f "$DATA/$f1" ] && [ -f "$DATA/$f2" ] || continue
  echo "===== pair $f1 + $f2 =====" >> "$OUT"
  "$BIN" "$DATA/$f1" "$DATA/$f2" fut_lmf > /dev/null 2>> "$OUT"
  n=$((n+1)); [ "$n" -ge 5 ] && break
done < "$MAN"

echo "### distinct stderr counter lines (grep filter/interval/lazy/exact/failure) ###"
grep -iE "filter|interval|lazy|exact|failure|predicate|construct|profile|counter" "$OUT" \
  | sort | uniq -c | sort -rn | head -60
echo "### full stderr saved to $OUT ($(wc -l < "$OUT") lines) ###"
