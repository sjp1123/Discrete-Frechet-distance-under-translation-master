#!/bin/bash
ROOT="/mnt/c/Users/shiju/Downloads/Discrete-Frechet-distance-under-translation-master/Discrete-Frechet-distance-under-translation-master"
D="$ROOT/_x1_results"; mkdir -p "$D"
python3 "$ROOT/_reanalyze.py"  > "$D/r1_r2_r3_analysis.txt" 2>&1
python3 "$ROOT/_r4_analyze.py" > "$D/r4_analysis.txt" 2>&1
python3 "$ROOT/_r56_analyze.py" > "$D/r56_analysis.txt" 2>&1
cp -f "$HOME/_x1/x3q.csv" "$D/x3q_queries.csv" 2>/dev/null
cp -f "$HOME/_x1/x3a_sweep.csv" "$D/x3a_sweep.csv" 2>/dev/null
echo "persisted R1-R6:"; ls -la "$D"/r*_analysis.txt "$D/x3q_queries.csv" "$D/x3a_sweep.csv"
