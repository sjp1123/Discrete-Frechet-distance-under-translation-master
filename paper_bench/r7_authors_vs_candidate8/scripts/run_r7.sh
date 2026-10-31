#!/bin/bash
# r7: the authors' GitLab code (authors_gitlab/) vs candidate8 as merged (f51bcbd source, MAXREGION_SLACK=budget),
# all of r5 again in one run with the r5 protocol: one pinned CPU ($CPU, default 1), the other CPU left idle,
# both arms back-to-back per job in rotating order, one process per job and arm.
#   build:  ARMS="authors_gitlab candidate8" DEPS=/opt/deps bash paper_bench/r4_original_vs_candidate7/scripts/build.sh
#   run:    bash paper_bench/r7_authors_vs_candidate8/scripts/run_r7.sh        (W=~/r7, about 2.5 h)
# Steps (the authors_check scripts with ARMS="gitlab candidate8"):
#   chars : Characters LMF, the 21,000 pairs of [BKN20] Table 4 (210 chunks x 100)
#   sig   : Sigspatial LMF, the authors' 1,000 decider pairs (the authors' code under a 5 GB cap)
#   dec4  : decider, factors 1 +- 4^l (paper Table 2), 3 benchmarks x 23 files x 1,000 queries
#   dec2  : decider, the authors' 2^l files, same layout (r5's run_dec2.sh)
set -u
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"; ROOT="$(cd "$HERE/../../.." && pwd)"; AC=$ROOT/paper_bench/authors_check/scripts
W=${W:-$HOME/r7}; CPU=${CPU:-1}; export ARMS="gitlab candidate8" CPU
mkdir -p $W
{ echo "start $(date -Is) $(nproc) cpus, pinned to $CPU, repo $(git -C $ROOT rev-parse --short HEAD)"
  echo "cpu: $(grep -m1 'model name' /proc/cpuinfo | cut -d: -f2-)"; echo "kernel: $(uname -r)"
  echo "packages: $(dpkg-query -W -f='${Package} ${Version}; ' libgmp10 libgmp-dev libgmpxx4ldbl libmpfr6 libmpfr-dev 2>/dev/null)"
  echo "g++: $(g++ --version | head -1)"; } >> $W/log.txt
W=$W/chars bash $AC/run_lmf3.sh
W=$W/sig   bash $AC/run_sig3.sh
W=$W/dec4  bash $AC/run_dec3.sh
W=$W/dec2  bash $ROOT/paper_bench/r5_authors_vs_candidate7/scripts/run_dec2.sh
echo "all done $(date -Is)" >> $W/log.txt; touch $W/ALL_DONE
