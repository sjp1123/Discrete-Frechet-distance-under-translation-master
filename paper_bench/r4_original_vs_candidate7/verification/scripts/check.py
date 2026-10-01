#!/usr/bin/env python3
"""Compare both arms against the exact oracle, per family.
LMF: error v - opt; pass if |v - opt| <= tol, tol = 1e-7 + 1e-15 * max|coordinate| * 4.
Decider: answer vs the truth class (YES must be 1, NO must be 0, EITHER is recorded only).
Writes res/summary.md and res/failures.txt.
"""
import csv, math, os, sys
from fractions import Fraction

D = os.environ.get("VD", "/root/verif/inst")
RES = os.environ.get("VR", "/root/verif/res")
FAMS = sys.argv[1:] or ["uniform", "grid", "cluster", "collinear", "copy", "singleton", "revisit", "offset_sig", "big", "tiny",
        "real_chars", "real_sig", "medium_uniform", "medium_real_chars", "medium_real_sig"]
ARMS = ["original", "candidate7"]

oracle = {}
for line in open(os.path.join(D, "oracle_out.txt")):
    p = line.split()
    oracle[p[0]] = (float(p[5]), Fraction(int(p[6]), int(p[7])), int(p[1]), int(p[2]))


from functools import lru_cache


@lru_cache(maxsize=None)
def maxcoord(fam, i):
    M = 0.0
    for s in ("a", "b"):
        for line in open(os.path.join(D, fam, f"{i}_{s}.txt")):
            x, y = map(float, line.split())
            M = max(M, abs(x), abs(y))
    return M


out, fails, tiefails = [], [], []
out.append("| family | inst | arm | LMF fail | LMF max abs err | LMF hang/crash | decider queries | wrong NO (δ ≥ δ* + resolution) | wrong YES (δ < δ*−band) | tie/below-resolution: NO/YES | either (YES/NO) | dec hang/crash |")
out.append("|---|--:|---|--:|--:|--:|--:|--:|--:|--:|--:|--:|")
tot = {a: dict(lf=0, lh=0, q=0, wn=0, wy=0, dh=0, inst=0) for a in ARMS}
for fam in FAMS:
    pairs = [l.split() for l in open(os.path.join(D, f"{fam}.pairs")) if l.strip()]
    truth = [l.split() for l in open(os.path.join(D, f"{fam}.truth")) if l.strip()]
    for arm in ARMS:
        lf = os.path.join(RES, f"{fam}_lmf_{arm}.csv")
        df = os.path.join(RES, f"{fam}_dec_{arm}.csv")
        if not (os.path.exists(lf) and os.path.exists(df)):
            continue
        L = list(csv.DictReader(open(lf)))
        Dd = list(csv.DictReader(open(df)))
        assert len(L) == len(pairs), (fam, arm, len(L), len(pairs))
        assert len(Dd) == len(truth), (fam, arm, len(Dd), len(truth))
        nfail = nhang = 0
        maxerr = 0.0
        for (a, b), r in zip(pairs, L):
            i = a.split("_")[0]
            key = f"{fam}/{i}"
            opt = oracle[key][0]
            if r["status"] != "OK":
                nhang += 1
                fails.append(f"{arm} LMF {r['status']} {key} opt={opt!r}")
                continue
            v = float(r["value"])
            err = v - opt
            tol = 1e-7 + 4e-15 * maxcoord(fam, i)
            maxerr = max(maxerr, abs(err))
            if abs(err) > tol:
                nfail += 1
                fails.append(f"{arm} LMF WRONG {key} value={v!r} opt={opt!r} err={err:.3e}")
        wn = wy = eyes = eno = dh = tie_ok = tie_no = 0
        for t, r in zip(truth, Dd):
            key, d, cls, kind = t
            if r["status"] != "OK":
                dh += 1
                fails.append(f"{arm} DEC {r['status']} {key} delta={d} truth={cls} kind={kind}")
                continue
            ans = r["answer"] == "1"
            dv, ov = float(d), oracle[key][0]
            up4 = ov
            for _ in range(4):
                up4 = math.nextafter(up4, math.inf)
            res = 16 * 2.0 ** -53 * (maxcoord(*key.split("/")) + dv)
            if cls == "YES" and (dv <= up4 or dv - ov < res):   # tie or gap below the coordinates' double resolution
                if ans: tie_ok += 1
                else:
                    tie_no += 1
                    tiefails.append(f"{arm} DEC tieNO {key} delta={d} opt={ov!r} kind={kind}")
                continue
            if cls == "YES" and not ans:
                wn += 1
                fails.append(f"{arm} DEC wrongNO {key} delta={d} opt={oracle[key][0]!r} kind={kind}")
            elif cls == "NO" and ans:
                wy += 1
                fails.append(f"{arm} DEC wrongYES {key} delta={d} opt={oracle[key][0]!r} kind={kind}")
            elif cls == "EITHER":
                if ans: eyes += 1
                else: eno += 1
        out.append(f"| {fam} | {len(pairs):,} | {arm} | {nfail} | {maxerr:.2e} | {nhang} | {len(truth):,} | {wn} | {wy} | {tie_no}/{tie_ok} | {eyes}/{eno} | {dh} |")
        T = tot[arm]
        T["tn"] = T.get("tn", 0) + tie_no; T["to"] = T.get("to", 0) + tie_ok
        T["lf"] += nfail; T["lh"] += nhang; T["q"] += len(truth); T["wn"] += wn; T["wy"] += wy; T["dh"] += dh; T["inst"] += len(pairs)
for arm in ARMS:
    T = tot[arm]
    out.append(f"| **total** | {T['inst']:,} | {arm} | {T['lf']} | | {T['lh']} | {T['q']:,} | {T['wn']} | {T['wy']} | {T.get('tn',0)}/{T.get('to',0)} | | {T['dh']} |")
open(os.path.join(RES, "summary.md"), "w").write("\n".join(out) + "\n")
open(os.path.join(RES, "failures.txt"), "w").write("\n".join(fails) + "\n")
open(os.path.join(RES, "tie_failures.txt"), "w").write("\n".join(tiefails) + "\n")
print("\n".join(out))
print(f"\n{len(fails)} failure lines -> res/failures.txt")
for f in fails[:40]:
    print(f)
