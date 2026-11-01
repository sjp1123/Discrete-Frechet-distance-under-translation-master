#!/bin/bash
# r8 builds, all with the flags of paper_bench/build.sh (RelWithDebInfo, CGAL 5.6 with GMPXX):
#   ~/b_pb_authors_gitlab   the authors' GitLab code (baseline)                 r4 build.sh
#   ~/b_pb_candidate7       candidate7 (maximal sets, enumeration radius delta)  r4 build.sh
#   ~/b_pb_candidate8       candidate8 (maximal sets, radius delta + s)          r4 build.sh
#   ~/b_pb_authors_slack    the authors' code with candidate8's radius slack     copy + patches/authors_slack.patch
#   ~/b_pb_candidate8_count candidate8 + base-case counters (COUNT_LOG)          copy + patches/candidate8_count.patch
#     DEPS=/opt/deps bash paper_bench/r8_ablation/scripts/build_r8.sh
set -e
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"; ROOT="$(cd "$HERE/../../.." && pwd)"; P=$HERE/../patches
EXTRA=()
if [ -n "${DEPS:-}" ]; then
  export LIBRARY_PATH=$DEPS/lib CPATH=$DEPS/include
  EXTRA=(-DCGAL_DIR=$DEPS/src/CGAL-5.6 -DCMAKE_PREFIX_PATH=$DEPS -DBoost_INCLUDE_DIR=$DEPS/include)
fi
ARMS="authors_gitlab candidate7 candidate8" bash $ROOT/paper_bench/r4_original_vs_candidate7/scripts/build.sh
S=$HOME/src_r8; rm -rf $S; mkdir -p $S
cp -r $ROOT/authors_gitlab $S/authors_slack; rm -rf $S/authors_slack/test_data $S/authors_slack/experiments
(cd $S/authors_slack && patch -p1 < $P/authors_slack.patch)
cp -r $ROOT/candidate8 $S/candidate8_count
(cd $S/candidate8_count && patch -p1 < $P/candidate8_count.patch)
for arm in authors_slack candidate8_count; do
  B="$HOME/b_pb_$arm"
  cmake -S "$ROOT/paper_bench" -B "$B" -DARM="$S/$arm" -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DCMAKE_CXX_FLAGS="-include cstdint -include array -include cstddef" "${EXTRA[@]}" > "$B.cfg.log" 2>&1
  cmake --build "$B" -j"$(nproc)" --target paper_bench > "$B.build.log" 2>&1
  grep -q "CGAL_USE_GMPXX=1" "$B/arm/lib/cgal_disk_arrangements/CMakeFiles/disc_arrangement_traversal.dir/flags.make" \
    && echo "$arm: GMPXX backend" || echo "$arm: WARNING, CGAL is not using GMPXX"
  ls -la "$B/paper_bench"
done
