#!/bin/bash
# Same end-to-end fut_lmf WALL-CLOCK 3-way as _bench_wall3.sh, but on REAL Geolife
# pairs (test_cases/geolife_100, n in [100,1000]).
#   O  = original   (no maximal)         C1 = candidate (maximal, CGAL)   C2 = candidate2 (maximal, float)
#   O/C1 = adding maximal only;  O/C2 = with float too.
ROOT="/mnt/c/Users/shiju/Downloads/Discrete-Frechet-distance-under-translation-master/Discrete-Frechet-distance-under-translation-master"
DATA="$ROOT/original/test_data/benchmark/Geolife Trajectories 1.3/data"
MAN="$ROOT/test_cases/geolife_100/manifest.txt"
O="$HOME/b_original/calc_frechet_distance_under_translation"
C1="$HOME/b_candidate/calc_frechet_distance_under_translation"
C2="$HOME/b_candidate2/calc_frechet_distance_under_translation"
OUT="$ROOT/_wall_geo.out"; RAW="$ROOT/_wall_geo.raw"
REPS=1

dist(){ awk 'match($0,/is: [0-9.eE+-]+/){v=substr($0,RSTART+4)}END{print v+0}'; }
best(){ local bin="$1" mn=99999999 out d ms t0 t1
  for r in $(seq 1 $REPS); do
    t0=$(date +%s%N); out=$("$bin" "$A" "$B" fut_lmf 2>/dev/null); t1=$(date +%s%N)
    ms=$(( (t1-t0)/1000000 )); [ $ms -lt $mn ] && mn=$ms; d=$(printf '%s' "$out"|dist)
  done; echo "$mn|$d"; }

: > "$RAW"; i=0
while read -r nrep f1 f2; do
  A="$DATA/$f1"; B="$DATA/$f2"; [ -f "$A" ] || continue
  r=$(best "$O");  ot=${r%|*};  od=${r#*|}
  r=$(best "$C1"); c1t=${r%|*}; c1d=${r#*|}
  r=$(best "$C2"); c2t=${r%|*}; c2d=${r#*|}
  echo "$nrep $ot $c1t $c2t $od $c1d $c2d" >> "$RAW"
  i=$((i+1)); [ $((i%20)) -eq 0 ] && echo "...$i/100 done" >&2
done < "$MAN"

awk '{
  n=$1; b=int(n/100)*100; o[b]+=$2; a[b]+=$3; c[b]+=$4; k[b]++; seen[b]=1;
  d1=$5-$6; if(d1<0)d1=-d1; if(d1>1e-6) m1[b]++;
  d2=$5-$7; if(d2<0)d2=-d2; if(d2>1e-6) m2[b]++;
}
END{
  x=0; for(bb in seen) order[x++]=bb;
  for(i=0;i<x;i++)for(j=i+1;j<x;j++) if(order[i]+0>order[j]+0){t=order[i];order[i]=order[j];order[j]=t}
  printf "%-10s %4s | %8s %8s %8s | %11s %12s | %s\n","n-bucket","cnt","O_ms","C1_ms","C2_ms","O/C1 (max)","O/C2 (both)","mism C1/C2";
  print  "----------------------------------------------------------------------------------------------------";
  to=ta=tc=0;
  for(i=0;i<x;i++){ bb=order[i];
    printf "%-10s %4d | %8d %8d %8d | %10.2fx %11.2fx | %d/%d\n",
      bb"-"(bb+99),k[bb],o[bb],a[bb],c[bb],(a[bb]?o[bb]/a[bb]:0),(c[bb]?o[bb]/c[bb]:0),m1[bb]+0,m2[bb]+0;
    to+=o[bb]; ta+=a[bb]; tc+=c[bb];
  }
  print  "----------------------------------------------------------------------------------------------------";
  printf "%-10s %4s | %8d %8d %8d | %10.2fx %11.2fx |\n","TOTAL","",to,ta,tc,(ta?to/ta:0),(tc?to/tc:0);
  print  "";
  print  "O/C1 = end-to-end effect of adding the maximal filter (CGAL) to fut_lmf on REAL Geolife.";
}' "$RAW" | tee "$OUT"
echo "saved -> $OUT" >&2
