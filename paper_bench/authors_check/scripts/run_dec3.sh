#!/bin/bash
# Decider 4^l (paperq4) sets, three arms per query file back-to-back on CPU $CPU, rotating order
R=/home/claude/Discrete-Frechet-distance-under-translation-master; Q=$R/paper_bench/queries
CH=$R/paper_data/characters_uci/data; SG=$R/paper_data/sigspatial/data
W=/root/dec3; CPU=${CPU:-1}; mkdir -p $W/gitlab $W/original $W/candidate7
echo "start $(date -Is) cpu $CPU" >> $W/log.txt
run() { local arm=$1 name=$2 dir=$3
  case $arm in
    gitlab) timeout 3600 taskset -c $CPU /root/b_pb_gitlab/paper_bench decider $Q/$name.txt $dir $W/gitlab/$name.csv > /dev/null 2>> $W/stderr.log ;;
    original) timeout 3600 taskset -c $CPU /root/b_pb_original/paper_bench decider $Q/$name.txt $dir $W/original/$name.csv > /dev/null 2>> $W/stderr.log ;;
    candidate7) env MAXREGION=cech MAXREGION_EXACT=1 MAXREGION_SLACK=0 timeout 3600 taskset -c $CPU /root/b_pb_candidate7/paper_bench decider $Q/$name.txt $dir $W/candidate7/$name.csv > /dev/null 2>> $W/stderr.log ;;
  esac
  echo "$name $arm rc=$?" >> $W/rc.txt; }
n=0
for set in characters_uci_same characters_uci_all sigspatial; do
  dir=$CH; [ $set = sigspatial ] && dir=$SG
  for ls in "2 plus" "1 plus" "0 plus" "-1 plus" "-2 plus" "-3 plus" "-4 plus" "-5 plus" "-6 plus" "-7 plus" "-8 plus" "-9 plus" "-10 plus" \
            "-1 minus" "-2 minus" "-3 minus" "-4 minus" "-5 minus" "-6 minus" "-7 minus" "-8 minus" "-9 minus" "-10 minus"; do
    set -- $ls; name=${set}_paperq4_$1_$2
    case $((n % 6)) in
      0) o="gitlab original candidate7";; 1) o="original candidate7 gitlab";; 2) o="candidate7 gitlab original";;
      3) o="gitlab candidate7 original";; 4) o="original gitlab candidate7";; 5) o="candidate7 original gitlab";;
    esac
    for a in $o; do run $a $name $dir; done
    n=$((n+1)); echo "$(date +%s) $n $name" >> $W/progress.txt
  done
done
echo "all done $(date -Is)" >> $W/log.txt; touch $W/ALL_DONE
