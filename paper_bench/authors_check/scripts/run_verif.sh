#!/bin/bash
# Correctness of one arm on the r4 verification families (authors_check/REPORT.md 4.5).
#   VD: folder where paper_bench/r4_original_vs_candidate7/verification/raw/oracle_instances.tar.gz was extracted
#       (it holds inst/ and inst2/)
#   bash paper_bench/authors_check/scripts/run_verif.sh <VD> [arm=gitlab]     (W=~/verif_<arm> CPU=0)
# LMF on every family (inst/ -> $W/gres, inst2/ -> $W/gres2), decider on DEC_FAMS (default "revisit cluster"),
# per-instance timeouts as in runner.py (TMO_CAP), then the verdicts of check_lmf.py / check_dec.py.
set -u
H="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VD=$(cd "${1:?usage: run_verif.sh <folder with inst/ and inst2/> [arm]}" && pwd); arm=${2:-gitlab}
W=${W:-$HOME/verif_$arm}; CPU=${CPU:-0}; DEC_FAMS=${DEC_FAMS:-revisit cluster}
for pair in "inst gres" "inst2 gres2"; do
  set -- $pair; I=$VD/$1; O=$W/$2; mkdir -p $O
  [ -d "$I" ] || { echo "missing $I" >&2; exit 1; }
  for p in $I/*.pairs; do fam=$(basename $p .pairs)
    python3 $H/runner.py $arm lmf $p $I/$fam $O/${fam}_lmf_$arm.csv 600 $CPU | tee -a $O/runner.log
    python3 $H/check_lmf.py $I $fam $O/${fam}_lmf_$arm.csv | tee -a $W/verdicts.txt
  done
  for fam in $DEC_FAMS; do [ -f $I/$fam.q ] || continue
    python3 $H/runner.py $arm decider $I/$fam.q $I/$fam $O/${fam}_dec_$arm.csv 600 $CPU | tee -a $O/runner.log
    python3 $H/check_dec.py $I $fam $O/${fam}_dec_$arm.csv | tee -a $W/verdicts.txt
  done
done
