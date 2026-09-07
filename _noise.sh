#!/bin/bash
# Timing-noise characterization (credibility item #2): quantify run-to-run wall
# variance and the process-overhead floor, and show the internal MEASUREMENT
# timer is noise-immune relative to wall. 8 pairs x R=10 reps, arms D1 & C4.
ROOT="/mnt/c/Users/shiju/Downloads/Discrete-Frechet-distance-under-translation-master/Discrete-Frechet-distance-under-translation-master"
DATA="$HOME/geodata"; MAN="$ROOT/test_cases/geolife_100/manifest.txt"
C2="$HOME/b_candidate2/calc_frechet_distance_under_translation"
C4="$HOME/b_candidate4/calc_frechet_distance_under_translation"
R="${1:-10}"; NP="${2:-8}"
internal(){ awk '/Arrangement computation of n/{if(match($0,/sum = [0-9.eE+-]+/))a=substr($0,RSTART+6)+0}
  /computation of n/&&!/Arrangement/{if(match($0,/sum = [0-9.eE+-]+/))f=substr($0,RSTART+6)+0}
  END{printf "%.4f", a+f}'; }
run_arm(){ # $1 label, $2 bin, $3 env
  local n=0
  while read -r nrep f1 f2; do
    [ -f "$DATA/$f1" ] && [ -f "$DATA/$f2" ] || continue
    n=$((n+1)); [ "$n" -gt "$NP" ] && break
    walls=(); intl=0
    for r in $(seq 1 "$R"); do
      t0=$EPOCHREALTIME
      out=$(env $3 "$2" "$DATA/$f1" "$DATA/$f2" fut_lmf 2>/dev/null)
      t1=$EPOCHREALTIME
      walls+=("$(awk -v a="$t0" -v b="$t1" 'BEGIN{printf "%.3f",(b-a)*1000}')")
      intl=$(printf '%s\n' "$out" | internal)
    done
    printf '%s\n' "${walls[@]}" | awk -v lbl="$1" -v pid="${f1%.txt}_${f2%.txt}" -v intl="$intl" '
      {x[NR]=$1; s+=$1; if(NR==1||$1<mn)mn=$1; if(NR==1||$1>mx)mx=$1}
      END{m=s/NR; for(i=1;i<=NR;i++)v+=(x[i]-m)^2; sd=sqrt(v/(NR-1));
        printf "%-4s %-16s wall_mean=%8.2f  CV=%5.1f%%  min=%8.2f  max=%8.2f  internal=%8.2f  overhead=%7.2f (%.0f%%)\n",
          lbl, pid, m, 100*sd/m, mn, mx, intl+0, m-intl, 100*(m-intl)/m }'
  done < "$MAN"
}
echo "=== per-pair wall variance over R=$R reps (wall in ms) ==="
run_arm D1 "$C2" ""              | tee /tmp/_noise_d1
run_arm C4 "$C4" "MAXREGION=cech" | tee /tmp/_noise_c4
echo
echo "=== summary: median wall CV, overhead floor ==="
cat /tmp/_noise_d1 /tmp/_noise_c4 | awk '
  match($0,/CV= *([0-9.]+)%/,c){cv[++n]=c[1]}
  match($0,/overhead= *([0-9.-]+)/,o){oh[n]=o[1]}
  END{asort(cv); asort(oh);
    printf "  wall CV: median=%.1f%%  min=%.1f%%  max=%.1f%%  (n=%d pair-arms)\n", cv[int(n/2)], cv[1], cv[n], n;
    printf "  process overhead (wall - internal): median=%.0f ms  min=%.0f  max=%.0f\n", oh[int(n/2)], oh[1], oh[n] }'
