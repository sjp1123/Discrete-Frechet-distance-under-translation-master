#!/usr/bin/env python3
"""r6 significance check: is candidate8 faster than candidate7 (and than the authors' code)?
   sig_r6.py <W of run_r6.sh or run_r6b.sh>      (the authors' rows only when W holds a gitlab arm)
Per benchmark: totals, per-instance time ratios (geomean, bootstrap 95 % CI, median), sign test and Wilcoxon
signed-rank test on log ratios (normal approximation), split into
  same work  : both arms made the same black-box calls and spent no time in the base case (pure timing noise)
  base case  : at least one arm reached a base case (where candidate8 changes anything).
Also the r5 'real slow' pairs: instances where candidate7 did different work and was slower than the authors."""
import csv, glob, math, os, random, statistics as st, sys

W = sys.argv[1]


def load(d, arms):
    rows = {}
    for arm in arms:
        for p in sorted(glob.glob(f'{d}/{arm}/*.csv')):
            u = os.path.basename(p)[:-4]
            with open(p) as fh:
                for i, r in enumerate(csv.DictReader(fh)):
                    rows.setdefault((u, i), {})[arm] = r
    return [v for k, v in sorted(rows.items()) if all(a in v for a in arms)]


f = lambda r, k: float(r[k])
gm = lambda xs: math.exp(sum(map(math.log, xs)) / len(xs))


def boot(xs, n=2000, seed=1):
    rnd = random.Random(seed); lg = [math.log(x) for x in xs]; k = len(lg); out = []
    for _ in range(n):
        s = rnd.choices(lg, k=k); out.append(math.exp(sum(s) / k))
    out.sort(); return out[int(0.025 * n)], out[int(0.975 * n) - 1]


def phi(z):  # standard normal upper tail, two-sided p
    return math.erfc(abs(z) / math.sqrt(2))


def sign_test(xs):
    pos = sum(1 for x in xs if x > 1); neg = sum(1 for x in xs if x < 1); n = pos + neg
    if n == 0: return pos, neg, 1.0
    z = (pos - n / 2) / math.sqrt(n / 4)
    return pos, neg, phi(z)


def wilcoxon(xs):
    d = [math.log(x) for x in xs if x != 1]
    n = len(d)
    if n < 10: return float('nan')
    order = sorted(range(n), key=lambda i: abs(d[i]))
    ranks = [0.0] * n; i = 0
    while i < n:
        j = i
        while j + 1 < n and abs(d[order[j + 1]]) == abs(d[order[i]]): j += 1
        r = (i + j) / 2 + 1
        for k in range(i, j + 1): ranks[order[k]] = r
        i = j + 1
    wp = sum(r for r, x in zip(ranks, d) if x > 0)
    mu = n * (n + 1) / 4; sd = math.sqrt(n * (n + 1) * (2 * n + 1) / 24)
    return phi((wp - mu) / sd)


def same_work(a, b):
    return a['bbcalls'] == b['bbcalls'] and f(a, 'n6_arr_ms') == 0 == f(b, 'n6_arr_ms') and f(a, 'n6_fre_ms') == 0 == f(b, 'n6_fre_ms')


def report(rows, num, den, label):
    rs = [f(r[num], 'time_ms') / f(r[den], 'time_ms') for r in rows]
    if not rs: print(f'| {label} | 0 | | | | | | |'); return
    lo, hi = boot(rs)
    pos, neg, ps = sign_test(rs)
    pw = wilcoxon(rs)
    tt = sum(f(r[num], 'time_ms') for r in rows) / sum(f(r[den], 'time_ms') for r in rows)
    print(f'| {label} | {len(rs):,} | {tt:.3f}× | {gm(rs):.3f}× [{lo:.3f}, {hi:.3f}] | {st.median(rs):.3f}× | '
          f'{pos:,} / {neg:,} | {ps:.1e} | {pw:.1e} |')


def unit_test(d, label):
    """Per job (Characters: a chunk of 100 pairs; Sigspatial: a pair; decider: a file of 1,000 queries):
    instances of one job share the moment they ran, so the job is the independent unit."""
    rs = []
    for p in sorted(glob.glob(f'{d}/candidate7/*.csv')):
        q = p.replace('/candidate7/', '/candidate8/')
        if not os.path.exists(q) or (label and not os.path.basename(p).startswith(label)): continue
        a = sum(float(r['time_ms']) for r in csv.DictReader(open(p)))
        b = sum(float(r['time_ms']) for r in csv.DictReader(open(q)))
        rs.append(a / b)
    pos = sum(1 for x in rs if x > 1); neg = sum(1 for x in rs if x < 1); n = pos + neg
    p = min(1.0, 2 * sum(math.comb(n, i) for i in range(min(pos, neg) + 1)) / 2 ** n) if n else 1.0
    return len(rs), gm(rs), st.median(rs), pos, neg, p


