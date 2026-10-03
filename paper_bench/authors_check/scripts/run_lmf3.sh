#!/bin/bash
# Characters LMF (21,000 pairs, 210 chunks of 100), the arms back-to-back per chunk on CPU $CPU, order rotating with chunk number.
#   ARMS="gitlab original candidate7" (default) or e.g. ARMS="gitlab candidate7"; binaries GITLAB/ORIG/C7 (see arms.sh)
#   bash paper_bench/authors_check/scripts/run_lmf3.sh        (W=~/lmf3 CPU=1)
source "$(dirname "${BASH_SOURCE[0]}")/arms.sh"
P=$ROOT/paper_bench/queries/characters_uci_lmf_pairs.txt; D=$ROOT/paper_data/characters_uci/data
W=${W:-$HOME/lmf3}; CPU=${CPU:-1}
mkdir -p $W/chunks; for a in "${ARM_LIST[@]}"; do mkdir -p $W/$a; done
split -l 100 -d -a 3 --additional-suffix=.txt $P $W/chunks/c
echo "start $(date -Is) cpu $CPU arms ${ARM_LIST[*]}" >> $W/log.txt
run() { local arm=$1 id=$2 f=$3
  env $(arm_env $arm) taskset -c $CPU $(arm_bin $arm) lmf $f $D $W/$arm/$id.csv > /dev/null 2>> $W/stderr.log
  echo "$id $arm rc=$?" >> $W/rc.txt; }
for f in $W/chunks/c*.txt; do id=$(basename $f .txt); n=$((10#${id#c}))
  for a in $(arm_order $n); do run $a $id $f; done
  echo "$(date +%s) $id" >> $W/progress.txt
done
echo "all done $(date -Is)" >> $W/log.txt; touch $W/ALL_DONE
