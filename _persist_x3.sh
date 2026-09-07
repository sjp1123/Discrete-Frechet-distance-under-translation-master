#!/bin/bash
ROOT="/mnt/c/Users/shiju/Downloads/Discrete-Frechet-distance-under-translation-master/Discrete-Frechet-distance-under-translation-master"
D="$ROOT/_x1_results"; mkdir -p "$D"
cp -f "$HOME/_x1/x3a_sweep.csv" "$D/x3a_sweep.csv" 2>/dev/null
cp -f "$HOME/_x1/x3q.csv"       "$D/x3q_queries.csv" 2>/dev/null
[ -f "$HOME/_x1/x3b.csv" ] && cp -f "$HOME/_x1/x3b.csv" "$D/x3b.csv"
python3 "$ROOT/_x3_analyze.py"  > "$D/x3a_analysis.txt"  2>&1
python3 "$ROOT/_x3q_analyze.py" > "$D/x3q_analysis.txt" 2>&1
[ -f "$HOME/_x1/x3b.csv" ] && python3 "$ROOT/_x3b_analyze.py" > "$D/x3b_analysis.txt" 2>&1
echo "=== _x1_results ==="; ls "$D" | sort
