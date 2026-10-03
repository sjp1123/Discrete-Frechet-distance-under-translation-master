#!/bin/bash
# Sigspatial LMF (authors' 1,000 decider pairs), three arms per pair back-to-back on CPU $CPU, rotating order.
# original skipped on pairs 125/432 (>12 GB, experiment_log 6.9); gitlab tried there under a 5 GB address-space cap.
R=/home/claude/Discrete-Frechet-distance-under-translation-master; P=$R/paper_bench/queries/sigspatial_pairs.txt; D=$R/paper_data/sigspatial/data
W=/root/sig3; CPU=${CPU:-0}; mkdir -p $W/pairs $W/gitlab $W/original $W/candidate7
echo "start $(date -Is) cpu $CPU" >> $W/log.txt
run() { local arm=$1 id=$2
  case $arm in
    gitlab) ( ulimit -v 5000000; timeout 1800 taskset -c $CPU /root/b_pb_gitlab/paper_bench lmf $W/pairs/$id.txt $D $W/gitlab/$id.csv > /dev/null 2>> $W/stderr.log ) ;;
    original) timeout 1800 taskset -c $CPU /root/b_pb_original/paper_bench lmf $W/pairs/$id.txt $D $W/original/$id.csv > /dev/null 2>> $W/stderr.log ;;
    candidate7) env MAXREGION=cech MAXREGION_EXACT=1 MAXREGION_SLACK=0 timeout 1800 taskset -c $CPU /root/b_pb_candidate7/paper_bench lmf $W/pairs/$id.txt $D $W/candidate7/$id.csv > /dev/null 2>> $W/stderr.log ;;
  esac
  echo "$id $arm rc=$?" >> $W/rc.txt; }
i=0
while read -r f1 f2; do
  [ -z "$f1" ] && continue
  i=$((i+1)); id=$(printf "s%04d" $i); printf "%s %s\n" "$f1" "$f2" > $W/pairs/$id.txt
  case $((i % 6)) in
    0) o="gitlab original candidate7";; 1) o="original candidate7 gitlab";; 2) o="candidate7 gitlab original";;
    3) o="gitlab candidate7 original";; 4) o="original gitlab candidate7";; 5) o="candidate7 original gitlab";;
  esac
  for a in $o; do
    if [ $a = original ] && { [ $i = 125 ] || [ $i = 432 ]; }; then echo "$id original skipped(>12GB)" >> $W/rc.txt; continue; fi
    run $a $id
  done
  echo "$(date +%s) $id" >> $W/progress.txt
done < $P
echo "all done $(date -Is)" >> $W/log.txt; touch $W/ALL_DONE
