#!/bin/bash
echo "=== g++ ===";   g++ --version 2>/dev/null | head -1 || echo MISSING
echo "=== cmake ==="; cmake --version 2>/dev/null | head -1 || echo MISSING
echo "=== make ===";  make --version 2>/dev/null | head -1 || echo MISSING
echo "=== CGAL ===";  (grep -m1 CGAL_VERSION_STR /usr/include/CGAL/version.h 2>/dev/null || echo "no CGAL header")
echo "=== boost/gmp/mpfr ==="
for p in libboost-dev libgmp-dev libmpfr-dev libcgal-dev; do
  dpkg -s "$p" >/dev/null 2>&1 && echo "$p: OK" || echo "$p: MISSING"
done
echo "=== repo size (Windows copy) ==="
REPO="/mnt/c/Users/shiju/Downloads/Discrete-Frechet-distance-under-translation-master/Discrete-Frechet-distance-under-translation-master"
du -sh "$REPO" 2>/dev/null | cut -f1
echo "--- test_data size ---"
du -sh "$REPO/test_data" 2>/dev/null | cut -f1
echo "--- original/candidate/candidate2 sizes ---"
for d in original candidate candidate2; do printf "%-11s " "$d"; du -sh "$REPO/$d" 2>/dev/null | cut -f1; done
