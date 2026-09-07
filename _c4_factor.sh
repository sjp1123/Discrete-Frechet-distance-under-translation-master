#!/bin/bash
# Factor separation: how much of candidate4's win is the pipeline (one witness
# per region) and how much is the §5 slack alignment?
#   c2            candidate2
#   legacy        candidate4, arrangement emitter, no slack   (== c2)
#   legacy+slack  candidate4, arrangement emitter, with slack (slack only)
#   cech          candidate4, pipeline, no slack              (pipeline only)
#   cech+slack    candidate4, default                         (both)
C2="$HOME/b_candidate2/calc_frechet_distance_under_translation"
C4="$HOME/b_candidate4/calc_frechet_distance_under_translation"
D="$HOME/geodata"
PAIRS="13962.txt:4238.txt 3110.txt:11704.txt 10725.txt:2829.txt 15818.txt:12700.txt 14551.txt:3143.txt 5431.txt:2218.txt"

row() { # $1=label  rest=env assignments then binary
  local label="$1"; shift
  local t0=$EPOCHREALTIME out
  out=$(env "$@" "$D/$A" "$D/$B" fut_lmf 2>/dev/null)
  local t1=$EPOCHREALTIME
  local wall; wall=$(awk -v a="$t0" -v b="$t1" 'BEGIN{printf "%8.1f",(b-a)*1000}')
  local ans; ans=$(printf '%s\n' "$out" | grep -a "calcDistance2" | grep -aoE "[0-9]+\.[0-9]+" | tail -1)
  local st;  st=$(printf '%s\n' "$out" | grep -a "\[sweep-stats\]" | tail -1)
  printf "  %-14s wall=%s ms  builds=%-5s queries=%-6s ans=%s\n" "$label" "$wall" \
    "$(echo "$st" | grep -aoE 'calls=[0-9]+' | cut -d= -f2)" \
    "$(echo "$st" | grep -aoE 'queries=[0-9]+' | cut -d= -f2)" "$ans"
}

for pr in $PAIRS; do
  A="${pr%%:*}"; B="${pr##*:}"
  echo "########## $A $B ##########"
  row "c2"           "$C2"
  row "legacy"       MAXREGION=legacy "$C4"
  row "legacy+slack" MAXREGION=legacy MAXREGION_LEGACY_SLACK=1 "$C4"
  row "cech"         MAXREGION=cech MAXREGION_SLACK=0 "$C4"
  row "cech+slack"   MAXREGION=cech "$C4"
done
