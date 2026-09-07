#!/bin/bash
# Build candidate4 (main target + the maximal_regions regression test) and run ctest.
ROOT="/mnt/c/Users/shiju/Downloads/Discrete-Frechet-distance-under-translation-master/Discrete-Frechet-distance-under-translation-master"
SRC="$ROOT/candidate4"
BUILD="$HOME/b_candidate4"
echo "### configure $(date)"
cmake -S "$SRC" -B "$BUILD" -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_CXX_FLAGS="-include cstdint -include array -include cstddef" > "$ROOT/_c4_build.log" 2>&1
echo "configure rc=$?"
echo "### build calc target"
cmake --build "$BUILD" -j"$(nproc)" --target calc_frechet_distance_under_translation >> "$ROOT/_c4_build.log" 2>&1
echo "calc build rc=$?"
echo "### build test target"
cmake --build "$BUILD" -j"$(nproc)" --target test_maximal_regions >> "$ROOT/_c4_build.log" 2>&1
echo "test build rc=$?"
ls -la "$BUILD/calc_frechet_distance_under_translation" 2>&1 | tail -1
echo "### ctest -R maximal_regions"
( cd "$BUILD" && ctest -R maximal_regions --output-on-failure ) 2>&1 | tail -40
