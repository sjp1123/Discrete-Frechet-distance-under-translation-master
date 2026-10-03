#!/bin/bash
# Decider 4^l (paperq4) sets, the arms per query file back-to-back on CPU $CPU, rotating order.
#   ARMS="gitlab original candidate7" (default) or e.g. ARMS="gitlab candidate7"; binaries GITLAB/ORIG/C7 (see arms.sh)
#   bash paper_bench/authors_check/scripts/run_dec3.sh        (W=~/dec3 CPU=1)
source "$(dirname "${BASH_SOURCE[0]}")/arms.sh"
Q=$ROOT/paper_bench/queries; CH=$ROOT/paper_data/characters_uci/data; SG=$ROOT/paper_data/sigspatial/data
W=${W:-$HOME/dec3}; CPU=${CPU:-1}
for a in "${ARM_LIST[@]}"; do mkdir -p $W/$a; done
echo "start $(date -Is) cpu $CPU arms ${ARM_LIST[*]}" >> $W/log.txt
run() { local arm=$1 name=$2 dir=$3
  env $(arm_env $arm) timeout 3600 taskset -c $CPU $(arm_bin $arm) decider $Q/$name.txt $dir $W/$arm/$name.csv > /dev/null 2>> $W/stderr.log
  echo "$name $arm rc=$?" >> $W/rc.txt; }
n=0
for set in characters_uci_same characters_uci_all sigspatial; do
  dir=$CH; [ $set = sigspatial ] && dir=$SG
  for ls in "2 plus" "1 plus" "0 plus" "-1 plus" "-2 plus" "-3 plus" "-4 plus" "-5 plus" "-6 plus" "-7 plus" "-8 plus" "-9 plus" "-10 plus" \
            "-1 minus" "-2 minus" "-3 minus" "-4 minus" "-5 minus" "-6 minus" "-7 minus" "-8 minus" "-9 minus" "-10 minus"; do
    set -- $ls; name=${set}_paperq4_$1_$2
    for a in $(arm_order $n); do run $a $name $dir; done
    n=$((n+1)); echo "$(date +%s) $n $name" >> $W/progress.txt
  done
done
echo "all done $(date -Is)" >> $W/log.txt; touch $W/ALL_DONE
