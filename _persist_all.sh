#!/bin/bash
ROOT="/mnt/c/Users/shiju/Downloads/Discrete-Frechet-distance-under-translation-master/Discrete-Frechet-distance-under-translation-master"
DEST="$ROOT/_x1_results"; mkdir -p "$DEST"
cp -f "$HOME/_x1/perpair.csv"         "$DEST/x1_perpair_geolife100.csv"      2>/dev/null
cp -f "$HOME/_x1/x2_perpair.csv"      "$DEST/x2_k0_perpair_geolife100.csv"   2>/dev/null
cp -f "$HOME/_x1/factor_perpair.csv"  "$DEST/factor_perpair_geolife100.csv"  2>/dev/null
python3 "$ROOT/_x1_analyze.py"      > "$DEST/x1_analysis.txt"     2>&1
python3 "$ROOT/_x2_analyze.py"      > "$DEST/x2_k0_analysis.txt"  2>&1
python3 "$ROOT/_factor_analyze.py"  > "$DEST/factor_analysis.txt" 2>&1
echo "persisted:"; ls -la "$DEST"
