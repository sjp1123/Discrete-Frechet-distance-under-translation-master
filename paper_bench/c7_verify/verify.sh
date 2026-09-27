#!/bin/bash
# candidate7 verification (results: paper_bench/results/C7_report.md).
#   bash paper_bench/c7_verify/verify.sh [OUT=~/c7_verify_out]
# Builds the harnesses (~/b_c7v) and paper_bench for candidate7 (~/b_pb_candidate7) if missing, then runs
# every job below, 12 at a time, in candidate7's default configuration (= the paper configuration).
# Each job writes $OUT/out/<job>.txt ending in "#rc=<rc> secs=<s>"; summarize.sh prints one line per job.
# QUICK=1 cuts every count by 10 (a smoke test of the kit, not the reported evidence).
set -u
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
K="$ROOT/paper_bench/c7_verify"
OUT="${1:-$HOME/c7_verify_out}"
B="${B:-$HOME/b_c7v}"
PB="${PB:-$HOME/b_pb_candidate7/paper_bench}"
FLAGS="-include cstdint -include array -include cstddef"
q() { if [ "${QUICK:-0}" = 1 ]; then echo $(( $1 / 10 )); else echo $1; fi; }
mkdir -p "$OUT/out" "$OUT/work/a" "$OUT/nt"
rm -f "$OUT"/out/*.txt "$OUT/out/_DONE" "$OUT/nt/rc.txt"
unset MAXREGION MAXREGION_EXACT MAXREGION_SLACK MAXREGION_FIX N6_RANGE LMF

if [ ! -x "$B/fuzz_oracle" ]; then
	cmake -S "$K" -B "$B" -DARM="$ROOT/candidate7" -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_CXX_FLAGS="$FLAGS" > "$OUT/build.log" 2>&1
	cmake --build "$B" -j6 >> "$OUT/build.log" 2>&1 || { echo "build failed, see $OUT/build.log" >&2; exit 1; }
fi
if [ ! -x "$PB" ]; then
	cmake -S "$ROOT/paper_bench" -B "$(dirname "$PB")" -DARM="$ROOT/candidate7" -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_CXX_FLAGS="$FLAGS" >> "$OUT/build.log" 2>&1
	cmake --build "$(dirname "$PB")" -j6 --target paper_bench >> "$OUT/build.log" 2>&1 || { echo "paper_bench build failed" >&2; exit 1; }
fi

# A: case files "<delta>\n<n>\n<n points>\n<m>\n<m points>" -> paper_bench curve files and queries
W="$OUT/work/a"
mk() { awk -v W="$W" -v T="$2" 'NR==1{next} NR==2{n=$1; next} NR>2 && NR<=2+n {print > (W "/" T "_pi.txt"); next} NR==3+n {next} {print > (W "/" T "_sigma.txt")}' "$1"; }
: > "$W/q_r3.txt"
for d in 45000 30000 20000; do
	mk "$K/cases/a/case_sig_$d.txt" S$d
	echo "S${d}_pi.txt S${d}_sigma.txt $d" >> "$W/q_r3.txt"
	echo "S${d}_pi.txt S${d}_sigma.txt $(awk -v d=$d 'BEGIN{printf "%.17g", d*0.999}')" >> "$W/q_r3.txt"
done
mk "$K/cases/a/HIT_2_0.02685716710196421.txt" H2
mk "$K/cases/a/HIT_30000_30_2_0.02685716710151593.txt" H30
printf "H2_pi.txt H2_sigma.txt\nH30_pi.txt H30_sigma.txt\n" > "$W/pairs_g1.txt"
printf "H2_pi.txt H2_sigma.txt 45000\nH30_pi.txt H30_sigma.txt 30000\nH2_pi.txt H2_sigma.txt 44000\nH30_pi.txt H30_sigma.txt 29300\n" > "$W/q_g1.txt"

# the job lines are word-split, so inputs and data dirs are referenced through $OUT (no spaces)
cp -r "$K/cases/g4" "$OUT/work/"
cp -r "$K/cases/near_threshold" "$OUT/work/"
ln -sfn "$ROOT/paper_data/sigspatial/data" "$OUT/work/sigspatial_data"
ln -sfn "$ROOT/paper_data/characters_uci/data" "$OUT/work/characters_data"
G4="$OUT/work/g4"
cat > "$OUT/jobs.txt" <<EOF
box_pred|$B/test_box_predicates
sp_dec|$B/test_single_point_family 1 $(q 20000) 6 6
sp_lmf|LMF=1 $B/test_single_point_family 1 $(q 20000) 6 6
sp_dec_12|$B/test_single_point_family 1 $(q 20000) 12 12
sp_dec_ext|$B/test_single_point_family 20001 $(q 30000) 6 6
cl|$B/test_clustered_family 1 $(q 20000)
exact_f1|$B/test_exact_ref 1 1 $(q 3000)
exact_f2|$B/test_exact_ref 2 1 $(q 3000)
exact_f3|$B/test_exact_ref 3 1 $(q 3000)
exact_f4|$B/test_exact_ref 4 1 $(q 3000)
exact_f5a|$B/test_exact_ref 5 1 $(q 5000)
exact_f5b|$B/test_exact_ref 5 5001 $(q 5000)
one_4697|$B/test_one 4697
diag|$B/test_diag
light|$B/test_light 1 $(q 3000) 8 5
far_m1|$B/test_far 1 $(q 1000) 16 1 1
far_m2|$B/test_far 1 $(q 1000) 16 2 1
far_27|$B/test_far 27 $(q 200) 16 1 100
hang_672e5|G3_T=60 $B/probe hang 6.72e7
hang_1e8|G3_T=60 $B/probe hang 1e8
hang_1e9|G3_T=60 $B/probe hang 1e9
circ_g3|G3_T=120 $B/probe circ 16777216 6e-8
circ_g3b|G3_T=120 $B/probe circ 16777216 1.2e-7
e2e_f09|$B/e2e 50 5525 1
a_r3_dec|$PB decider $W/q_r3.txt $W $W/r3_dec.csv
a_g1_lmf|$PB lmf $W/pairs_g1.txt $W $W/g1_lmf.csv
a_g1_dec|$PB decider $W/q_g1.txt $W $W/g1_dec.csv
b_g4_f01|$PB decider $G4/f01_wrongNO.q $G4 $W/g4_f01.csv
c_g4_f03|$PB decider $G4/f03_hang.q $G4 $W/g4_f03.csv
h_g4_assert|$PB decider $G4/assert_ii.q $G4 $W/g4_assert.csv
EOF
for g in uniform grid dup cluster single offset scale swap hdup revisit amp; do
	echo "fz_${g}_def|$B/fuzz_oracle $g 1 $(q 2000) 0 0" >> "$OUT/jobs.txt"
done
for g in grid dup uniform; do
	echo "fz_${g}_6_3|$B/fuzz_oracle $g 1 $(q 2000) 6 3" >> "$OUT/jobs.txt"
	echo "fz_${g}_3_2|$B/fuzz_oracle $g 1 $(q 2000) 3 2" >> "$OUT/jobs.txt"
	echo "fz_${g}_40_3|$B/fuzz_oracle $g 1 $(q 2000) 40 3" >> "$OUT/jobs.txt"
	echo "fz_${g}_dp2|MAXREGION_DP_LIMIT=2 $B/fuzz_oracle $g 1 $(q 2000) 0 0" >> "$OUT/jobs.txt"
done
# near-threshold real-data queries: candidate6's LMF value v +- 1e-6 per pair (truth: plus YES, minus NO)
NT="$OUT/work/near_threshold"
for s in plus minus; do
	for i in 0 1; do echo "nt_sig_${s}_$i|$PB decider $NT/sig_c6val_${s}_$i.txt $OUT/work/sigspatial_data $OUT/nt/sig_c6val_${s}_$i.csv" >> "$OUT/jobs.txt"; done
	for i in 0 1 2 3; do echo "nt_chars_${s}_$i|$PB decider $NT/chars_c6val_${s}_$i.txt $OUT/work/characters_data $OUT/nt/chars_c6val_${s}_$i.csv" >> "$OUT/jobs.txt"; done
done
[ "${QUICK:-0}" = 1 ] && sed -i '/^nt_chars/d; /^box_pred/d' "$OUT/jobs.txt"

runjob() {
	local name="${1%%|*}" cmd="${1#*|}"
	local t0; t0=$(date +%s.%N)
	( cd "$OUT/work" && timeout 7200 env $cmd ) > "$OUT/out/$name.txt" 2>&1
	local rc=$?
	echo "#rc=$rc secs=$(echo "$(date +%s.%N) - $t0" | bc)" >> "$OUT/out/$name.txt"
}
export -f runjob; export OUT
grep -v '^$' "$OUT/jobs.txt" | xargs -d '\n' -P 12 -I{} bash -c 'runjob "$@"' _ {}
echo "VERIFY DONE $(date)" > "$OUT/out/_DONE"
bash "$K/summarize.sh" "$OUT"
