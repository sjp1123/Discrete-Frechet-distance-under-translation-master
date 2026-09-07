#!/bin/bash
# F1 control: run the SAME CGAL_PROFILE binary + same alg (fut_lmf) on a
# general-position SYNTHETIC pair, and parse the Lazy_kernel DAG-depth histogram.
# Compare depth-0 (interval filter resolves cheaply) fraction to the degenerate
# geolife pair. Design thesis: degenerate -> tiny depth-0 frac (pervasive exact
# fallback); general position -> large depth-0 frac.
ROOT="/mnt/c/Users/shiju/Downloads/Discrete-Frechet-distance-under-translation-master/Discrete-Frechet-distance-under-translation-master"
BIN="$HOME/b_original_prof/calc_frechet_distance_under_translation"
DATA="$HOME/geodata"
DEST="$ROOT/_x1_results"; mkdir -p "$DEST"

echo "=== available synthetic curve dirs ==="
for d in _wall _nsweep; do echo "[$d]"; ls "$ROOT/$d" 2>/dev/null | head -8; done

# pick a general-position synthetic pair from _wall (E4 generator) if present,
# else fall back to _nsweep n=16.
SA=""; SB=""
if ls "$ROOT/_wall"/*_a.txt >/dev/null 2>&1; then
  SA=$(ls "$ROOT/_wall"/*_a.txt | sort | tail -1); SB="${SA%_a.txt}_b.txt"
elif ls "$ROOT/_nsweep"/n0016*_a.txt >/dev/null 2>&1; then
  SA=$(ls "$ROOT/_nsweep"/n0016*_a.txt | head -1); SB="${SA%_a.txt}_b.txt"
fi
echo; echo "chosen synthetic: $SA + $SB"

parse_hist() {  # stdin raw stderr -> "depth0_frac total n_gmpq_ops"
  awk '
    /Lazy_kernel DAG depths/ {
      if (match($0, /\[ *([0-9]+|Total) *: *([0-9.]+) *\]/, mm)) {
        key=mm[1]; val=mm[2];
        if (key=="Total") total=val;
        else { per[key]=val; sum+=val; if(key+0==0) d0=val; }
      }
    }
    /Profile_counter/ && /Lazy_exact_nt/ && (/operator\+/||/operator\*/||/operator-/||/operator\//||/Square/) {
      if (match($0, /([0-9.]+) calls/, cc)) gmpq+=cc[1];
    }
    END{
      # per-depth values are in units of 1000 (sum*1000 ~= total)
      printf "%.4f %s %.0f", (sum>0? d0/sum:0), total, gmpq*1000;
    }'
}

# degenerate geolife pair (reuse first manifest pair)
read -r nrep g1 g2 < "$ROOT/test_cases/geolife_100/manifest.txt"
GRAW="$DEST/f1_geo_raw.txt"; "$BIN" "$DATA/$g1" "$DATA/$g2" fut_lmf >/dev/null 2>"$GRAW"
GEO=$(parse_hist < "$GRAW")

SYN="n/a"
if [ -n "$SA" ] && [ -f "$SA" ] && [ -f "$SB" ]; then
  SRAW="$DEST/f1_syn_raw.txt"; "$BIN" "$SA" "$SB" fut_lmf >/dev/null 2>"$SRAW"
  SYN=$(parse_hist < "$SRAW")
fi

echo
echo "=== F1 comparison (depth0_frac  hist_total  approx_gmpq_ops) ==="
echo "  DEGENERATE geolife ($g1+$g2): $GEO"
echo "  GENERAL-POS synthetic       : $SYN"
