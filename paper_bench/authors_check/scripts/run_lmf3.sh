#!/bin/bash
# Characters LMF (21,000 pairs, 210 chunks), three arms back-to-back per chunk on CPU 1, order rotating with chunk number
R=/home/claude/Discrete-Frechet-distance-under-translation-master; D=$R/paper_data/characters_uci/data
W=/root/lmf3; mkdir -p $W/gitlab $W/original $W/candidate7
echo "start $(date -Is)" >> $W/log.txt
run() { local arm=$1 id=$2 f=$3
  case $arm in
    gitlab) taskset -c 1 /root/b_pb_gitlab/paper_bench lmf $f $D $W/gitlab/$id.csv > /dev/null 2>> $W/stderr.log ;;
    original) taskset -c 1 /root/b_pb_original/paper_bench lmf $f $D $W/original/$id.csv > /dev/null 2>> $W/stderr.log ;;
    candidate7) env MAXREGION=cech MAXREGION_EXACT=1 MAXREGION_SLACK=0 taskset -c 1 /root/b_pb_candidate7/paper_bench lmf $f $D $W/candidate7/$id.csv > /dev/null 2>> $W/stderr.log ;;
  esac
  echo "$id $arm rc=$?" >> $W/rc.txt; }
for f in /root/r4/chunks/c*.txt; do id=$(basename $f .txt); n=$((10#${id#c}))
  case $((n % 6)) in
    0) o="gitlab original candidate7";; 1) o="original candidate7 gitlab";; 2) o="candidate7 gitlab original";;
    3) o="gitlab candidate7 original";; 4) o="original gitlab candidate7";; 5) o="candidate7 original gitlab";;
  esac
  for a in $o; do run $a $id $f; done
  echo "$(date +%s) $id" >> $W/progress.txt
done
echo "all done $(date -Is)" >> $W/log.txt; touch $W/ALL_DONE
