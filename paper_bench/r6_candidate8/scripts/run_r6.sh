#!/bin/bash
# r6: candidate8 against candidate7 and the authors' code, the r5 protocol (one pinned CPU $CPU, the other CPU
# idle, the arms back-to-back per job in rotating order, one process per job and arm).
#   build:  ARMS="authors_gitlab candidate7 candidate8" bash paper_bench/r4_original_vs_candidate7/scripts/build.sh
#   run:    bash paper_bench/r6_candidate8/scripts/run_r6.sh          (W=~/r6)
# Steps (the authors_check scripts):
#   chars : Characters LMF, the 21,000 pairs of [BKN20] Table 4, ARMS="gitlab candidate7 candidate8"
#   sig   : Sigspatial LMF, the authors' 1,000 decider pairs, same arms (the authors' code under a 5 GB cap)
#   dec4  : decider, factors 1 ± 4^l, 3 benchmarks x 23 files x 1,000 queries, ARMS="candidate7 candidate8"
#           (the authors' code on these files is in r5)
set -u
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"; ROOT="$(cd "$HERE/../../.." && pwd)"; AC=$ROOT/paper_bench/authors_check/scripts
W=${W:-$HOME/r6}; CPU=${CPU:-1}; export CPU
mkdir -p $W; echo "start $(date -Is) $(nproc) cpus, pinned to $CPU" >> $W/log.txt
ARMS="gitlab candidate7 candidate8" W=$W/chars bash $AC/run_lmf3.sh
ARMS="gitlab candidate7 candidate8" W=$W/sig   bash $AC/run_sig3.sh
ARMS="candidate7 candidate8"        W=$W/dec4  bash $AC/run_dec3.sh
echo "all done $(date -Is)" >> $W/log.txt; touch $W/ALL_DONE
