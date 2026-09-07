#!/bin/bash
# Independent perf spot-check: candidate2 vs candidate4(cech), min-of-2 wall, and
# decider-call count from [sweep-stats] queries (both emit it; = getNext calls).
ROOT="/mnt/c/Users/shiju/Downloads/Discrete-Frechet-distance-under-translation-master/Discrete-Frechet-distance-under-translation-master"
DATA="$HOME/geodata"; MAN="$ROOT/test_cases/geolife_100/manifest.txt"
C4="$HOME/b_candidate4/calc_frechet_distance_under_translation"
C2="$HOME/b_candidate2/calc_frechet_distance_under_translation"
LIMIT="${1:-25}"
q(){ grep -E "\[sweep-stats\]" | tail -1 | sed -E 's/.*queries=([0-9]+).*/\1/'; }
run(){ # $1 bin, $2 env -> "wall_ms queries" (min wall over 2 reps)
  local best=99999999 bq=0
  for r in 1 2; do
    t0=$EPOCHREALTIME
    out=$(env $2 "$1" "$DATA/$f1" "$DATA/$f2" fut_lmf 2>/dev/null)
    t1=$EPOCHREALTIME
    w=$(awk -v a="$t0" -v b="$t1" 'BEGIN{printf "%.2f",(b-a)*1000}')
    qq=$(printf '%s\n' "$out" | q); qq=${qq:-0}
    awk -v w="$w" -v b="$best" 'BEGIN{exit !(w<b)}' && { best=$w; bq=$qq; }
  done
  echo "$best $bq"
}
n=0; sw2=0; sw4=0; sq2=0; sq4=0; faster=0
while read -r nrep f1 f2; do
  [ -f "$DATA/$f1" ] && [ -f "$DATA/$f2" ] || continue
  n=$((n+1)); [ "$n" -gt "$LIMIT" ] && break
  r2=($(run "$C2" "")); r4=($(run "$C4" "MAXREGION=cech"))
  sw2=$(awk -v a="$sw2" -v b="${r2[0]}" 'BEGIN{printf "%.2f",a+b}')
  sw4=$(awk -v a="$sw4" -v b="${r4[0]}" 'BEGIN{printf "%.2f",a+b}')
  sq2=$((sq2 + ${r2[1]})); sq4=$((sq4 + ${r4[1]}))
  awk -v x="${r4[0]}" -v y="${r2[0]}" 'BEGIN{exit !(x<y)}' && faster=$((faster+1))
done < "$MAN"
echo "pairs=$n  faster(c4<c2)=$faster/$n"
echo "WALL sum: c2=${sw2}ms  c4=${sw4}ms  ratio c2/c4=$(awk -v a=$sw2 -v b=$sw4 'BEGIN{printf "%.2f",a/b}')x"
echo "decider calls (Σ queries): c2=$sq2  c4=$sq4  ratio=$(awk -v a=$sq2 -v b=$sq4 'BEGIN{printf "%.2f",(b?a/b:0)}')x"
