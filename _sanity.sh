#!/bin/bash
ROOT="/mnt/c/Users/shiju/Downloads/Discrete-Frechet-distance-under-translation-master/Discrete-Frechet-distance-under-translation-master"
DATA="$ROOT/original/test_data/benchmark/Geolife Trajectories 1.3/data"
MAN="$ROOT/test_cases/geolife_small/manifest.txt"
echo "=== manifest head ==="; head -3 "$MAN"
read -r nrep f1 f2 < "$MAN"
A="$DATA/$f1"; B="$DATA/$f2"
echo "=== pair: '$f1'  '$f2' ==="
echo "A exists: $([ -f "$A" ] && echo yes || echo NO)"
echo "B exists: $([ -f "$B" ] && echo yes || echo NO)"
echo "=== run original fut_lmf ==="
"$HOME/b_original/calc_frechet_distance_under_translation" "$A" "$B" fut_lmf
