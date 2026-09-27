#!/bin/bash
# One line per verify.sh job, then the paper_bench reproducer outputs and the near-threshold answers.
#   bash paper_bench/c7_verify/summarize.sh [OUT=~/c7_verify_out]
OUT="${1:-$HOME/c7_verify_out}"; O="$OUT/out"; W="$OUT/work/a"
for f in "$O"/*.txt; do
	n=$(basename "$f" .txt)
	rc=$(grep -o '#rc=[0-9]* secs=[0-9.]*' "$f" | tail -1); [ -z "$rc" ] && rc="(running)"
	case $n in
		box_pred) s=$(grep -o 'mismatches=[0-9]*' "$f" | tail -1) ;;
		sp_*) s=$(grep 'DONE' "$f" | tail -1) ;;
		cl) s="lines=$(grep -c '^S ' "$f") zero_answers=$(awk '/^S /{for(i=4;i<=NF;i++) if($i==0) z++} END{print z+0}' "$f")" ;;
		exact_*|far_*) s=$(grep 'DONE' "$f" | tail -1) ;;
		diag) s=$(grep -E 'lt\(0\.1767766952966368' "$f" | tr '\n' ' ') ;;
		light) s=$(grep 'DONE' "$f") ;;
		one_*) s=$(grep -E 'value=|END|Assert' "$f" | tr '\n' ' ') ;;
		hang_*|circ_*) s=$(grep -E '^(VAL|TIMEOUT|HANG|CHILD)' "$f") ;;
		e2e_*) s=$(grep 'calcDistance2' "$f") ;;
		fz_*) s="$(grep '^DONE' "$f" | cut -c1-150) bad_lines=$(grep -cE '^(FN|FP|VAL|CRASH|TIMEOUT|CHILD_BAD|EXC|TRANS|ORACLE_MISMATCH)' "$f")" ;;
		*) s="" ;;
	esac
	echo "$n | $rc | $s"
done
echo "--- reproducers through paper_bench (file1,file2,distance|value,answer|...)"
for c in r3_dec g1_lmf g1_dec g4_f01 g4_f03 g4_assert; do echo "== $c"; [ -f "$W/$c.csv" ] && cut -d, -f1-5 "$W/$c.csv"; done
echo "--- near-threshold real data (truth: plus YES, minus NO)"
for c in "$OUT"/nt/*.csv; do
	[ -f "$c" ] || continue
	e=1; case $c in *minus*) e=0 ;; esac
	awk -F, -v e=$e -v f="$(basename "$c")" 'NR>1{n++; if($4!=e) w++} END{printf "%s n=%d wrong=%d\n", f, n, w+0}' "$c"
done