for title, sub in (('Characters LMF', 'chars'), ('Sigspatial LMF', 'sig')):
    have = [a for a in ('gitlab', 'candidate7', 'candidate8') if os.path.isdir(f'{W}/{sub}/{a}')]
    rows = load(f'{W}/{sub}', have)
    if not rows: continue
    print(f'## {title} ({len(rows):,} instances)\n')
    for a in have:
        t = sum(f(r[a], 'time_ms') for r in rows); c = sum(f(r[a], 'bbcalls') for r in rows)
        bc = sum(f(r[a], 'arr2_ms') for r in rows)
        print(f'- {a}: time {t:,.0f} ms ({t/len(rows):.2f}/inst.), calls {c:,.0f} ({c/len(rows):,.1f}/inst.), base case {bc:,.0f} ms')
    print()
    print('| subset (ratio = first ÷ second arm) | n | total ratio | per-instance geomean [95 % CI] | median | first slower / faster | sign test p | Wilcoxon p |')
    print('|---|--:|--:|--:|--:|--:|--:|--:|')
    sw = [r for r in rows if same_work(r['candidate7'], r['candidate8'])]
    bc = [r for r in rows if not same_work(r['candidate7'], r['candidate8'])]
    report(rows, 'candidate7', 'candidate8', 'c7 ÷ c8, all')
    report(sw, 'candidate7', 'candidate8', 'c7 ÷ c8, same work (noise)')
    report(bc, 'candidate7', 'candidate8', 'c7 ÷ c8, base case reached')
    if 'gitlab' in have:
        report(rows, 'gitlab', 'candidate7', 'authors ÷ c7, all')
        report(rows, 'gitlab', 'candidate8', 'authors ÷ c8, all')
        real7 = [r for r in rows if f(r['candidate7'], 'time_ms') > f(r['gitlab'], 'time_ms') and not same_work(r['candidate7'], r['gitlab'])]
        report(real7, 'gitlab', 'candidate7', "authors ÷ c7, c7's real slow pairs")
        report(real7, 'gitlab', 'candidate8', "authors ÷ c8, c7's real slow pairs")
    print()
    n, g, md, pos, neg, p = unit_test(f'{W}/{sub}', '')
    print(f'Per job ({"chunk of 100 pairs" if sub == "chars" else "pair"}): {n} jobs, c7 ÷ c8 geomean {g:.3f}×, median {md:.3f}×, '
          f'c8 faster in {pos}, slower in {neg}, exact sign test p = {p:.1e}.\n')
    if 'gitlab' in have:
        s7 = sum(1 for r in rows if f(r['candidate7'], 'time_ms') > f(r['gitlab'], 'time_ms'))
        s8 = sum(1 for r in rows if f(r['candidate8'], 'time_ms') > f(r['gitlab'], 'time_ms'))
        r8 = sum(1 for r in rows if f(r['candidate8'], 'time_ms') > f(r['gitlab'], 'time_ms') and not same_work(r['candidate8'], r['gitlab']))
        print(f'Slower than the authors\' code: c7 {s7} (different work {len(real7)}), c8 {s8} (different work {r8}).')
    dc = [f(r['candidate8'], 'bbcalls') - f(r['candidate7'], 'bbcalls') for r in rows]
    print(f'Black-box calls c8 − c7: fewer on {sum(1 for x in dc if x < 0)}, more on {sum(1 for x in dc if x > 0)}, '
          f'total {sum(f(r["candidate7"], "bbcalls") for r in rows):,.0f} → {sum(f(r["candidate8"], "bbcalls") for r in rows):,.0f}.')
    for x, y in (('gitlab', 'candidate7'), ('gitlab', 'candidate8'), ('candidate7', 'candidate8')):
        if x not in have: continue
        d = [f(r[y], 'value') - f(r[x], 'value') for r in rows]
        print(f'Value {y} − {x}: min {min(d):.3g}, max {max(d):.3g}, |d| > 1e-7: {sum(1 for v in d if abs(v) > 1e-7)}')
    print()

dec = f'{W}/dec4'
if os.path.isdir(dec):
    print('## Decider 4^l (candidate7 ÷ candidate8)\n')
    print('| benchmark | n | total ratio | per-query geomean [95 % CI] | median | c7 slower / faster | sign test p | Wilcoxon p |')
    print('|---|--:|--:|--:|--:|--:|--:|--:|')
    for b in ('characters_uci_same', 'characters_uci_all', 'sigspatial'):
        rows = []
        for p in sorted(glob.glob(f'{dec}/candidate7/{b}_paperq4_*.csv')):
            q = p.replace('/candidate7/', '/candidate8/')
            if not os.path.exists(q): continue
            a = list(csv.DictReader(open(p))); c = list(csv.DictReader(open(q)))
            exp = 1 if p.endswith('_plus.csv') else 0
            for x, y in zip(a, c):
                assert int(x['answer']) == exp and int(y['answer']) == exp, (p, x, y)
                rows.append({'candidate7': x, 'candidate8': y})
        if rows: report(rows, 'candidate7', 'candidate8', b)
    print('\n(all answers checked against the expected YES/NO of each file)\n')
    print('Per file (1,000 queries run in one process; the independent unit):\n')
    print('| benchmark | files | c7 ÷ c8 geomean | median | c8 faster / slower | exact sign test p | calls c7 | calls c8 |')
    print('|---|--:|--:|--:|--:|--:|--:|--:|')
    for b in ('characters_uci_same', 'characters_uci_all', 'sigspatial'):
        n, g, md, pos, neg, p = unit_test(dec, b)
        c7 = sum(float(r['bbcalls']) for q in glob.glob(f'{dec}/candidate7/{b}_*.csv') for r in csv.DictReader(open(q)))
        c8 = sum(float(r['bbcalls']) for q in glob.glob(f'{dec}/candidate8/{b}_*.csv') for r in csv.DictReader(open(q)))
        print(f'| {b} | {n} | {g:.3f}× | {md:.3f}× | {pos} / {neg} | {p:.2f} | {c7:,.0f} | {c8:,.0f} |')
