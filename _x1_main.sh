#!/bin/bash
# X1 preview on the MAIN path (fut_lmf): aggregate per MAXIMAL_MODE over the
# geolife_small manifest. Reports m, K, gen/mask/filter, queries, C_q, internal
# Arr/Frechet sums, and wall time — enough for a first D0 (off) vs D1 (on) read.
ROOT="/mnt/c/Users/shiju/Downloads/Discrete-Frechet-distance-under-translation-master/Discrete-Frechet-distance-under-translation-master"
C2="$HOME/b_candidate2/calc_frechet_distance_under_translation"
DATA="$HOME/geodata"
MAN="$ROOT/test_cases/geolife_100/manifest.txt"
LIMIT="${1:-30}"
ALG="${2:-fut_lmf}"

agg() {  # mode -> one summary line block, reading many process outputs on stdin
  awk -v mode="$1" -v wall="$2" -v npair="$3" '
    /\[sweep-stats\]/ { for(i=1;i<=NF;i++){ if(split($i,kv,"=")==2) v[kv[1]]=kv[2] }
      calls+=v["calls"]; m+=v["before"]; K+=v["after"];
      gen+=v["gen_ms"]; msk+=v["mask_ms"]; flt+=v["filter_ms"];
      npts+=v["npts"]; degen+=v["degen"]; q+=v["queries"] }
    /\[frec-stats\]/ { for(i=1;i<=NF;i++){ if(split($i,kv,"=")==2) f[kv[1]]=kv[2] }
      fcalls+=f["calls"]; floop+=f["loop_ms"] }
    /Arrangement computation of n/ { if(match($0,/sum = [0-9.eE+-]+/)) arr+=substr($0,RSTART+6)+0 }
    /computation of n/ && !/Arrangement/ { if(match($0,/sum = [0-9.eE+-]+/)) fre+=substr($0,RSTART+6)+0 }
    END {
      printf "== %-9s ==  pairs=%d  wall=%.2fs\n", mode, npair, wall;
      printf "   builds(calls)=%d  m=%d  K=%d  K/m=%.3f  degen/npts=%.3f\n",
             calls, m, K, (m?K/m:0), (npts?degen/npts:0);
      printf "   setup_ms: gen=%.1f mask=%.1f filter=%.1f (sum=%.1f)\n",
             gen, msk, flt, gen+msk+flt;
      printf "   predicate: queries=%d  frec_loop_ms=%.1f  C_q=%.0f ns\n",
             q, floop, (q?floop*1e6/q:0);
      printf "   MEASUREMENT: Arr_sum=%.1f ms  Frechet_sum=%.1f ms  internal_total=%.1f ms\n\n",
             arr, fre, arr+fre;
    }'
}

for mode in on off mask-only; do
  tmp="$HOME/_x1/main_$mode.txt"; mkdir -p "$HOME/_x1"; : > "$tmp"
  n=0; t0=$EPOCHREALTIME
  while read -r nrep f1 f2; do
    [ -f "$DATA/$f1" ] && [ -f "$DATA/$f2" ] || continue
    MAXIMAL_MODE="$mode" "$C2" "$DATA/$f1" "$DATA/$f2" "$ALG" >> "$tmp" 2>/dev/null
    n=$((n+1)); [ "$n" -ge "$LIMIT" ] && break
  done < "$MAN"
  t1=$EPOCHREALTIME
  wall=$(awk -v a="$t0" -v b="$t1" 'BEGIN{printf "%.2f", b-a}')
  agg "$mode" "$wall" "$n" < "$tmp"
done
