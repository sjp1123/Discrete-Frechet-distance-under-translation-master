#!/bin/bash
SRC="/mnt/c/Users/shiju/Downloads/Discrete-Frechet-distance-under-translation-master/Discrete-Frechet-distance-under-translation-master/original/test_data/benchmark/Geolife Trajectories 1.3/data"
MAN="/mnt/c/Users/shiju/Downloads/Discrete-Frechet-distance-under-translation-master/Discrete-Frechet-distance-under-translation-master/test_cases/geolife_100/manifest.txt"
DST="$HOME/geodata"
rm -rf "$DST"; mkdir -p "$DST"
n=0
while read -r nrep f1 f2; do
  for f in "$f1" "$f2"; do
    [ -f "$DST/$f" ] && continue
    if [ -f "$SRC/$f" ]; then cp "$SRC/$f" "$DST/$f" && n=$((n+1)); fi
  done
done < "$MAN"
echo "copied $n files; DST has $(ls "$DST"/*.txt 2>/dev/null | wc -l) .txt"
