#!/bin/bash
# Shared by run_lmf3.sh, run_sig3.sh, run_dec3.sh, run_dec_gitlab.sh (source it, do not run it).
# Repository root from the script location, binaries from the environment (as in the r4 scripts):
#   GITLAB=~/b_pb_authors_gitlab/paper_bench  (ARMS=authors_gitlab .../r4_original_vs_candidate7/scripts/build.sh)
#   ORIG=~/b_pb_original/paper_bench  C7=~/b_pb_candidate7/paper_bench
# ARMS selects the arms and their base order (default "gitlab original candidate7").
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
GITLAB=${GITLAB:-$HOME/b_pb_authors_gitlab/paper_bench}
ORIG=${ORIG:-$HOME/b_pb_original/paper_bench}
C7=${C7:-$HOME/b_pb_candidate7/paper_bench}
read -r -a ARM_LIST <<< "${ARMS:-gitlab original candidate7}"

arm_bin() {
  case $1 in
    gitlab) echo "$GITLAB" ;; original) echo "$ORIG" ;; candidate7) echo "$C7" ;;
    *) echo "unknown arm $1 (gitlab, original, candidate7)" >&2; return 1 ;;
  esac
}

# arm_env <arm>: environment prefix for the arm (candidate7 in its paper configuration)
arm_env() { [ "$1" = candidate7 ] && echo "MAXREGION=cech MAXREGION_EXACT=1 MAXREGION_SLACK=0"; }

# arm_order <n>: the run order for job n, rotating through 2k orders of the k arms so that every arm
# takes every position equally often.  Orders 0..k-1 rotate ARM_LIST left; orders k..2k-1 rotate
# (first arm, then the rest reversed) right.  For the three default arms these are the six orders
# used in the measured runs: g o c, o c g, c g o, g c o, o g c, c o g.
arm_order() {
  local k=${#ARM_LIST[@]} j i out=()
  j=$(( $1 % (2 * k) ))
  if [ $j -lt $k ]; then
    for ((i = 0; i < k; i++)); do out+=("${ARM_LIST[$(( (i + j) % k ))]}"); done
  else
    local base=("${ARM_LIST[0]}") r=$(( j - k ))
    for ((i = k - 1; i >= 1; i--)); do base+=("${ARM_LIST[$i]}"); done
    for ((i = 0; i < k; i++)); do out+=("${base[$(( (i - r + k) % k ))]}"); done
  fi
  echo "${out[@]}"
}

for a in "${ARM_LIST[@]}"; do arm_bin "$a" > /dev/null || exit 1; done
