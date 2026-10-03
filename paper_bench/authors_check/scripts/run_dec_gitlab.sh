#!/bin/bash
# decider with the authors' code (arm gitlab), same files as r4 (tag given as $1: paperq or paperq4), pinned CPU $CPU
#   bash paper_bench/authors_check/scripts/run_dec_gitlab.sh paperq      (W=~/gitdec CPU=1, binary GITLAB, see arms.sh)
ARMS=gitlab source "$(dirname "${BASH_SOURCE[0]}")/arms.sh"
Q=$ROOT/paper_bench/queries; CH=$ROOT/paper_data/characters_uci/data; SG=$ROOT/paper_data/sigspatial/data
W=${W:-$HOME/gitdec}; CPU=${CPU:-1}; BIN=$GITLAB; mkdir -p $W/gitlab
tag=${1:?usage: run_dec_gitlab.sh <paperq|paperq4>}
for set in characters_uci_same characters_uci_all sigspatial; do
  dir=$CH; [ $set = sigspatial ] && dir=$SG
  for ls in "2 plus" "1 plus" "0 plus" "-1 plus" "-2 plus" "-3 plus" "-4 plus" "-5 plus" "-6 plus" "-7 plus" "-8 plus" "-9 plus" "-10 plus" \
            "-1 minus" "-2 minus" "-3 minus" "-4 minus" "-5 minus" "-6 minus" "-7 minus" "-8 minus" "-9 minus" "-10 minus"; do
    set -- $ls; name=${set}_${tag}_$1_$2
    timeout 3600 taskset -c $CPU $BIN decider $Q/$name.txt $dir $W/gitlab/$name.csv > /dev/null 2>> $W/stderr.log
    echo "$name rc=$?" >> $W/rc.txt
  done
done
echo "done $tag $(date -Is)" >> $W/log.txt
