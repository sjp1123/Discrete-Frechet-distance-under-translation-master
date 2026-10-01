#!/bin/bash
# Split candidate5 / candidate7 black-box calls by phase on the 21,000 Characters pairs (RESULTS.md §7).
# Scratch copies of the two arms get counters (patch.py; the repository sources stay untouched), then candidate7
# runs with its runtime knobs switched one at a time.  Counts are deterministic, so the two streams may share the host.
#   DEPS=/opt/deps (optional, see ../build.sh)  INST=~/inst  bash scripts/bbcalls_split/run.sh
# -> $INST/out/{A..E}.csv; tar them into raw/raw_characters_bbcalls_split.tar.gz
#   A candidate5 | B candidate7 default | C candidate7 MAXREGION_FIX=none N6_RANGE=0 | D none, N6_RANGE=1 | E union, N6_RANGE=0
set -eu
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/../../../.." && pwd)"
INST=${INST:-$HOME/inst}; export INST
rm -rf $INST; mkdir -p $INST/pb $INST/out
for a in candidate5 candidate7; do
  mkdir -p $INST/$a && (cd $ROOT/$a && tar cf - --exclude=./test_data --exclude=./experiments --exclude=./tests .) | (cd $INST/$a && tar xf -)
  cp $HERE/inst_counters.h $INST/$a/src/
done
cp $ROOT/paper_bench/CMakeLists.txt $ROOT/paper_bench/paper_bench.cpp $INST/pb/
python3 $HERE/patch.py
EXTRA=()
if [ -n "${DEPS:-}" ]; then export LIBRARY_PATH=$DEPS/lib CPATH=$DEPS/include
  EXTRA=(-DCGAL_DIR=$DEPS/src/CGAL-5.6 -DCMAKE_PREFIX_PATH=$DEPS -DBoost_INCLUDE_DIR=$DEPS/include); fi
for a in candidate5 candidate7; do
  cmake -S $INST/pb -B $INST/b_$a -DARM=$INST/$a -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DCMAKE_CXX_FLAGS="-include cstdint -include array -include cstddef" "${EXTRA[@]}" > $INST/b_$a.cfg.log 2>&1
  cmake --build $INST/b_$a -j"$(nproc)" --target paper_bench > $INST/b_$a.build.log 2>&1
done
P=$ROOT/paper_bench/queries/characters_uci_lmf_pairs.txt; D=$ROOT/paper_data/characters_uci/data
C5=$INST/b_candidate5/paper_bench; C7=$INST/b_candidate7/paper_bench
base="MAXREGION=cech MAXREGION_EXACT=1 MAXREGION_SLACK=0"
one() { local cpu=$1 cfg=$2 cmd
  case $cfg in
    A) cmd="env $base $C5" ;;
    B) cmd="env $base $C7" ;;
    C) cmd="env $base MAXREGION_FIX=none N6_RANGE=0 $C7" ;;
    D) cmd="env $base MAXREGION_FIX=none N6_RANGE=1 $C7" ;;
    E) cmd="env $base MAXREGION_FIX=union N6_RANGE=0 $C7" ;;
  esac
  t0=$(date +%s); taskset -c $cpu $cmd lmf $P $D $INST/out/$cfg.csv > /dev/null 2> $INST/out/$cfg.err
  echo "$cfg rc=$? $(( $(date +%s)-t0 ))s" >> $INST/out/done.txt; }
( one 0 A; one 0 C; one 0 E ) & ( one 1 B; one 1 D ) & wait
echo "done: $INST/out"
