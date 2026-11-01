#!/bin/bash
# r8: 2x2 ablation of candidate8's two ingredients, maximal sets and the enumeration-radius slack, with the
# r5 protocol (one pinned CPU $CPU = 1, the other CPU idle, the arms back-to-back per job in rotating order,
# one process per job and arm), then the counting run (run_count.sh, both CPUs, not timed).
#   arms:   gitlab        the authors' GitLab code                       (arrangement, radius delta)
#           gitlab_slack  the authors' code + candidate8's radius slack  (arrangement, radius delta + s)
#           candidate7    maximal sets, radius delta                     (MAXREGION_SLACK=0)
#           candidate8    maximal sets, radius delta + s                 (MAXREGION_SLACK=budget)
#   build:  DEPS=/opt/deps bash paper_bench/r8_ablation/scripts/build_r8.sh
#   run:    bash paper_bench/r8_ablation/scripts/run_r8.sh        (W=~/r8, about 4 h)
set -u
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"; ROOT="$(cd "$HERE/../../.." && pwd)"; AC=$ROOT/paper_bench/authors_check/scripts
W=${W:-$HOME/r8}; CPU=${CPU:-1}; export ARMS="gitlab gitlab_slack candidate7 candidate8" CPU
mkdir -p $W
{ echo "start $(date -Is) $(nproc) cpus, pinned to $CPU, repo $(git -C $ROOT rev-parse --short HEAD)"
  echo "cpu: $(grep -m1 'model name' /proc/cpuinfo | cut -d: -f2-)"; echo "kernel: $(uname -r)"
  echo "packages: $(dpkg-query -W -f='${Package} ${Version}; ' libgmp10 libgmp-dev libgmpxx4ldbl libmpfr6 libmpfr-dev 2>/dev/null)"
  echo "g++: $(g++ --version | head -1)"; } >> $W/log.txt
W=$W/chars bash $AC/run_lmf3.sh
W=$W/sig   bash $AC/run_sig3.sh
echo "timing done $(date -Is)" >> $W/log.txt
W=$W/count bash $HERE/run_count.sh
echo "all done $(date -Is)" >> $W/log.txt; touch $W/ALL_DONE
