#!/bin/bash
# r4 Characters: the 21,000 characters_full pairs ([BKN20] Table 4), value computation (LMF), original vs candidate7.
# 210 chunks x 100 pairs; per chunk both arms back-to-back on one pinned CPU, one process per chunk and arm,
# order alternating with the chunk number (even: original first).  Leave the other CPU idle.
#   bash paper_bench/r4_original_vs_candidate7/scripts/run_characters.sh      (W=~/r4 CPU=1)
#   (cd $W && tar czf <folder>/raw/raw_characters_lmf_r4.tar.gz original candidate7 rc.txt log.txt)
set -u
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
P=$ROOT/paper_bench/queries/characters_uci_lmf_pairs.txt
D=$ROOT/paper_data/characters_uci/data
W=${W:-$HOME/r4}; CPU=${CPU:-1}
ORIG=${ORIG:-$HOME/b_pb_original/paper_bench}; C7=${C7:-$HOME/b_pb_candidate7/paper_bench}
mkdir -p $W/chunks $W/original $W/candidate7
split -l 100 -d -a 3 --additional-suffix=.txt $P $W/chunks/c
echo "start $(date -Is) $(nproc) cpus, pinned to $CPU" >> $W/log.txt
run() { local arm=$1 id=$2
  if [ $arm = original ]; then taskset -c $CPU $ORIG lmf $W/chunks/$id.txt $D $W/original/$id.csv > /dev/null 2>> $W/stderr.log
  else env MAXREGION=cech MAXREGION_EXACT=1 MAXREGION_SLACK=0 taskset -c $CPU $C7 lmf $W/chunks/$id.txt $D $W/candidate7/$id.csv > /dev/null 2>> $W/stderr.log; fi
  echo "$id $arm rc=$?" >> $W/rc.txt; }
for f in $W/chunks/c*.txt; do id=$(basename $f .txt); n=$((10#${id#c}))
  if [ $((n % 2)) = 0 ]; then run original $id; run candidate7 $id; else run candidate7 $id; run original $id; fi
  echo "$(date +%s) $id" >> $W/progress.txt
done
echo "all done $(date -Is)" >> $W/log.txt; touch $W/ALL_DONE
