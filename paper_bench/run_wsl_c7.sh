#!/bin/bash
# Paired timing of original, candidate7 and candidate5 on this PC (WSL2), from a plan made by
# plan_wsl_c7.py.  Each stream is pinned to one vCPU and runs its jobs in order; each job runs
# the three arms back-to-back on that vCPU, in an order that rotates with the job number.
#
#   bash paper_bench/run_wsl_c7.sh <plan dir> [W=~/wsl_c7]
#
# Outputs: $W/{lmf_chars,lmf_sig,decider}/<arm>/<job>.csv (paper_bench CSVs, flushed per row),
#          $W/rss/<job>_<arm>.txt ("<elapsed s> <peak RSS kB>" from /usr/bin/time),
#          $W/rc.txt ("<job> <arm> rc=<rc>"), $W/ALL_DONE.
# Binaries: $ORIG, $C7, $C5 (default ~/b_pb_<arm>/paper_bench, from paper_bench/build.sh).
# candidate7 needs no environment (its defaults are the paper configuration); the same variables
# as for candidate5 are set anyway so that the two proposed arms are configured identically.
set -u
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PLAN="$1"; W="${2:-$HOME/wsl_c7}"
ORIG="${ORIG:-$HOME/b_pb_original/paper_bench}"
C7="${C7:-$HOME/b_pb_candidate7/paper_bench}"
C5="${C5:-$HOME/b_pb_candidate5/paper_bench}"
for b in "$ORIG" "$C7" "$C5"; do [ -x "$b" ] || { echo "missing binary: $b" >&2; exit 1; }; done
CH="$ROOT/paper_data/characters_uci/data"; SG="$ROOT/paper_data/sigspatial/data"; Q="$ROOT/paper_bench/queries"
mkdir -p "$W/rss" "$W/tmp"
for t in lmf_chars lmf_sig decider; do for a in original candidate7 candidate5; do mkdir -p "$W/$t/$a"; done; done

run_arm() {  # run_arm <arm> <cpu> <timeout s> <tag> <paper_bench args...>
	local arm=$1 cpu=$2 tmo=$3 tag=$4; shift 4
	local bin
	case $arm in
		original)   bin=$ORIG ;;
		candidate7) bin=$C7 ;;
		candidate5) bin=$C5 ;;
	esac
	if [ "$arm" = original ]; then
		/usr/bin/time -f "%e %M" -o "$W/rss/${tag}_$arm.txt" timeout "$tmo" taskset -c "$cpu" "$bin" "$@" > /dev/null 2>> "$W/stderr.log"
	else
		/usr/bin/time -f "%e %M" -o "$W/rss/${tag}_$arm.txt" env MAXREGION=cech MAXREGION_EXACT=1 MAXREGION_SLACK=0 \
			timeout "$tmo" taskset -c "$cpu" "$bin" "$@" > /dev/null 2>> "$W/stderr.log"
	fi
	echo "$tag $arm rc=$?" >> "$W/rc.txt"
}

run_stream() {  # run_stream <k> <cpu>
	local k=$1 cpu=$2 type id rest n arms arm
	while read -r type id rest; do
		[ -z "$type" ] && continue
		n=$((10#${id//[!0-9]/} % 3))
		case $n in
			0) arms="original candidate7 candidate5" ;;
			1) arms="candidate7 candidate5 original" ;;
			2) arms="candidate5 original candidate7" ;;
		esac
		for arm in $arms; do
			case $type in
				lmfc) run_arm $arm $cpu 3600 "$id" lmf "$PLAN/chunks/$id.txt" "$CH" "$W/lmf_chars/$arm/$id.csv" ;;
				lmfs)
					set -- $rest   # <i> <f1> <f2> <skip_original>
					if [ "$arm" = original ] && [ "$4" = 1 ]; then echo "$id $arm skipped(>12GB in r3)" >> "$W/rc.txt"; continue; fi
					printf "%s %s\n" "$2" "$3" > "$W/tmp/$id.txt"
					run_arm $arm $cpu 1800 "$id" lmf "$W/tmp/$id.txt" "$SG" "$W/lmf_sig/$arm/$id.csv" ;;
				dec)
					local dir=$CH; case $rest in sigspatial*) dir=$SG ;; esac
					run_arm $arm $cpu 3600 "$id" decider "$Q/$rest.txt" "$dir" "$W/decider/$arm/$id.csv" ;;
			esac
		done
		echo "$(date +%s) $id" >> "$W/progress_s$k.txt"
	done < "$PLAN/s$k.txt"
	echo "stream $k done $(date)" >> "$W/log.txt"
}

read -r -a CPUS < "$PLAN/cpus.txt"
echo "start $(date) streams=${#CPUS[@]} orig=$ORIG c7=$C7 c5=$C5" >> "$W/log.txt"
for k in "${!CPUS[@]}"; do run_stream "$k" "${CPUS[$k]}" & done
wait
touch "$W/ALL_DONE"
echo "all done $(date)" >> "$W/log.txt"
