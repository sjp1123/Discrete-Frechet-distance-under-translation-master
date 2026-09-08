#!/usr/bin/env python3
# Four-arm analysis for _c5_bench_all.sh output: A0 / D1 / C4 / C5, all measured
# in one round-robin on one machine.  Same statistics as _c4_analyze.py.
import csv, math, random, sys
random.seed(20260908)
PATH = sys.argv[1] if len(sys.argv) > 1 else "_c5_bench_all.csv"
ARMS = ['A0', 'D1', 'C4', 'C5']
LABEL = {'A0': 'original (Epeck+DCEL)', 'D1': 'candidate2 (double list)',
         'C4': 'candidate4 (double+Cech)', 'C5': 'candidate5 (Epeck+Cech)'}
d = {}
for r in csv.DictReader(open(PATH)):
    try: r['_int'] = float(r['arr_ms']) + float(r['fre_ms'])
    except (ValueError, TypeError): r['_int'] = None
    d[(r['pair'], r['arm'])] = r
pairs = sorted({k[0] for k in d})

def val(p, a, m, floor=None):
    r = d.get((p, a))
    if not r or r['status'] != 'OK': return None
    v = r['_int'] if m == '_int' else (float(r[m]) if r[m] not in ('', 'NA', None) else None)
    if v is None or v <= 0 or (floor and v < floor): return None
    return v

def boot(logs, B=10000):
    n = len(logs)
    g = sorted(math.exp(sum(logs[random.randrange(n)] for _ in range(n)) / n) for _ in range(B))
    return g[int(.025 * B)], g[int(.975 * B)]

def con(num, den, m, floor=None):
    logs = []
    for p in pairs:
        a, b = val(p, num, m, floor), val(p, den, m, floor)
        if a and b: logs.append(math.log(a / b))
    rs = sorted(math.exp(x) for x in logs)
    lo, hi = boot(logs)
    return math.exp(sum(logs) / len(logs)), lo, hi, rs[len(rs) // 2], sum(1 for x in rs if x > 1), len(rs)

print("== four-arm analysis: %s ==  pairs=%d" % (PATH, len(pairs)))
print("censored: " + "  ".join("%s=%d" % (a, sum(1 for p in pairs if d[(p, a)]['status'] != 'OK')) for a in ARMS))
print("\n-- answers vs A0 (original, exact Epeck) --")
for a in ARMS[1:]:
    ds = [abs(float(d[(p, 'A0')]['ans']) - float(d[(p, a)]['ans'])) for p in pairs
          if d[(p, 'A0')]['status'] == 'OK' and d[(p, a)]['status'] == 'OK']
    print("  %-3s max|d|=%.2e  over-eps(1e-7)=%d/%d  bit-identical=%d"
          % (a, max(ds), sum(1 for x in ds if x > 1e-7), len(ds), sum(1 for x in ds if x == 0)))
for m, lab, fl in [('wall_ms', 'WALL', None), ('arr_ms', 'ARRANGEMENT', 0.05),
                   ('_int', 'INTERNAL (arr+fre)', 0.05), ('bbcalls', 'DECIDER CALLS', None)]:
    print("\n################  metric = %s  ################" % lab)
    for num, den in [('A0','D1'), ('A0','C4'), ('A0','C5'), ('D1','C4'), ('D1','C5'), ('C4','C5')]:
        g, lo, hi, med, win, n = con(num, den, m, fl)
        print("  %s/%-3s geomean=%7.3f CI[%.3f,%.3f]  median=%7.3f  faster=%d/%d"
              % (num, den, g, lo, hi, med, win, n))
common = [p for p in pairs if all(val(p, a, 'wall_ms') for a in ARMS)]
print("\n-- absolute totals, all arms complete (n=%d) --" % len(common))
print("  %-4s %-26s %9s %9s %9s %9s %10s" % ('arm', '', 'wall', 'arr', 'arr+fre', 'residual', 'bbcalls'))
for a in ARMS:
    w = sum(val(p, a, 'wall_ms') for p in common) / 1000
    ar = sum(val(p, a, 'arr_ms') or 0 for p in common) / 1000
    it = sum(val(p, a, '_int') or 0 for p in common) / 1000
    bb = sum(int(d[(p, a)]['bbcalls']) for p in common)
    print("  %-4s %-26s %8.2fs %8.2fs %8.2fs %8.2fs %10d" % (a, LABEL[a], w, ar, it, w - it, bb))
