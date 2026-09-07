#!/bin/bash
# Launch the resumable geolife chunk DETACHED so the wsl.exe call returns immediately
# and the benchmark keeps running inside WSL. Poll _wall_geo.raw for progress.
LOG="/mnt/c/Users/shiju/Downloads/Discrete-Frechet-distance-under-translation-master/Discrete-Frechet-distance-under-translation-master/_geo_full.log"
CHUNK="${1:-30}"
setsid bash "/mnt/c/Users/shiju/Downloads/Discrete-Frechet-distance-under-translation-master/Discrete-Frechet-distance-under-translation-master/_geo_chunk.sh" "$CHUNK" > "$LOG" 2>&1 < /dev/null &
echo "LAUNCHED pid=$! chunk=$CHUNK at $(date)"
