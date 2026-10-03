#!/bin/bash
# r5: the authors' GitLab code (authors_gitlab/) vs candidate7, the r4 protocol: one pinned CPU ($CPU, default 1),
# the other CPU left idle, both arms back-to-back per job with alternating order, one process per job and arm.
#   build:  ARMS="authors_gitlab candidate7" [DEPS=...] bash paper_bench/r4_original_vs_candidate7/scripts/build.sh
#   run:    bash paper_bench/r5_authors_vs_candidate7/scripts/run_r5.sh        (W=~/r5, about 2.5 h)
# Steps (the authors_check scripts with ARMS="gitlab candidate7"):
#   chars : Characters LMF, the 21,000 pairs of [BKN20] Table 4 (210 chunks x 100)
#   sig   : Sigspatial LMF, the authors' 1,000 decider pairs (all of them; the authors' code under a 5 GB cap)
#   dec4  : decider, factors 1 +- 4^l (paper Table 2), 3 benchmarks x 23 files x 1,000 queries
#   dec2  : decider, the authors' 2^l files, same layout
set -u
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"; ROOT="$(cd "$HERE/../../.." && pwd)"; AC=$ROOT/paper_bench/authors_check/scripts
W=${W:-$HOME/r5}; CPU=${CPU:-1}; export ARMS="gitlab candidate7" CPU
mkdir -p $W; echo "start $(date -Is) $(nproc) cpus, pinned to $CPU" >> $W/log.txt
W=$W/chars bash $AC/run_lmf3.sh
W=$W/sig   bash $AC/run_sig3.sh
W=$W/dec4  bash $AC/run_dec3.sh
W=$W/dec2  bash $HERE/run_dec2.sh     # run_dec3.sh with the authors' 2^l files (tag paperq)
echo "all done $(date -Is)" >> $W/log.txt; touch $W/ALL_DONE
