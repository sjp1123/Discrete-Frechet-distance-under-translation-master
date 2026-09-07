#!/bin/bash
SRC="/mnt/c/Users/shiju/Downloads/Discrete-Frechet-distance-under-translation-master/Discrete-Frechet-distance-under-translation-master/original/test_data/benchmark/Geolife Trajectories 1.3/data"
DST="$HOME/geodata"
mkdir -p "$DST"
cp -r "$SRC/." "$DST/"
echo "copied files: $(ls "$DST" | wc -l)  size: $(du -sh "$DST" | cut -f1)"
