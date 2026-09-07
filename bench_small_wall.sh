#!/bin/bash
cd /home/sj10132/FRECHET_EVOLVE2
ORIG=./original/build/calc_frechet_distance_under_translation
CAND=./candidate2/build/calc_frechet_distance_under_translation
DIR=test_cases/bench_small
so=0; sc=0
while read tag n; do
  A=$DIR/${tag}_a.txt; B=$DIR/${tag}_b.txt
  s=$(date +%s.%N); $ORIG "$A" "$B" fut_lmf >/dev/null 2>&1; e=$(date +%s.%N)
  so=$(awk -v a="$so" -v s="$s" -v e="$e" 'BEGIN{print a+(e-s)*1000}')
  s=$(date +%s.%N); $CAND "$A" "$B" fut_lmf >/dev/null 2>&1; e=$(date +%s.%N)
  sc=$(awk -v a="$sc" -v s="$s" -v e="$e" 'BEGIN{print a+(e-s)*1000}')
done < $DIR/manifest.txt
awk -v o="$so" -v c="$sc" 'BEGIN{printf "WALL-CLOCK total (100 pairs): orig=%.0f ms  cand2=%.0f ms  speedup=%.2fx\n", o, c, o/c}'
