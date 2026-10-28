#!/usr/bin/env python3
"""Write the r6 real-data decider queries at v ± 5e-8 from the c7_verify kit's near-threshold files.
The kit's files hold v + 1e-6 (plus) and v − 1e-6 (minus) per pair, v = candidate6's LMF value; their mean is v.
   make_tight_queries.py [OUT=paper_bench/r6_candidate8/queries]"""
import glob, os, sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..'))
NT = os.path.join(ROOT, 'paper_bench', 'c7_verify', 'cases', 'near_threshold')
OUT = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, 'paper_bench', 'r6_candidate8', 'queries')
os.makedirs(OUT, exist_ok=True)
for p in sorted(glob.glob(os.path.join(NT, '*_plus_*.txt'))):
    plus = [l.split() for l in open(p)]
    minus = [l.split() for l in open(p.replace('_plus_', '_minus_'))]
    assert len(plus) == len(minus)
    name = os.path.basename(p).replace('_c6val_plus_', '_c6val_%s_')
    for tag, off in (('p5e-8', 5e-8), ('m5e-8', -5e-8)):
        with open(os.path.join(OUT, name % tag), 'w') as f:
            for a, b in zip(plus, minus):
                assert a[:2] == b[:2]
                v = (float(a[2]) + float(b[2])) / 2
                f.write(f'{a[0]} {a[1]} {v + off:.17g}\n')
print(f'{len(os.listdir(OUT))} files in {OUT}')
