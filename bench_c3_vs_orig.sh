#!/bin/bash
# original (CGAL arrangement, no maximal filter)  vs  candidate3 (exact predicates + band)
#
# 기존 bench_*.sh 는 전부 candidate2 를 대상으로 하고, bench_exact.sh 는 요약표에
# original 시간을 넣지 않는다. 이 스크립트가 그 빈자리를 채운다.
#
# 사용법:
#   ./bench_c3_vs_orig.sh [테스트디렉터리] [알고리즘]
#   기본값: test_cases/bench_100  fut_lmf
#
# 하드코딩된 절대경로 대신 스크립트 위치를 기준으로 동작한다.

set -u

ROOT="$(cd "$(dirname "$0")" && pwd)"
O="$ROOT/original/build/calc_frechet_distance_under_translation"
C3="$ROOT/candidate3/build/calc_frechet_distance_under_translation"
DIR="${1:-$ROOT/test_cases/bench_100}"
ALG="${2:-fut_lmf}"
EPS=1e-7

for b in "$O" "$C3"; do
  [ -x "$b" ] || { echo "빌드가 필요합니다: $b" >&2; exit 1; }
done
[ -f "$DIR/manifest.txt" ] || { echo "manifest 를 찾을 수 없습니다: $DIR/manifest.txt" >&2; exit 1; }

TMP=$(mktemp)
trap 'rm -f "$TMP"' EXIT

# 거리값 추출: fut_lmf 는 "... for LMF is: X", n6 는 "... for N6 is: X"
getd()   { grep 'is:' | sed 's/.*is: //' | tail -1; }
# 내부 계측: arrangement 구축에만 쓰인 시간
arr_ms() { awk '/Arrangement computation of n/ { if (match($0,/sum = [0-9.eE+-]+/)) a+=substr($0,RSTART+6)+0 } END{printf "%.4f",a}'; }

i=0
while read -r tag n; do
  A="$DIR/${tag}_a.txt"; B="$DIR/${tag}_b.txt"
  [ -f "$A" ] || continue

  # 내부 arrangement 시간 + 거리값 (stdout 캡처)
  oo=$("$O"  "$A" "$B" "$ALG" 2>/dev/null); od=$(echo "$oo" | getd); oa=$(echo "$oo" | arr_ms)
  co=$("$C3" "$A" "$B" "$ALG" 2>/dev/null); cd3=$(echo "$co" | getd); ca=$(echo "$co" | arr_ms)

  # wall clock 은 stdout 을 버린 별도 실행으로 측정한다.
  # candidate3 는 판정마다 "candidate count = ..." 를 찍으므로, 캡처하면
  # 출력량 차이가 그대로 시간 차이로 섞여 들어간다.
  s=$(date +%s.%N); "$O"  "$A" "$B" "$ALG" >/dev/null 2>&1; e=$(date +%s.%N)
  om=$(awk -v s="$s" -v e="$e" 'BEGIN{printf "%.3f",(e-s)*1000}')
  s=$(date +%s.%N); "$C3" "$A" "$B" "$ALG" >/dev/null 2>&1; e=$(date +%s.%N)
  cm=$(awk -v s="$s" -v e="$e" 'BEGIN{printf "%.3f",(e-s)*1000}')

  echo "$n $om $cm $oa $ca $od $cd3 $tag" >> "$TMP"
  i=$((i+1)); if [ $((i % 20)) -eq 0 ]; then echo "...$i done" >&2; fi
done < "$DIR/manifest.txt"

echo
echo "======== original vs candidate3 ($ALG, $(basename "$DIR")) ========"
printf "%-10s %6s %11s %11s %8s %11s %11s %8s %6s\n" \
       "n-bucket" "count" "orig(ms)" "c3(ms)" "speedup" "orig-arr" "c3-arr" "arr-sp" "mism"
echo "---------------------------------------------------------------------------------------------"
awk -v eps="$EPS" '
{
  n=$1; om=$2; cm=$3; oa=$4; ca=$5; od=$6; cd=$7; tag=$8;
  b=int(n/100)*100;
  cnt[b]++; osum[b]+=om; csum[b]+=cm; oasum[b]+=oa; casum[b]+=ca;
  d=od-cd; if(d<0)d=-d;
  if(d>eps){ mm[b]++; mmtot++;
    print "  MISMATCH", tag, "n="n, "orig="od, "c3="cd, "|diff|="d > "/dev/stderr" }
  tco+=om; tcc+=cm; toa+=oa; tca+=ca; tcnt++;
}
END{
  for(b=0;b<=100000;b+=100) if(cnt[b]>0){
    sp =(csum[b] >0)?osum[b] /csum[b] :0;
    asp=(casum[b]>0)?oasum[b]/casum[b]:0;
    printf "%-10s %6d %11.0f %11.0f %7.2fx %11.0f %11.0f %7.2fx %6d\n",
           b"-"(b+99), cnt[b], osum[b], csum[b], sp, oasum[b], casum[b], asp, mm[b]+0;
  }
  print "---------------------------------------------------------------------------------------------";
  sp =(tcc>0)?tco/tcc:0; asp=(tca>0)?toa/tca:0;
  printf "%-10s %6d %11.0f %11.0f %7.2fx %11.0f %11.0f %7.2fx %6d\n",
         "TOTAL", tcnt, tco, tcc, sp, toa, tca, asp, mmtot+0;
  print "";
  print "speedup = original/candidate3 (1 초과면 candidate3 가 빠름)";
  print "arr-sp  = arrangement 구축 구간만 비교";
  print "mism    = |orig - c3| > " eps " 인 케이스 수 (0 이어야 정상)";
}' "$TMP"
