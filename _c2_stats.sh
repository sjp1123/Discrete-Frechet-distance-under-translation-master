#!/bin/bash
ROOT="/mnt/c/Users/shiju/Downloads/Discrete-Frechet-distance-under-translation-master/Discrete-Frechet-distance-under-translation-master"
C2="$HOME/b_candidate2/calc_frechet_distance_under_translation"
DATA="$ROOT/original/test_data/benchmark/Geolife Trajectories 1.3/data"
MAN="$ROOT/test_cases/geolife_small/manifest.txt"
OUT="$ROOT/_c2_stats.out"

dump() {  # aggregate [sweep-stats] + Arr/Frechet sums from many process outputs
  awk -v label="$1" '
    /\[sweep-stats\]/ {
      for(i=1;i<=NF;i++){ if(split($i,kv,"=")==2) v[kv[1]]=kv[2] }
      calls+=v["calls"]; before+=v["before"]; after+=v["after"];
      gen+=v["gen_ms"]; msk+=v["mask_ms"]; flt+=v["filter_ms"];
      npts+=v["npts"]; degen+=v["degen"]; queries+=v["queries"];
    }
    /Arrangement computation of n/ { if(match($0,/sum = [0-9.eE+-]+/)) arr+=substr($0,RSTART+6)+0 }
    /computation of n/ && !/Arrangement/ { if(match($0,/sum = [0-9.eE+-]+/)) fre+=substr($0,RSTART+6)+0 }
    END{
      printf "== %s ==\n", label;
      printf "  calls=%d  m(dedup)=%d  K(survivors)=%d  K/m=%.3f\n", calls, before, after, (before? after/before:0);
      printf "  npts=%d  degen(hi!=lo)=%d  degen/npts=%.3f\n", npts, degen, (npts? degen/npts:0);
      printf "  queries(getNext)=%d  Frechet_sum=%.3f ms  C_q=%.0f ns/query\n", queries, fre, (queries? fre*1e6/queries:0);
      printf "  Arr(setup)_sum=%.3f ms  [gen=%.1f mask=%.1f filter=%.1f]\n", arr, gen, msk, flt;
      printf "\n";
    }'
}

# ---- main path (N<=12): fut_lmf over up to 40 geolife pairs ----
{
  n=0
  while read -r nrep f1 f2; do
    [ -f "$DATA/$f1" ] || continue
    "$C2" "$DATA/$f1" "$DATA/$f2" fut_lmf 2>/dev/null
    n=$((n+1)); [ $n -ge 40 ] && break
  done < "$MAN"
} | dump "fut_lmf  (MAIN path, N<=12)  40 geolife pairs" | tee "$OUT"

# ---- global path (N=mn): n6 over a few nsweep sizes ----
{
  for f in "$ROOT/_nsweep"/n0008_p0_a.txt "$ROOT/_nsweep"/n0012_p0_a.txt "$ROOT/_nsweep"/n0016_p0_a.txt; do
    b="${f%_a.txt}_b.txt"
    "$C2" "$f" "$b" n6 2>/dev/null
  done
} | dump "n6  (GLOBAL path, N=mn)  nsweep n=8,12,16" | tee -a "$OUT"

echo "saved -> $OUT" >&2
