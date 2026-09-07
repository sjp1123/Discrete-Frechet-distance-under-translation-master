#!/bin/bash
BENCH="/mnt/c/Users/shiju/Downloads/Discrete-Frechet-distance-under-translation-master/Discrete-Frechet-distance-under-translation-master/_bench_wall_geo.sh"
LOG="/mnt/c/Users/shiju/Downloads/Discrete-Frechet-distance-under-translation-master/Discrete-Frechet-distance-under-translation-master/_geo_run.log"
echo "=== START $(date) ===" > "$LOG"
bash "$BENCH" >> "$LOG" 2>&1
echo "=== BENCH_EXIT=$? $(date) ===" >> "$LOG"
echo "wrapper done"
