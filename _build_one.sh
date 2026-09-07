#!/bin/bash
# Usage: _build_one.sh <dirname>   e.g. original | candidate | candidate2
# Out-of-source build (source on /mnt/c, objects in WSL native fs ~/) of just the
# calc_frechet_distance_under_translation target.
name="$1"
ROOT="/mnt/c/Users/shiju/Downloads/Discrete-Frechet-distance-under-translation-master/Discrete-Frechet-distance-under-translation-master"
SRC="$ROOT/$name"
BUILD="$HOME/b_$name"
LOG="$ROOT/_build_${name}.log"
TGT=calc_frechet_distance_under_translation
{
  echo "### [$name] configure $(date)"
  cmake -S "$SRC" -B "$BUILD" -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DCMAKE_CXX_FLAGS="-include cstdint -include array -include cstddef" 2>&1
  cfg=$?
  echo "### [$name] configure rc=$cfg"
  if [ $cfg -eq 0 ]; then
    echo "### [$name] build $(date)"
    cmake --build "$BUILD" -j"$(nproc)" --target "$TGT" 2>&1
    echo "### [$name] build rc=$?"
  fi
  echo "### [$name] artifact:"
  ls -la "$BUILD/$TGT" 2>&1
  echo "### [$name] DONE $(date)"
} | tee "$LOG"
