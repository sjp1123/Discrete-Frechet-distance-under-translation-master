#!/bin/bash
# End-to-end fut_lmf WALL-CLOCK, 3-way:
#   original   = fut_lmf, NO maximal        (baseline)
#   candidate  = fut_lmf, + maximal (CGAL)  <-- "added maximal only";  O/C1 = the answer
#   candidate2 = fut_lmf, + maximal (float) <-- also swaps arithmetic;  O/C2 = combined
ROOT="/mnt/c/Users/shiju/Downloads/Discrete-Frechet-distance-under-translation-master/Discrete-Frechet-distance-under-translation-master"
DIR="$ROOT/_wall"
O="$HOME/b_original/calc_frechet_distance_under_translation"
C1="$HOME/b_candidate/calc_frechet_distance_under_translation"
C2="$HOME/b_candidate2/calc_frechet_distance_under_translation"
OUT="$ROOT/_wall3.out"
RAW="$ROOT/_wall3.raw"
REPS=2   # take the min of REPS runs to cut noise

dist(){ awk 'match($0,/is: [0-9.eE+-]+/){v=substr($0,RSTART+4)}END{print v+0}'; }
best(){ # $1=bin ; echos "min_ms|distance"
  local bin="$1" mn=99999999 out d
  for r in $(seq 1 $REPS); do
    local t0=$(date +%s%N); out=$("$bin" "$A" "$B" fut_lmf 2>/dev/null); local t1=$(date +%s%N)
    local ms=$(( (t1-t0)/1000000 )); [ $ms -lt $mn ] && mn=$ms
    d=$(printf '%s' "$out"|dist)
  done
  echo "$mn|$d"
}

: > "$RAW"
for f in "$DIR"/n*_a.txt; do
  A="$f"; B="${f%_a.txt}_b.txt"; base=$(basename "$f" _a.txt)
  n=${base%%_*}; n=${n#n}; n=$((10#$n))
  r=$(best "$O");  ot=${r%|*};  od=${r#*|}
  r=$(best "$C1"); c1t=${r%|*}; c1d=${r#*|}
  r=$(best "$C2"); c2t=${r%|*}; c2d=${r#*|}
  echo "$n $ot $c1t $c2t $od $c1d $c2d" >> "$RAW"
  echo "  n=$n  O=${ot}ms  C1=${c1t}ms  C2=${c2t}ms" >&2
done

awk '{
  n=$1; o[n]+=$2; a[n]+=$3; c[n]+=$4; k[n]++; seen[n]=1;
  d1=$5-$6; if(d1<0)d1=-d1; if(d1>1e-6) m1[n]++;
  d2=$5-$7; if(d2<0)d2=-d2; if(d2>1e-6) m2[n]++;
}
END{
  x=0; for(nn in seen) order[x++]=nn;
  for(i=0;i<x;i++)for(j=i+1;j<x;j++) if(order[i]+0>order[j]+0){t=order[i];order[i]=order[j];order[j]=t}
  printf "%-7s %3s | %8s %8s %8s | %11s %12s | %s\n","n","cnt","O_ms","C1_ms","C2_ms","O/C1 (max)","O/C2 (both)","mism C1/C2";
  print  "-------------------------------------------------------------------------------------------";
  to=ta=tc=0;
  for(i=0;i<x;i++){ nn=order[i];
    printf "n=%-5s %3d | %8d %8d %8d | %10.2fx %11.2fx | %d/%d\n",
      nn,k[nn],o[nn],a[nn],c[nn],(a[nn]?o[nn]/a[nn]:0),(c[nn]?o[nn]/c[nn]:0),m1[nn]+0,m2[nn]+0;
    to+=o[nn]; ta+=a[nn]; tc+=c[nn];
  }
  print  "-------------------------------------------------------------------------------------------";
  printf "%-7s %3s | %8d %8d %8d | %10.2fx %11.2fx |\n","TOTAL","",to,ta,tc,(ta?to/ta:0),(tc?to/tc:0);
  print  "";
  print  "O/C1>1 => adding maximal (CGAL) speeds up total fut_lmf;  O/C2 = with float too.";
}' "$RAW" | tee "$OUT"
echo "saved -> $OUT" >&2
