#!/bin/bash
# r4 Sigspatial: the authors' 1,000 decider pairs (queries/sigspatial_pairs.txt), value computation (LMF),
# original vs candidate7 on this container.  One process per pair and arm, both arms of a pair back-to-back
# on CPU 1, order alternating with the pair number (even: original first).  The original is skipped on
# pairs 125 and 432 (1-based), which need > 12 GB (experiment_log §6.9); candidate7 runs all 1,000.
# Needs paper_data/sigspatial/data: python3 paper_data/convert_sigspatial.py
#   bash paper_bench/r4_original_vs_candidate7/scripts/run_sigspatial.sh      (W=~/r4sig CPU=1)
#   (cd $W && tar czf <folder>/raw/raw_sigspatial_lmf_r4.tar.gz original candidate7 rc.txt log.txt)
set -u
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
P=$ROOT/paper_bench/queries/sigspatial_pairs.txt
D=$ROOT/paper_data/sigspatial/data
W=${W:-$HOME/r4sig}; CPU=${CPU:-1}
ORIG=${ORIG:-$HOME/b_pb_original/paper_bench}; C7=${C7:-$HOME/b_pb_candidate7/paper_bench}
mkdir -p $W/pairs $W/original $W/candidate7
echo "start $(date -Is) $(nproc) cpus, pinned to $CPU" >> $W/log.txt
run() { local arm=$1 id=$2
  if [ $arm = original ]; then timeout 1800 taskset -c $CPU $ORIG lmf $W/pairs/$id.txt $D $W/original/$id.csv > /dev/null 2>> $W/stderr.log
  else env MAXREGION=cech MAXREGION_EXACT=1 MAXREGION_SLACK=0 timeout 1800 taskset -c $CPU $C7 lmf $W/pairs/$id.txt $D $W/candidate7/$id.csv > /dev/null 2>> $W/stderr.log; fi
  echo "$id $arm rc=$?" >> $W/rc.txt; }
i=0
while read -r f1 f2; do
  [ -z "$f1" ] && continue
  i=$((i+1)); id=$(printf "s%04d" $i)
  printf "%s %s\n" "$f1" "$f2" > $W/pairs/$id.txt
  if [ $i = 125 ] || [ $i = 432 ]; then echo "$id original skipped(>12GB)" >> $W/rc.txt; run candidate7 $id
  elif [ $((i % 2)) = 0 ]; then run original $id; run candidate7 $id
  else run candidate7 $id; run original $id; fi
  echo "$(date +%s) $id" >> $W/progress.txt
done < $P
echo "all done $(date -Is)" >> $W/log.txt; touch $W/ALL_DONE
