#!/usr/bin/env python3
"""From the oracle output, write decider query files per family with the expected answer of each query.
query file line : <a> <b> <delta>            (paper_bench decider input)
truth file line : <id> <delta> <YES|NO|EITHER> <kind>
YES    : delta >= opt (exact comparison delta^2 >= opt^2)
NO     : delta < opt - band, band = 1.5e-8 + 1e-15 * max|coordinate|  (the implementations accept a slack of 9e-9
         plus the rounding of the translated coordinates)
EITHER : opt - band <= delta < opt
"""
import math, os, sys
from fractions import Fraction

D = sys.argv[1]
res = {}
for line in open(os.path.join(D, "oracle_out.txt")):
    p = line.split()
    res[p[0]] = (float(p[5]), Fraction(int(p[6]), int(p[7])))

fams = sorted({k.split("/")[0] for k in res})
for fam in fams:
    q = open(os.path.join(D, f"{fam}.q"), "w")
    t = open(os.path.join(D, f"{fam}.truth"), "w")
    for k in sorted(x for x in res if x.startswith(fam + "/")):
        i = k.split("/")[1]
        opt, opt2 = res[k]
        M = 0.0
        for s in ("a", "b"):
            for line in open(os.path.join(D, fam, f"{i}_{s}.txt")):
                x, y = map(float, line.split())
                M = max(M, abs(x), abs(y))
        band = 1.5e-8 + 1e-15 * M
        up = opt
        while Fraction(up) ** 2 < opt2:
            up = math.nextafter(up, math.inf)
        ds = [(opt * 2, "x2"), (opt * (1 + 1e-2), "p1e-2"), (opt * (1 + 1e-4), "p1e-4"), (opt * (1 + 1e-6), "p1e-6"),
              (up, "tie_up"), (up + 1e-12 * max(1.0, opt), "tie_up+"), (opt + 2e-8, "p2e-8abs")]
        if opt > 0:
            ds += [(opt * 0.5, "half"), (opt * (1 - 1e-2), "m1e-2"), (opt * (1 - 1e-4), "m1e-4"), (opt * (1 - 1e-6), "m1e-6"),
                   (opt - 5e-8, "m5e-8abs"), (opt - 2e-7, "m2e-7abs"), (math.nextafter(up, 0.0), "tie_dn")]
        else:
            ds += [(0.0, "zero")]
        for d, kind in ds:
            if d < 0:
                continue
            if Fraction(d) ** 2 >= opt2:
                cls = "YES"
            elif d < opt - band:
                cls = "NO"
            else:
                cls = "EITHER"
            q.write(f"{i}_a.txt {i}_b.txt {d!r}\n")
            t.write(f"{k} {d!r} {cls} {kind}\n")
    q.close(); t.close()
print("families", len(fams))
