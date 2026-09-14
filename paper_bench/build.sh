#!/bin/bash
# Build paper_bench for both arms.  Out-of-source builds in $HOME/b_pb_<arm>.
#   bash paper_bench/build.sh            # original + candidate5
# GCC 13+: -include flags as in _build_one.sh / _c5_build.sh (identical for both arms).
set -e
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
for arm in ${ARMS:-original candidate5}; do
  B="$HOME/b_pb_$arm"
  cmake -S "$ROOT/paper_bench" -B "$B" -DARM="$ROOT/$arm" -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DCMAKE_CXX_FLAGS="-include cstdint -include array -include cstddef" > "$B.cfg.log" 2>&1
  cmake --build "$B" -j"$(nproc)" --target paper_bench > "$B.build.log" 2>&1
  ls -la "$B/paper_bench"
done
