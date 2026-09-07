#!/bin/bash
# X3-a scaling sweep + X5 degeneracy: run n6 --maximal=exact on matched-n
# geolife (degenerate) and synthetic (general position) pairs. Each run appends
# per-build rows tagged by X3_PATH=dataset:n:pair; analysis takes the converged
# (last) row per tag.
ROOT="/mnt/c/Users/shiju/Downloads/Discrete-Frechet-distance-under-translation-master/Discrete-Frechet-distance-under-translation-master"
C2="$HOME/b_candidate2/calc_frechet_distance_under_translation"
W="$HOME/_x3work"
mkdir -p "$HOME/_x1"
DUMP="$HOME/_x1/x3a_sweep.csv"; rm -f "$DUMP"
NS="${1:-8 12 16 20}"
PAIRS="${2:-6}"
TO="${3:-240}"
for ds in geo syn; do
  for n in $NS; do
    nn=$(printf "%02d" "$n")
    for ((p=0;p<PAIRS;p++)); do
      A="$W/${ds}_n${nn}_p${p}_a.txt"; B="$W/${ds}_n${nn}_p${p}_b.txt"
      [ -f "$A" ] && [ -f "$B" ] || continue
      MAXIMAL_MODE=exact X3_DUMP="$DUMP" X3_PATH="${ds}:n${nn}:p${p}" \
        timeout "${TO}s" "$C2" "$A" "$B" n6 >/dev/null 2>&1
      echo "  ran ${ds} n=${n} p=${p} rc=$?"
    done
  done
done
echo "sweep dump: $DUMP ($(($(wc -l < "$DUMP")-1)) rows)"
