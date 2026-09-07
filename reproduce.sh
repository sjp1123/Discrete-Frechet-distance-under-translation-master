#!/bin/bash
# One-command reproduction of the headline: rebuild every arm, run the
# correctness gates, and regenerate the factor + candidate4 tables.
# Longer scaling sweeps are opt-in (see REPRODUCE.md §4).
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"
cd "$HERE"
echo "############ 1. build arms ############"
bash _build_one.sh original
bash _build_one.sh candidate2
bash _x2_build_epick.sh
bash _c4_build.sh
echo "############ 2. correctness gates ############"
( cd "$HOME/b_candidate4" && ctest -R maximal_regions --output-on-failure )
bash _oracle_check.sh
bash _c4_check.sh 30
echo "############ 3. headline tables ############"
bash _factor_perpair.sh 100 && python3 _factor_analyze.py | tee "$HERE/_x1_results/factor_analysis.txt"
bash _c4_bench.sh            && python3 _c4_analyze.py     | tee "$HERE/_x1_results/c4_analysis.txt"
echo "############ 4. timing noise ############"
bash _noise.sh 10 8
echo "DONE — see _x1_results/*.md and *.txt"
