#!/bin/bash
# r8 counting run (not timed): candidate8 with base-case counters (patches/candidate8_count.patch) on the
# Characters 21,000 pairs (CPU 0) and the Sigspatial 1,000 pairs (CPU 1) in parallel.  Every base-case
# decision appends "m V K1 K K0 ovf ovf0 calls_c8 calls_A yes" to $W/<bench>/<job>/<pid>.txt
# (meaning: fut_n6_algorithm.cpp of the patch).  Values are candidate8's (checked against r7 in analysis).
#     bash paper_bench/r8_ablation/scripts/run_count.sh          (W=~/r8/count)
set -u
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"; ROOT="$(cd "$HERE/../../.." && pwd)"
CNT=${CNT:-$HOME/b_pb_candidate8_count/paper_bench}; W=${W:-$HOME/r8/count}
ENVV="MAXREGION=cech MAXREGION_EXACT=1 MAXREGION_SLACK=budget"
mkdir -p $W/chars $W/sig; echo "start $(date -Is) $CNT" >> $W/log.txt
chars() {
  mkdir -p $W/chars/chunks; split -l 100 -d -a 3 --additional-suffix=.txt $ROOT/paper_bench/queries/characters_uci_lmf_pairs.txt $W/chars/chunks/c
  for f in $W/chars/chunks/c*.txt; do id=$(basename $f .txt); mkdir -p $W/chars/$id
    env $ENVV COUNT_LOG=$W/chars/$id taskset -c 0 $CNT lmf $f $ROOT/paper_data/characters_uci/data $W/chars/$id.csv > /dev/null 2>> $W/chars/stderr.log
    echo "$id rc=$?" >> $W/chars/rc.txt; done
  echo "chars done $(date -Is)" >> $W/log.txt; }
sig() {
  mkdir -p $W/sig/pairs; i=0
  while read -r f1 f2; do [ -z "$f1" ] && continue; i=$((i+1)); id=$(printf "s%04d" $i); printf "%s %s\n" "$f1" "$f2" > $W/sig/pairs/$id.txt; mkdir -p $W/sig/$id
    ( ulimit -v 5000000; env $ENVV COUNT_LOG=$W/sig/$id timeout 1800 taskset -c 1 $CNT lmf $W/sig/pairs/$id.txt $ROOT/paper_data/sigspatial/data $W/sig/$id.csv > /dev/null 2>> $W/sig/stderr.log )
    echo "$id rc=$?" >> $W/sig/rc.txt
  done < $ROOT/paper_bench/queries/sigspatial_pairs.txt
  echo "sig done $(date -Is)" >> $W/log.txt; }
chars & sig & wait
echo "all done $(date -Is)" >> $W/log.txt; touch $W/ALL_DONE
