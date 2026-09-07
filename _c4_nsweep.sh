#!/bin/bash
# Candidate 4 on the GLOBAL n6 path (discs = n1*n2, so the disc set grows with
# the curve length instead of being capped at CUT_LIMIT).  This is the regime
# where the 2^m DP hits its component-size limit and hands work back to the
# arrangement path, so it is the honest stress test for the pipeline.
#
# Usage: _c4_nsweep.sh [REPS=3] [TIMEOUT=60]
ROOT="/mnt/c/Users/shiju/Downloads/Discrete-Frechet-distance-under-translation-master/Discrete-Frechet-distance-under-translation-master"
DIR="$ROOT/_nsweep"
REPS="${1:-3}"
TL="${2:-60}"
OUT="$HOME/_c4/nsweep.csv"; mkdir -p "$HOME/_c4"

ARMS="D1 C4S C4N C4"
declare -A BIN
BIN[D1]="$HOME/b_candidate2/calc_frechet_distance_under_translation"
BIN[C4S]="$HOME/b_candidate4/calc_frechet_distance_under_translation"
BIN[C4N]="$HOME/b_candidate4/calc_frechet_distance_under_translation"
BIN[C4]="$HOME/b_candidate4/calc_frechet_distance_under_translation"
armenv() {
  case "$1" in
    C4S) echo "MAXREGION=legacy MAXREGION_LEGACY_SLACK=1";;
    C4N) echo "MAXREGION=cech MAXREGION_SLACK=0";;
    C4)  echo "MAXREGION=cech";;
    *)   echo "";;
  esac
}

TMP=$(mktemp); trap 'rm -f "$TMP"' EXIT
echo "n,p,arm,ans,status,wall_ms,arr_ms,fre_ms,builds,queries,regions,overflow,max_comp" > "$OUT"

parse() {
  awk '
    /Arrangement computation of n/ { if(match($0,/sum = [0-9.eE+-]+/)) arr=substr($0,RSTART+6)+0 }
    /computation of n/ && !/Arrangement/ { if(match($0,/sum = [0-9.eE+-]+/)) fre=substr($0,RSTART+6)+0 }
    /\[sweep-stats\]/    { for(i=1;i<=NF;i++) if(split($i,kv,"=")==2) v[kv[1]]=kv[2] }
    /\[maxregion-stats\]/{ for(i=1;i<=NF;i++) if(split($i,kw,"=")==2) w[kw[1]]=kw[2] }
    END{ printf "%.4f,%.4f,%d,%d,%d,%d,%d", arr+0, fre+0, v["calls"]+0, v["queries"]+0,
                w["regions"]+0, w["overflow"]+0, w["max_comp"]+0 }' "$1"
}
ansof() { awk 'match($0,/is: [0-9.eE+-]+/){v=substr($0,RSTART+4)}END{print v+0}' "$1"; }

for f in "$DIR"/n*_a.txt; do
  b="${f%_a.txt}_b.txt"; base=$(basename "$f" _a.txt)
  n=${base%%_*}; n=$((10#${n#n})); p=${base##*_p}
  declare -A best line
  for r in $(seq 1 "$REPS"); do
    for arm in $(printf '%s\n' $ARMS | shuf); do
      t0=$EPOCHREALTIME
      env $(armenv "$arm") timeout "${TL}s" "${BIN[$arm]}" "$f" "$b" n6 >"$TMP" 2>/dev/null
      rc=$?
      t1=$EPOCHREALTIME
      if [ "$rc" -ne 0 ]; then
        [ -z "${best[$arm]}" ] && { best[$arm]=$((TL*1000)); line[$arm]="$n,$p,$arm,NA,TIMEOUT,${TL}000,NA,NA,NA,NA,NA,NA,NA"; }
        continue
      fi
      wall=$(awk -v a="$t0" -v b="$t1" 'BEGIN{printf "%.2f",(b-a)*1000}')
      cur=${best[$arm]:-99999999}
      if awk -v w="$wall" -v c="$cur" 'BEGIN{exit !(w<c)}'; then
        best[$arm]=$wall
        line[$arm]="$n,$p,$arm,$(ansof "$TMP"),OK,$wall,$(parse "$TMP")"
      fi
    done
  done
  for arm in $ARMS; do echo "${line[$arm]}"; done >> "$OUT"
  unset best line
  echo "  n=$n p=$p done" >&2
done

echo "wrote $OUT" >&2
cp "$OUT" "$ROOT/_c4_nsweep.csv" 2>/dev/null

# compact table
awk -F, 'NR>1 && $5=="OK" { k=$1"_"$2; w[k"|"$3]=$6; a[k"|"$3]=$7; f[k"|"$3]=$8;
                            q[k"|"$3]=$10; o[k"|"$3]=$12; seen[k]=1 }
  END{ printf "%-8s %10s %10s %8s %10s %10s %8s %8s\n","n_p","D1 wall","C4 wall","x","D1 int","C4 int","x","C4 ovf";
       n=asorti(seen,ks);
       for(i=1;i<=n;i++){ k=ks[i];
         d=w[k"|D1"]; c=w[k"|C4"]; di=a[k"|D1"]+f[k"|D1"]; ci=a[k"|C4"]+f[k"|C4"];
         printf "%-8s %10.1f %10.1f %8.2f %10.2f %10.2f %8.2f %8s\n", k, d, c, (c>0?d/c:0),
                di, ci, (ci>0?di/ci:0), o[k"|C4"] } }' "$OUT" 2>/dev/null || true
