#!/bin/bash
# Resumable geolife 3-way wall-clock chunk. timeout-guarded per binary (a pair that
# hangs is recorded as ERR and skipped). Heartbeat to _geo_hb.log; results to RAW.
ROOT="/mnt/c/Users/shiju/Downloads/Discrete-Frechet-distance-under-translation-master/Discrete-Frechet-distance-under-translation-master"
DATA="$HOME/geodata"
MAN="$ROOT/test_cases/geolife_100/manifest.txt"
O="$HOME/b_original/calc_frechet_distance_under_translation"
C1="$HOME/b_candidate/calc_frechet_distance_under_translation"
C2="$HOME/b_candidate2/calc_frechet_distance_under_translation"
RAW="$ROOT/_wall_geo.raw"
HB="$ROOT/_geo_hb.log"
CHUNK="${1:-12}"
TL="${2:-30}"
touch "$RAW"

declare -A DONE
while read -r _n _o _a _b _d _e _f key; do [ -n "$key" ] && DONE[$key]=1; done < "$RAW"

run(){ # $1=bin -> RT(ms), RD(dist or ERRrc)
  local t0 t1 out rc
  t0=${EPOCHREALTIME/./}
  out=$(timeout "$TL" "$1" "$A" "$B" fut_lmf 2>/dev/null); rc=$?
  t1=${EPOCHREALTIME/./}
  RT=$(( (10#$t1 - 10#$t0)/1000 ))
  if [ $rc -ne 0 ]; then RD="ERR$rc"; else out="${out#*is: }"; RD="${out%%$'\n'*}"; fi
}

echo "== chunk start $(date +%T) chunk=$CHUNK tl=$TL have=$(wc -l < "$RAW") ==" >> "$HB"
done=0
while read -r nrep f1 f2; do
  key="${f1}+${f2}"
  [ -n "${DONE[$key]}" ] && continue
  A="$DATA/$f1"; B="$DATA/$f2"; [ -f "$A" ] || continue
  run "$O";  ot=$RT;  od=$RD
  run "$C1"; c1t=$RT; c1d=$RD
  run "$C2"; c2t=$RT; c2d=$RD
  echo "$nrep $ot $c1t $c2t $od $c1d $c2d $key" >> "$RAW"
  echo "$(date +%T) +pair n=$nrep O=${ot} C1=${c1t} C2=${c2t} ($key)" >> "$HB"
  done=$((done+1)); [ "$done" -ge "$CHUNK" ] && break
done < "$MAN"
echo "== chunk end $(date +%T) +$done have=$(wc -l < "$RAW") ==" >> "$HB"
