#!/bin/bash
# P0-a: repeated-point multiplicity in the E6 pathological pair (4547,10167),
# and a broader scan of geolife_100 files for stationary/repeated vertices.
D="$HOME/geodata"
for f in 4547.txt 10167.txt 10186.txt; do
  [ -f "$D/$f" ] || { echo "$f: MISSING"; continue; }
  n=$(wc -l < "$D/$f")
  dl=$(sort "$D/$f" | uniq -d | wc -l)          # distinct lines that repeat
  top=$(sort "$D/$f" | uniq -cd | sort -rn | head -3 | tr '\n' '|')
  echo "$f: lines=$n  distinct_repeated=$dl  top_multiplicities=[$top]"
done
echo "--- how many geolife_100 files contain ANY repeated point? ---"
cnt=0; tot=0
for f in "$D"/*.txt; do
  tot=$((tot+1))
  if [ -n "$(sort "$f" | uniq -d | head -1)" ]; then cnt=$((cnt+1)); fi
done
echo "files_with_repeats=$cnt / $tot"
