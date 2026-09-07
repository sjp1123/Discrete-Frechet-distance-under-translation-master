#!/bin/bash
cd /home/sj10132/FRECHET_EVOLVE2
O=./original/build/calc_frechet_distance_under_translation
C2=./candidate2/build/calc_frechet_distance_under_translation
C3=./candidate3/build/calc_frechet_distance_under_translation
getd() { grep 'LMF' | grep 'is:' | sed 's/.*is: //'; }

check() {
  local A="$1" B="$2" name="$3"
  od=$($O "$A" "$B" fut_lmf 2>/dev/null | getd)
  c2=$($C2 "$A" "$B" fut_lmf 2>/dev/null | getd)
  c3=$($C3 "$A" "$B" fut_lmf 2>/dev/null | getd)
  awk -v o="$od" -v c2="$c2" -v c3="$c3" -v nm="$name" 'BEGIN{
    d2=o-c2; if(d2<0)d2=-d2; d3=o-c3; if(d3<0)d3=-d3;
    printf "%-14s orig=%s  c2|d|=%.1e  c3|d|=%.1e  %s\n", nm, o, d2, d3,
           ((d2<=1e-7 && d3<=1e-7)?"OK":"*** CHECK ***");
  }'
}

check test_cases/bench_n_sweep/n0100_p1_a.txt test_cases/bench_n_sweep/n0100_p1_b.txt "n0100_p1"
for p in 000 001 002 003 004; do
  check test_cases/bench_large/pair${p}_a.txt test_cases/bench_large/pair${p}_b.txt "bench_$p"
done
DATA="original/test_data/benchmark/Geolife Trajectories 1.3/data"
head -4 test_cases/geolife_small/manifest.txt | while read nrep f1 f2; do
  check "$DATA/$f1" "$DATA/$f2" "geo_${f1%.txt}"
done
