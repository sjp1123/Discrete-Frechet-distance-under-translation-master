#!/bin/bash
# r4 decision problem ([BKN20] Table 2 format), original vs candidate7, same protocol as the r4 value computation:
# every query file (1,000 queries) runs both arms back-to-back on one pinned CPU, one process per file and arm,
# order alternating with the file number (even: original first).  Leave the other CPU idle.
#   sets: characters_uci_same, characters_uci_all, sigspatial; 23 files each (l = -10..2 plus, -10..-1 minus)
#   tags: paperq4 (factors 1 +- 4^l, the paper's text; the main table) then paperq (1 +- 2^l, the authors' files)
# Sigspatial needs paper_data/sigspatial/data: python3 paper_data/convert_sigspatial.py
#   bash paper_bench/r4_original_vs_candidate7/scripts/run_decider.sh      (W=~/r4dec CPU=1)
#   (cd $W && tar czf <folder>/raw/raw_decider_r4.tar.gz original candidate7 rc.txt log.txt)
set -u
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
Q=$ROOT/paper_bench/queries
CH=$ROOT/paper_data/characters_uci/data; SG=$ROOT/paper_data/sigspatial/data
W=${W:-$HOME/r4dec}; CPU=${CPU:-1}
ORIG=${ORIG:-$HOME/b_pb_original/paper_bench}; C7=${C7:-$HOME/b_pb_candidate7/paper_bench}
mkdir -p $W/original $W/candidate7
echo "start $(date -Is) $(nproc) cpus, pinned to $CPU" >> $W/log.txt
run() { local arm=$1 name=$2 dir=$3
  if [ $arm = original ]; then taskset -c $CPU $ORIG decider $Q/$name.txt $dir $W/original/$name.csv > /dev/null 2>> $W/stderr.log
  else env MAXREGION=cech MAXREGION_EXACT=1 MAXREGION_SLACK=0 taskset -c $CPU $C7 decider $Q/$name.txt $dir $W/candidate7/$name.csv > /dev/null 2>> $W/stderr.log; fi
  echo "$name $arm rc=$?" >> $W/rc.txt; }
n=0
for tag in paperq4 paperq; do
  for set in characters_uci_same characters_uci_all sigspatial; do
    dir=$CH; [ $set = sigspatial ] && dir=$SG
    for ls in "2 plus" "1 plus" "0 plus" "-1 plus" "-2 plus" "-3 plus" "-4 plus" "-5 plus" "-6 plus" "-7 plus" "-8 plus" "-9 plus" "-10 plus" \
              "-1 minus" "-2 minus" "-3 minus" "-4 minus" "-5 minus" "-6 minus" "-7 minus" "-8 minus" "-9 minus" "-10 minus"; do
      set -- $ls; name=${set}_${tag}_$1_$2
      [ -f $Q/$name.txt ] || { echo "missing $name" >> $W/rc.txt; continue; }
      if [ $((n % 2)) = 0 ]; then run original $name $dir; run candidate7 $name $dir; else run candidate7 $name $dir; run original $name $dir; fi
      n=$((n+1)); echo "$(date +%s) $n $name" >> $W/progress.txt
    done
  done
done
echo "all done $(date -Is)" >> $W/log.txt; touch $W/ALL_DONE
