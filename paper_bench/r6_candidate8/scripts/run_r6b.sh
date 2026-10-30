#!/bin/bash
# r6b: candidate7 vs candidate8 after the centre-rounding fix (f51bcbd), the r5 protocol (one pinned CPU $CPU,
# the other CPU idle, both arms back-to-back per job in rotating order).  The authors' code is in run_r6.sh's run.
#   build:  ARMS="candidate7 candidate8" bash paper_bench/r4_original_vs_candidate7/scripts/build.sh
#   run:    bash paper_bench/r6_candidate8/scripts/run_r6b.sh          (W=~/r6b, about 70 min)
set -u
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"; ROOT="$(cd "$HERE/../../.." && pwd)"; AC=$ROOT/paper_bench/authors_check/scripts
W=${W:-$HOME/r6b}; CPU=${CPU:-1}; export CPU
mkdir -p $W; echo "start $(date -Is) candidate8 $(git -C $ROOT rev-parse --short HEAD)" >> $W/log.txt
ARMS="candidate7 candidate8" W=$W/chars bash $AC/run_lmf3.sh
ARMS="candidate7 candidate8" W=$W/sig   bash $AC/run_sig3.sh
ARMS="candidate7 candidate8" W=$W/dec4  bash $AC/run_dec3.sh
echo "all done $(date -Is)" >> $W/log.txt; touch $W/ALL_DONE
