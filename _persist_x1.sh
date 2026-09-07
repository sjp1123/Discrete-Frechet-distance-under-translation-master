#!/bin/bash
# Copy X1 measurement artifacts from WSL-native ~/_x1 back into the repo (/mnt/c)
# so they persist and can be inspected from Windows.
ROOT="/mnt/c/Users/shiju/Downloads/Discrete-Frechet-distance-under-translation-master/Discrete-Frechet-distance-under-translation-master"
DEST="$ROOT/_x1_results"; mkdir -p "$DEST"
cp -f "$HOME/_x1/perpair.csv" "$DEST/x1_perpair_geolife100.csv" 2>/dev/null
python3 "$ROOT/_x1_analyze.py" > "$DEST/x1_analysis.txt" 2>&1
echo "persisted to $DEST:"; ls -la "$DEST"
