#!/bin/bash
ROOT="/mnt/c/Users/shiju/Downloads/Discrete-Frechet-distance-under-translation-master/Discrete-Frechet-distance-under-translation-master"
echo "=== ~/geodata: $(ls "$HOME/geodata" | wc -l) files ==="
echo "=== test_cases/ ==="; ls "$ROOT/test_cases/" 2>&1
echo "=== manifests found ==="
find "$ROOT/test_cases" -name manifest.txt 2>/dev/null | while read -r mf; do
  echo "--- $mf ($(wc -l < "$mf") lines) ---"; head -2 "$mf"
done
echo "=== do ~/geodata files match geolife_small manifest? ==="
MS="$ROOT/test_cases/geolife_small/manifest.txt"
if [ -f "$MS" ]; then
  hit=0; miss=0
  while read -r nrep f1 f2; do
    if [ -f "$HOME/geodata/$f1" ] && [ -f "$HOME/geodata/$f2" ]; then hit=$((hit+1)); else miss=$((miss+1)); fi
  done < "$MS"
  echo "  geolife_small: present_pairs=$hit  missing_pairs=$miss"
fi
echo "=== do ~/geodata files match geolife_100 manifest? ==="
M1="$ROOT/test_cases/geolife_100/manifest.txt"
if [ -f "$M1" ]; then
  hit=0; miss=0
  while read -r nrep f1 f2; do
    if [ -f "$HOME/geodata/$f1" ] && [ -f "$HOME/geodata/$f2" ]; then hit=$((hit+1)); else miss=$((miss+1)); fi
  done < "$M1"
  echo "  geolife_100: present_pairs=$hit  missing_pairs=$miss"
fi
echo "=== full geolife data dir on /mnt/c ==="
D="$ROOT/original/test_data/benchmark/Geolife Trajectories 1.3/data"
if [ -d "$D" ]; then echo "  EXISTS: $(ls "$D" | wc -l) files"; else echo "  NOT at: $D"; fi
find "$ROOT" -maxdepth 4 -type d -name data 2>/dev/null | head
