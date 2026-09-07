#!/bin/bash
# X1 switch validation: same pair, three MAXIMAL_MODE values, main path fut_lmf.
# Checks: (a) answer identical across modes, (b) on: K<m, off/mask-only: K==m,
# (c) off skips mask pass (mask_ms ~0, degen 0).
C2="$HOME/b_candidate2/calc_frechet_distance_under_translation"
F1="${1:-$HOME/geodata/10167.txt}"
F2="${2:-$HOME/geodata/10186.txt}"
ALG="${3:-fut_lmf}"
OUT="$HOME/_x1"; mkdir -p "$OUT"
echo "pair: $(basename "$F1") + $(basename "$F2")   alg=$ALG"
for mode in on off mask-only; do
  MAXIMAL_MODE="$mode" "$C2" "$F1" "$F2" "$ALG" > "$OUT/$mode.txt" 2>/dev/null
  ans=$(grep -E "calcDistance2 \(LMF\)|for N6 is" "$OUT/$mode.txt" | tail -1)
  stats=$(grep -E "\[sweep-stats\]" "$OUT/$mode.txt" | tail -1)
  frec=$(grep -E "\[frec-stats\]" "$OUT/$mode.txt" | tail -1)
  printf "== %-9s ==\n  ans   : %s\n  sweep : %s\n  frec  : %s\n" \
         "$mode" "${ans:-<none>}" "${stats:-<none>}" "${frec:-<none>}"
done
