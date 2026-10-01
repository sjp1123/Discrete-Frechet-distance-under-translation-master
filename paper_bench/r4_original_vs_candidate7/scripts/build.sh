#!/bin/bash
# Build paper_bench for the two arms of r4 (original, candidate7) in ~/b_pb_<arm>, the flags of paper_bench/build.sh.
# With the distro packages (libcgal-dev: CGAL 5.6, Boost 1.83, GMP/MPFR/GMPXX) nothing else is needed.
# r4 ran in a container without the Ubuntu archive; there CGAL 5.6 and Boost 1.83 came from their GitHub releases and
# GMP/MPFR/GMPXX headers + libgmpxx were built against the system libgmp/libmpfr, all under one prefix:
#   DEPS=/opt/deps bash scripts/build.sh     (CGAL_DIR=$DEPS/src/CGAL-5.6, headers in $DEPS/include, libs in $DEPS/lib)
# CGAL then finds GMPXX and uses the GMPXX number-type backend, as with the distro packages.
set -e
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
EXTRA=()
if [ -n "${DEPS:-}" ]; then
  export LIBRARY_PATH=$DEPS/lib CPATH=$DEPS/include
  EXTRA=(-DCGAL_DIR=$DEPS/src/CGAL-5.6 -DCMAKE_PREFIX_PATH=$DEPS -DBoost_INCLUDE_DIR=$DEPS/include)
fi
for arm in ${ARMS:-original candidate7}; do
  B="$HOME/b_pb_$arm"
  cmake -S "$ROOT/paper_bench" -B "$B" -DARM="$ROOT/$arm" -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DCMAKE_CXX_FLAGS="-include cstdint -include array -include cstddef" "${EXTRA[@]}" > "$B.cfg.log" 2>&1
  cmake --build "$B" -j"$(nproc)" --target paper_bench > "$B.build.log" 2>&1
  grep -q "CGAL_USE_GMPXX=1" "$B/arm/lib/cgal_disk_arrangements/CMakeFiles/disc_arrangement_traversal.dir/flags.make" \
    && echo "$arm: GMPXX backend" || echo "$arm: WARNING, CGAL is not using GMPXX"
  ls -la "$B/paper_bench"
done
