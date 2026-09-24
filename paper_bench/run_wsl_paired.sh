#!/bin/bash
# Paired re-measurement on a WSL2 host (r3), written after two problems of the first WSL runs:
#  1. the clock: libstdc++'s high_resolution_clock is system_clock, which WSL2 steps at Hyper-V time
#     syncs (steps of several hundred ms up to ~1 s: one of -643.7 ms observed in 90 s; negative instance
#     times down to -958.6 ms in the old archives).  The harness and both arms' MEASUREMENT library now
#     use steady_clock (measurement code only).
#  2. the cores: WSL2 vCPUs are scheduled by Hyper-V over the host's performance and efficiency cores
#     (Core Ultra 5 125H: 4 P + 10 E); taskset pins a process to a vCPU, not to a physical core.  So the
#     arms of one instance (LMF) or of one query file (decider) run back-to-back on the SAME vCPU, and at
#     most three streams run at a time.  Which arm goes first alternates with the global pair / file index;
#     with two LMF streams (pairs split by index parity) each stream therefore keeps one order and the
#     order is balanced across the two streams, not within each.
#
#   bash paper_bench/run_wsl_paired.sh lmf_sig <stream> <nstreams> <cpu>
#        Sigspatial value computation on the 1,000 decider pairs, pairs with ((index-1) % nstreams ==
#        stream); per pair one process per arm (original, candidate5 exact, candidate5 noslack), each with
#        /usr/bin/time peak RSS.  The two pairs on which the baseline needs > 12 GB (125, 432) run only on
#        the candidate5 arms.  Output: $W/lmf/<arm>/<index>.csv, $W/lmf/rss.txt
#   bash paper_bench/run_wsl_paired.sh decider <prefix> <data_dir> <cpu>
#        the 23 x 1,000 decider instances of queries/<prefix>_{paperq4,paperq}_*, both arms per file;
#        the proposed arm is candidate5 with MAXREGION_EXACT=1 MAXREGION_SLACK=0.
#        Output: $W/decider/<prefix>_<tag>_<l>_<sign>_<arm>_r3.csv
#   then: python3 paper_bench/merge_wsl_r3.py <W as a Windows/WSL path>   -> results/raw_*_r3.tar.gz
# Binaries: $ORIG, $C5 (default: the paths paper_bench/build.sh produces, ~/b_pb_{original,candidate5}/paper_bench;
# the sources use steady_clock since 2026-09-25.  The r3 run of 2026-09-25 used ~/b_pb_<arm>_s, built from the same
# sources with the same flags.)  Standard error of every run goes to $W/stderr.log.
set -u
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"; PB="$ROOT/paper_bench"; cd "$ROOT"
ORIG="${ORIG:-$HOME/b_pb_original/paper_bench}"; C5="${C5:-$HOME/b_pb_candidate5/paper_bench}"
W="${W:-$HOME/wsl_r3}"; mkdir -p "$W"
for b in "$ORIG" "$C5"; do [ -x "$b" ] || { echo "missing binary: $b (run paper_bench/build.sh or set ORIG/C5)" >&2; exit 1; }; done
MODE="$1"; shift
run_arm() {  # run_arm <arm> <cpu> <mode> <query> <dir> <out>
  local arm=$1 cpu=$2; shift 2
  case "$arm" in
    original) taskset -c "$cpu" "$ORIG" "$@" ;;
    candidate5_exact) env MAXREGION=cech MAXREGION_SLACK=0 MAXREGION_EXACT=1 taskset -c "$cpu" "$C5" "$@" ;;
    candidate5_noslack) env MAXREGION=cech MAXREGION_SLACK=0 taskset -c "$cpu" "$C5" "$@" ;;
  esac
}
if [ "$MODE" = lmf_sig ]; then
  stream=$1; nstreams=$2; cpu=$3; DIR=paper_data/sigspatial/data
  mkdir -p "$W/lmf/original" "$W/lmf/candidate5_exact" "$W/lmf/candidate5_noslack"
  i=0
  while read -r a b; do
    i=$((i+1)); [ $(( (i-1) % nstreams )) -eq "$stream" ] || continue
    printf "%s %s\n" "$a" "$b" > "$W/lmf/p_$stream.txt"
    if [ $((i % 2)) -eq 0 ]; then arms="original candidate5_exact candidate5_noslack"; else arms="candidate5_noslack candidate5_exact original"; fi
    for arm in $arms; do
      if [ "$arm" = original ] && { [ "$i" -eq 125 ] || [ "$i" -eq 432 ]; }; then continue; fi
      /usr/bin/time -f "%M" -o "$W/lmf/t_$stream.txt" timeout 1800 bash -c "$(declare -f run_arm); ORIG=$ORIG C5=$C5 run_arm $arm $cpu lmf $W/lmf/p_$stream.txt $DIR $W/lmf/$arm/$i.csv" > /dev/null 2>> "$W/stderr.log"
      rc=$?; echo "$i $arm rc=$rc rss_kb=$(tail -1 "$W/lmf/t_$stream.txt")" >> "$W/lmf/rss.txt"
    done
  done < "$PB/queries/sigspatial_pairs.txt"
  echo "[r3] lmf_sig stream $stream DONE" >> "$W/log.txt"
elif [ "$MODE" = decider ]; then
  prefix=$1; DIR=$2; cpu=$3; k=0; mkdir -p "$W/decider"
  for tag in paperq4 paperq; do
    for spec in $(for l in -10 -9 -8 -7 -6 -5 -4 -3 -2 -1 0 1 2; do echo "${l}_plus"; done; for l in -10 -9 -8 -7 -6 -5 -4 -3 -2 -1; do echo "${l}_minus"; done); do
      k=$((k+1)); q="$PB/queries/${prefix}_${tag}_${spec}.txt"
      if [ $((k % 2)) -eq 0 ]; then arms="original candidate5"; else arms="candidate5 original"; fi
      for arm in $arms; do
        a=$arm; [ "$arm" = candidate5 ] && a=candidate5_exact
        run_arm $a "$cpu" decider "$q" "$DIR" "$W/decider/${prefix}_${tag}_${spec}_${arm}_r3.csv" 2>> "$W/stderr.log"
      done
    done
    echo "[r3] decider $prefix $tag DONE" >> "$W/log.txt"
  done
fi
