#!/bin/bash
# Build candidate6 (candidate5 + box-restricted maximal sets, exact box predicates, LMF range fix).
# Out-of-source build (source on /mnt/c, objects in WSL native fs ~/) of just the
# calc_frechet_distance_under_translation target, same shape as _build_one.sh.
#
# ROOT defaults to the directory this script lives in, so the checkout can move.
ROOT="${ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)}"
SRC="$ROOT/candidate6"
BUILD="$HOME/b_candidate5"
LOG="$ROOT/_c6_build.log"
TGT=calc_frechet_distance_under_translation
{
  echo "### [candidate6] configure $(date)"
  # -include: GCC 13+ no longer pulls <array>/<cstdint> in transitively.
  # CGAL >= 6 requires C++17; the tree asks for C++14, so newer CGAL needs
  #   sed -i 's/-std=c++14/-std=c++17/' candidate6/CMakeLists.txt
  # applied identically to every arm being compared.
  cmake -S "$SRC" -B "$BUILD" -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DCMAKE_CXX_FLAGS="-include cstdint -include array -include cstddef" 2>&1
  cfg=$?
  echo "### [candidate6] configure rc=$cfg"
  if [ $cfg -eq 0 ]; then
    echo "### [candidate6] build $(date)"
    cmake --build "$BUILD" -j"$(nproc)" --target "$TGT" 2>&1
    echo "### [candidate6] build rc=$?"
  fi
  echo "### [candidate6] artifact:"
  ls -la "$BUILD/$TGT" 2>&1
  echo "### [candidate6] DONE $(date)"
} | tee "$LOG"
