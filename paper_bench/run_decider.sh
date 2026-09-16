#!/bin/bash
# Decider benchmark with the paper's protocol (Sec. 6): for each of the first PAIRS
# pairs of queries/<ds>_pairs.txt, delta* is computed by the ORIGINAL arm at precision
# 1e-7, then 23 query sets are written:
#   NO : (1 - 4^l) (delta* - eps),  l = -10..-1
#   YES: (1 + 4^l) (delta* + eps),  l = -10..2
# Each set is run on both arms, REPS reps, arm order alternating per rep, one core.
# Query files: queries/<ds>_<TAG>_<l>_<plus|minus>.txt ; results: results/<ds>_<TAG>_...
#
#   bash paper_bench/run_decider.sh [REPS] [DATASETS] [PAIRS] [TAG]
set -u
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"; PB="$ROOT/paper_bench"
REPS="${1:-3}"; DATASETS="${2:-characters_all characters_same sigspatial_subset}"
PAIRS="${3:-1000}"; TAG="${4:-decider4}"; CPU="${CPU:-2}"
declare -A BIN=( [original]="$HOME/b_pb_original/paper_bench" [candidate5]="$HOME/b_pb_candidate5/paper_bench" )
declare -A DIR=( [characters_all]="$ROOT/paper_data/characters/data" \
                 [characters_same]="$ROOT/paper_data/characters/data" \
                 [sigspatial_subset]="$ROOT/paper_data/sigspatial_subset/data" )
LS_PLUS="-10 -9 -8 -7 -6 -5 -4 -3 -2 -1 0 1 2"; LS_MINUS="-10 -9 -8 -7 -6 -5 -4 -3 -2 -1"
mkdir -p "$PB/results"
for ds in $DATASETS; do
  dir="${DIR[$ds]}"; qp="$PB/queries/${ds}_${TAG}"
  if [ ! -f "${qp}_computed_distances.check" ]; then
    head -n "$PAIRS" "$PB/queries/${ds}_pairs.txt" > "$PB/queries/${ds}_pairs_first${PAIRS}.txt"
    echo "[$ds] generating $TAG queries (base 4) with original on $PAIRS pairs ..." >&2
    GEN_BASE=4 taskset -c "$CPU" "${BIN[original]}" gen "$PB/queries/${ds}_pairs_first${PAIRS}.txt" "$dir" "$qp" 2>&1 | tail -1 >&2
  fi
  for rep in $(seq 1 "$REPS"); do
    if [ $((rep % 2)) -eq 1 ]; then order="original candidate5"; else order="candidate5 original"; fi
    for arm in $order; do
      t0=$EPOCHREALTIME
      for l in $LS_PLUS;  do taskset -c "$CPU" "${BIN[$arm]}" decider "${qp}_${l}_plus.txt"  "$dir" "$PB/results/${ds}_${TAG}_${l}_plus_${arm}_r${rep}.csv"  2>/dev/null; done
      for l in $LS_MINUS; do taskset -c "$CPU" "${BIN[$arm]}" decider "${qp}_${l}_minus.txt" "$dir" "$PB/results/${ds}_${TAG}_${l}_minus_${arm}_r${rep}.csv" 2>/dev/null; done
      echo "[$ds] $TAG rep $rep $arm done in $(awk -v a="$t0" -v b="$EPOCHREALTIME" 'BEGIN{printf "%.0f", b-a}') s" >&2
    done
  done
done
echo "$TAG all done" >&2
