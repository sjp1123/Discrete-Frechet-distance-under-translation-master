#!/bin/bash
# C_q–ply run (NEXT §3.4): fut_lmf over geolife pairs with X3Q_DUMP set. Emits one
# row per decider query: ply, t_query_ns, is_maximal. Run ALONE (timing-sensitive).
ROOT="/mnt/c/Users/shiju/Downloads/Discrete-Frechet-distance-under-translation-master/Discrete-Frechet-distance-under-translation-master"
C2="$HOME/b_candidate2/calc_frechet_distance_under_translation"
DATA="$HOME/geodata"
MAN="$ROOT/test_cases/geolife_100/manifest.txt"
LIMIT="${1:-12}"
OUT="$HOME/_x1/x3q.csv"; mkdir -p "$HOME/_x1"; rm -f "$OUT"
n=0
while read -r nrep f1 f2; do
  [ -f "$DATA/$f1" ] && [ -f "$DATA/$f2" ] || continue
  n=$((n+1)); [ "$n" -gt "$LIMIT" ] && break
  X3Q_DUMP="$OUT" timeout 120s "$C2" "$DATA/$f1" "$DATA/$f2" fut_lmf >/dev/null 2>&1
done < "$MAN"
echo "wrote $OUT ($(($(wc -l < "$OUT")-1)) query rows from $n pairs)"
