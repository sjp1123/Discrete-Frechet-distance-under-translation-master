#!/bin/bash
# Build paper_bench against the authors' code as published on GitLab
# (gitlab.com/anusser/frechet_distance_under_translation, master, commit 3bbb30502e201564bf9859806cda94198f8ac0d4,
#  all files dated 2020-06-27).  Only build/measurement files are taken from original/:
#   CMakeLists.txt, lib/cgal_disk_arrangements/CMakeLists.txt  (CGAL 5 / CMake 3 port, no code change)
#   lib/measurement_tool/measurement_tool.h                     (steady_clock; measurement only)
# Every algorithm source (src/, lib/cgal_disk_arrangements/*.cpp) is the GitLab file.
#   bash build_gitlab_arm.sh                       uses <repo>/authors_gitlab/ (already prepared that way), no zip
#   ZIP=<frechet_distance_under_translation-master.zip> bash build_gitlab_arm.sh
#                                                  unpacks the zip into A=~/arms/gitlab and prepares it as above
#   DEPS=/opt/deps ...                             CGAL/Boost/GMPXX under one prefix (see r4 scripts/build.sh)
# Binary: B=~/b_pb_authors_gitlab (the default GITLAB of the run scripts).
set -e
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
B=${B:-$HOME/b_pb_authors_gitlab}
if [ -z "${ZIP:-}" ] && [ -d "$ROOT/authors_gitlab" ]; then
  A=$ROOT/authors_gitlab
else
  : "${ZIP:?no authors_gitlab/ in the repository: set ZIP=<path to frechet_distance_under_translation-master.zip>}"
  A=${A:-$HOME/arms/gitlab}
  T=$(mktemp -d); unzip -q "$ZIP" -d $T
  rm -rf $A; mkdir -p $(dirname $A); cp -a $T/frechet_distance_under_translation-master $A; rm -rf $A/tools/CMakeCache.txt $A/tools/CMakeFiles
  cp $ROOT/original/CMakeLists.txt $A/CMakeLists.txt
  cp $ROOT/original/lib/cgal_disk_arrangements/CMakeLists.txt $A/lib/cgal_disk_arrangements/CMakeLists.txt
  cp $ROOT/original/lib/measurement_tool/measurement_tool.h $A/lib/measurement_tool/measurement_tool.h
fi
EXTRA=()
if [ -n "${DEPS:-}" ]; then export LIBRARY_PATH=$DEPS/lib CPATH=$DEPS/include
  EXTRA=(-DCGAL_DIR=$DEPS/src/CGAL-5.6 -DCMAKE_PREFIX_PATH=$DEPS -DBoost_INCLUDE_DIR=$DEPS/include); fi
cmake -S $ROOT/paper_bench -B $B -DARM=$A -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DCMAKE_CXX_FLAGS="-include cstdint -include array -include cstddef" "${EXTRA[@]}" > $B.cfg.log 2>&1
cmake --build $B -j"$(nproc)" --target paper_bench > $B.build.log 2>&1
ls -la $B/paper_bench
