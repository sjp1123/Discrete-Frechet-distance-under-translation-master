#!/bin/bash
# r6: one arm on the r4 verification families (exact oracle), LMF and decider, then the verdict table.
#   mkdir -p ~/verif && tar xzf paper_bench/r4_original_vs_candidate7/verification/raw/oracle_instances.tar.gz -C ~/verif
#   bash paper_bench/r6_candidate8/scripts/run_oracle.sh <arm> [VD=~/verif] [OUT=~/r6v/oracle_<arm>] [CPU=0]
# runner.py kills an instance after 45 s without progress (decider) and records it as HANG.
set -u
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"; ROOT="$(cd "$HERE/../../.." && pwd)"
arm=${1:?arm}; VD=${VD:-$HOME/verif}; OUT=${OUT:-$HOME/r6v/oracle_$arm}; CPU=${CPU:-0}
R=$ROOT/paper_bench/authors_check/scripts/runner.py
for pair in "inst res uniform grid cluster collinear copy singleton revisit offset_sig big tiny real_chars real_sig medium_uniform medium_real_chars medium_real_sig" \
            "inst2 res2 review far_t_1e7 far_t_1e8 far_t_sig"; do
  set -- $pair; I=$VD/$1; O=$OUT/$2; shift 2; mkdir -p $O
  for fam in "$@"; do
    python3 $R $arm lmf $I/$fam.pairs $I/$fam $O/${fam}_lmf_$arm.csv 600 $CPU >> $O/runner.log 2>&1
    python3 $R $arm decider $I/$fam.q $I/$fam $O/${fam}_dec_$arm.csv 600 $CPU >> $O/runner.log 2>&1
  done
  ( cd $VD && VD=$I VR=$O ARMS=$arm python3 $HERE/check_oracle.py "$@" ) | tee $O/verdicts.md
done
