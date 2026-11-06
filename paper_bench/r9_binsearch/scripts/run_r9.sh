#!/bin/bash
# r9: the authors' Binary Search baseline ([BKN20] Figure 6) with its known defects fixed (binsearch_fixes.patch:
# P10 kd-tree lower bound, C depth-limit step, D bisection guard) vs candidate7's LMF, on the 21,000 Characters
# pairs of [BKN20] Table 4.  Same protocol as r5: one pinned CPU ($CPU, default 1), the other CPU idle, both arms
# back-to-back per 100-pair chunk in one process each, order alternating with the chunk number.
#   build:  copy authors_gitlab/ to $A, apply binsearch_fixes.patch, then
#           cmake -S paper_bench -B ~/b_pb_authors_binfix -DARM=$A ... (as authors_check/scripts/build_gitlab_arm.sh)
#   run:    bash paper_bench/r9_binsearch/scripts/run_r9.sh      (W=~/r9, about 2.5 h)
set -u
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
BIN=${BIN:-$HOME/b_pb_authors_binfix/paper_bench}; C7=${C7:-$HOME/b_pb_candidate7/paper_bench}
P=$ROOT/paper_bench/queries/characters_uci_lmf_pairs.txt; D=$ROOT/paper_data/characters_uci/data
W=${W:-$HOME/r9}; CPU=${CPU:-1}
mkdir -p $W/chunks $W/binsearch $W/candidate7
split -l 100 -d -a 3 --additional-suffix=.txt $P $W/chunks/c
echo "start $(date -Is) $(nproc) cpus, pinned to $CPU, $(grep -m1 'model name' /proc/cpuinfo | cut -d: -f2)" >> $W/log.txt
run() { local arm=$1 id=$2 f=$3
  if [ $arm = binsearch ]; then taskset -c $CPU $BIN binsearch $f $D $W/$arm/$id.csv > /dev/null 2>> $W/stderr.log
  else env MAXREGION=cech MAXREGION_EXACT=1 MAXREGION_SLACK=0 taskset -c $CPU $C7 lmf $f $D $W/$arm/$id.csv > /dev/null 2>> $W/stderr.log; fi
  echo "$id $arm rc=$?" >> $W/rc.txt; }
for f in $W/chunks/c*.txt; do id=$(basename $f .txt); n=$((10#${id#c}))
  if [ $((n % 2)) -eq 0 ]; then run binsearch $id $f; run candidate7 $id $f; else run candidate7 $id $f; run binsearch $id $f; fi
  echo "$(date +%s) $id" >> $W/progress.txt
done
echo "all done $(date -Is)" >> $W/log.txt; touch $W/ALL_DONE
