#!/bin/bash
# Real-data decider queries at the kit's near-threshold values v ± 5e-8 (v = candidate6's LMF value, within about
# 1.4e-8 of δ*; the kit itself uses v ± 1e-6).  Truth: v + 5e-8 YES, v − 5e-8 NO (5e-8 clears the decider's own
# acceptance band δ + 9e-9 plus its rounding, ≤ 1.2e-8 on Sigspatial).  One paper_bench process per file.
#   ARM=candidate8 bash paper_bench/r6_candidate8/scripts/run_tight.sh [OUT=~/r6/tight]
set -u
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
ARM=${ARM:-candidate8}; PB=${PB:-$HOME/b_pb_$ARM/paper_bench}; OUT=${1:-$HOME/r6/tight}/$ARM
Q=$ROOT/paper_bench/r6_candidate8/queries; P=${P:-2}
mkdir -p $OUT
[ -f $Q/sig_c6val_p5e-8_0.txt ] || python3 $ROOT/paper_bench/r6_candidate8/scripts/make_tight_queries.py $Q
unset MAXREGION MAXREGION_EXACT MAXREGION_SLACK MAXREGION_FIX N6_RANGE LMF
[ "$ARM" = candidate7 ] && export MAXREGION_SLACK=0
job() { local f=$1 d
  case $f in sig_*) d=$ROOT/paper_data/sigspatial/data ;; *) d=$ROOT/paper_data/characters_uci/data ;; esac
  timeout 7200 $PB decider $Q/$f.txt $d $OUT/$f.csv > /dev/null 2> $OUT/$f.err; echo "$f rc=$?" >> $OUT/rc.txt; }
export -f job; export ROOT PB Q OUT
ls $Q | sed 's/\.txt$//' | xargs -P $P -I{} bash -c 'job {}'
for c in $OUT/*.csv; do e=1; case $c in *_m5e-8_*) e=0 ;; esac
  awk -F, -v e=$e -v f="$(basename $c)" 'NR>1{n++; if($4!=e) w++} END{printf "%s n=%d wrong=%d\n", f, n, w+0}' $c; done | tee $OUT/summary.txt
