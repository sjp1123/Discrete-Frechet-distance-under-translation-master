#!/bin/bash
# X2 §3.1: build original with the Epick kernel switch, then run the correctness
# gate BEFORE any performance claim: answers vs A0 (< 1e-6), crashes, and
# arrangement-build count (n_arr_builds) within +-10% of Epeck (else topology
# breakage -> branch&bound wanders, the E6 symptom).
set -o pipefail
ROOT="/mnt/c/Users/shiju/Downloads/Discrete-Frechet-distance-under-translation-master/Discrete-Frechet-distance-under-translation-master"
SRC="$ROOT/original"
BUILD="$HOME/b_original_epick"
TGT=calc_frechet_distance_under_translation
DATA="$HOME/geodata"
MAN="$ROOT/test_cases/geolife_100/manifest.txt"
DEST="$ROOT/_x1_results"; mkdir -p "$DEST"

echo "### configure+build Epick $(date)"
cmake -S "$SRC" -B "$BUILD" -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_CXX_FLAGS="-include cstdint -include array -include cstddef -DUSE_EPICK" \
  > "$DEST/x2_epick_cfg.log" 2>&1
echo "configure rc=$?"
cmake --build "$BUILD" -j"$(nproc)" --target "$TGT" > "$DEST/x2_epick_build.log" 2>&1
echo "build rc=$?"
ls -la "$BUILD/$TGT" 2>&1 || { echo "BUILD FAILED - see $DEST/x2_epick_build.log"; tail -20 "$DEST/x2_epick_build.log"; exit 1; }

A0="$HOME/b_original/$TGT"     # Epeck baseline (reference answers)
B0="$BUILD/$TGT"              # Epick

ans() { grep -E "calcDistance2 \(LMF\)" | tail -1 | grep -oE "[0-9]+\.[0-9]+" | tail -1; }
# original prints "candidate count = N" per arrangement build -> count them.
nbuilds() { grep -c "candidate count" ; }

echo
echo "### correctness gate: A0(Epeck) vs B0(Epick), first 40 geolife pairs ###"
printf "%-22s %-14s %-14s %-8s %-8s %-8s\n" "pair" "A0_ans" "B0_ans" "|diff|" "A0blds" "B0blds"
n=0; mism=0; crash=0; bsum_a=0; bsum_b=0
while read -r nrep f1 f2; do
  [ -f "$DATA/$f1" ] && [ -f "$DATA/$f2" ] || continue
  n=$((n+1)); [ "$n" -gt 40 ] && break
  oa=$(timeout 60s "$A0" "$DATA/$f1" "$DATA/$f2" fut_lmf 2>/dev/null); ra=$?
  ob=$(timeout 60s "$B0" "$DATA/$f1" "$DATA/$f2" fut_lmf 2>/dev/null); rb=$?
  aa=$(printf '%s\n' "$oa" | ans); ab=$(printf '%s\n' "$ob" | ans)
  ba=$(printf '%s\n' "$oa" | nbuilds); bb=$(printf '%s\n' "$ob" | nbuilds)
  bsum_a=$((bsum_a+ba)); bsum_b=$((bsum_b+bb))
  if [ "$rb" -ne 0 ]; then crash=$((crash+1)); fi
  df=$(awk -v x="$aa" -v y="$ab" 'BEGIN{ if(x==""||y=="") {print "NA"} else {d=x-y; if(d<0)d=-d; printf "%.2e", d} }')
  flag=""
  case "$df" in NA) flag="?";; *) awk -v d="$df" 'BEGIN{exit !(d>1e-6)}' && flag="  <== MISMATCH" && mism=$((mism+1));; esac
  printf "%-22s %-14.9s %-14.9s %-8s %-8s %-8s%s\n" "${f1%.txt}_${f2%.txt}" "$aa" "$ab" "$df" "$ba" "$bb" "$flag"
done < "$MAN"
echo
echo "### GATE SUMMARY: pairs=$n  answer_mismatches(>1e-6)=$mism  epick_crashes=$crash"
echo "    arr_builds  A0(Epeck)=$bsum_a  B0(Epick)=$bsum_b  ratio=$(awk -v a=$bsum_a -v b=$bsum_b 'BEGIN{printf "%.3f", (a? b/a:0)}') (want ~1.0 +-10%)"
