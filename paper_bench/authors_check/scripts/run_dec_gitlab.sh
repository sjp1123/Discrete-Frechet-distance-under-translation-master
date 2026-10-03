#!/bin/bash
# decider with the authors' N6 (arm gitlab), same files as r4 (tag given as $1), pinned CPU 1
R=/home/claude/Discrete-Frechet-distance-under-translation-master; Q=$R/paper_bench/queries
CH=$R/paper_data/characters_uci/data; SG=$R/paper_data/sigspatial/data
W=/root/gitdec; BIN=/root/b_pb_gitlab/paper_bench; mkdir -p $W/gitlab
tag=$1
for set in characters_uci_same characters_uci_all sigspatial; do
  dir=$CH; [ $set = sigspatial ] && dir=$SG
  for ls in "2 plus" "1 plus" "0 plus" "-1 plus" "-2 plus" "-3 plus" "-4 plus" "-5 plus" "-6 plus" "-7 plus" "-8 plus" "-9 plus" "-10 plus" \
            "-1 minus" "-2 minus" "-3 minus" "-4 minus" "-5 minus" "-6 minus" "-7 minus" "-8 minus" "-9 minus" "-10 minus"; do
    set -- $ls; name=${set}_${tag}_$1_$2
    timeout 3600 taskset -c 1 $BIN decider $Q/$name.txt $dir $W/gitlab/$name.csv > /dev/null 2>> $W/stderr.log
    echo "$name rc=$?" >> $W/rc.txt
  done
done
echo "done $tag $(date -Is)" >> $W/log.txt
