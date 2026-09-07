#!/bin/bash
D="$HOME/geodata"
A="$D/10167.txt"; B="$D/4547.txt"
O="$HOME/b_original/calc_frechet_distance_under_translation"
C1="$HOME/b_candidate/calc_frechet_distance_under_translation"
C2="$HOME/b_candidate2/calc_frechet_distance_under_translation"
OUT="/mnt/c/Users/shiju/Downloads/Discrete-Frechet-distance-under-translation-master/Discrete-Frechet-distance-under-translation-master/_pair_diag.txt"
{
  echo "############ ORIGINAL (CGAL, no maximal) ############"
  s=$SECONDS; timeout 60 "$O" "$A" "$B" fut_lmf 2>&1; echo "[orig rc=$? wall=$((SECONDS-s))s]"
  echo
  echo "############ CANDIDATE2 (float, maximal) ############"
  s=$SECONDS; timeout 60 "$C2" "$A" "$B" fut_lmf 2>&1; echo "[cand2 rc=$? wall=$((SECONDS-s))s]"
  echo
  echo "############ CANDIDATE (CGAL, maximal) — up to 150s ############"
  s=$SECONDS; timeout 150 "$C1" "$A" "$B" fut_lmf 2>&1; echo "[cand rc=$? wall=$((SECONDS-s))s]"
} > "$OUT" 2>&1
echo "diag done"
