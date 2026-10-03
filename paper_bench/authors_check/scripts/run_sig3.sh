#!/bin/bash
# Sigspatial LMF (authors' 1,000 decider pairs), the arms per pair back-to-back on CPU $CPU, rotating order.
# original skipped on pairs 125/432 (>12 GB, experiment_log 6.9); gitlab tried there under a 5 GB address-space cap.
#   ARMS="gitlab original candidate7" (default) or e.g. ARMS="gitlab candidate7"; binaries GITLAB/ORIG/C7 (see arms.sh)
#   bash paper_bench/authors_check/scripts/run_sig3.sh        (W=~/sig3 CPU=0)
source "$(dirname "${BASH_SOURCE[0]}")/arms.sh"
P=$ROOT/paper_bench/queries/sigspatial_pairs.txt; D=$ROOT/paper_data/sigspatial/data
W=${W:-$HOME/sig3}; CPU=${CPU:-0}
mkdir -p $W/pairs; for a in "${ARM_LIST[@]}"; do mkdir -p $W/$a; done
echo "start $(date -Is) cpu $CPU arms ${ARM_LIST[*]}" >> $W/log.txt
run() { local arm=$1 id=$2
  if [ $arm = gitlab ]; then
    ( ulimit -v 5000000; timeout 1800 taskset -c $CPU $(arm_bin $arm) lmf $W/pairs/$id.txt $D $W/$arm/$id.csv > /dev/null 2>> $W/stderr.log )
  else
    env $(arm_env $arm) timeout 1800 taskset -c $CPU $(arm_bin $arm) lmf $W/pairs/$id.txt $D $W/$arm/$id.csv > /dev/null 2>> $W/stderr.log
  fi
  echo "$id $arm rc=$?" >> $W/rc.txt; }
i=0
while read -r f1 f2; do
  [ -z "$f1" ] && continue
  i=$((i+1)); id=$(printf "s%04d" $i); printf "%s %s\n" "$f1" "$f2" > $W/pairs/$id.txt
  for a in $(arm_order $i); do
    if [ $a = original ] && { [ $i = 125 ] || [ $i = 432 ]; }; then echo "$id original skipped(>12GB)" >> $W/rc.txt; continue; fi
    run $a $id
  done
  echo "$(date +%s) $id" >> $W/progress.txt
done < $P
echo "all done $(date -Is)" >> $W/log.txt; touch $W/ALL_DONE
