#!/bin/bash
# Reconnaissance for the reproducibility/credibility package.
ROOT="/mnt/c/Users/shiju/Downloads/Discrete-Frechet-distance-under-translation-master/Discrete-Frechet-distance-under-translation-master"
echo "===== CPU ====="; lscpu 2>/dev/null | grep -iE "model name|^CPU\(s\)|Thread|Core|Socket|MHz|cache" | head -20
echo "===== governor / freq control ====="
for c in /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor; do
  [ -r "$c" ] && echo "governor: $(cat $c)" || echo "governor: (not exposed under WSL)"; done
echo "===== mem / kernel / wsl ====="; free -h 2>/dev/null | head -2; uname -a
echo "===== toolchain ====="; g++ --version | head -1; cmake --version | head -1
grep -m1 "define CGAL_VERSION " /usr/include/CGAL/version.h 2>/dev/null
ls /usr/include/boost/version.hpp >/dev/null 2>&1 && grep -m1 "BOOST_LIB_VERSION" /usr/include/boost/version.hpp
echo "===== existing .gitignore ====="; cat "$ROOT/.gitignore" 2>/dev/null; echo "---"
echo "===== git present? ====="; git -C "$ROOT" rev-parse --is-inside-work-tree 2>&1
echo "===== seed usage in generators ====="
grep -nE "seed|random_state|np.random|srand|mt19937|default_random" "$ROOT"/_x3_gen.py "$ROOT"/_gen_wall.py "$ROOT"/_gen_nsweep.py 2>/dev/null | head -20
echo "===== stored reference results (baseline ground-truth) ====="
ls -la "$ROOT/original/test_data/benchmark/Geolife Trajectories 1.3/data" 2>/dev/null | head -1
du -sh "$ROOT/original/test_data" 2>/dev/null
echo "candidate_results.json sizes (original authors' results):"
ls -la "$ROOT/original/candidate_results.json" "$ROOT/candidate"*/candidate_results.json 2>/dev/null | awk '{print $5, $NF}'
echo "===== naive/brute-force decider targets available? ====="
grep -rl "naive\|brute" "$ROOT"/CMakeLists.txt 2>/dev/null
ls "$ROOT/src"/*naive* 2>/dev/null
echo "===== total repo size (sans data) ====="
du -sh --exclude="test_data" "$ROOT" 2>/dev/null
