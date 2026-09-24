#!/usr/bin/env python3
"""Collect the r3 re-measurement of run_wsl_paired.sh (all sources in its work directory W) into
paper_bench/results/raw_*_r3.tar.gz.  Safe to re-run: the sources in W are never deleted, an archive
is replaced only atomically and never by one with fewer CSV members or with a CSV member that has fewer
rows (the script stops instead), and nothing is written when W holds no rows for an archive.

  LMF      W/lmf/<arm>/<index>.csv (one file per pair and arm; rows ordered as queries/sigspatial_pairs.txt)
           + W/lmf/rss.txt (exit code and peak RSS per pair and arm)          -> raw_sigspatial_lmf_r3.tar.gz
  decider  W/decider/<prefix>_<tag>_<l>_<sign>_<arm>_r3.csv                   -> raw_sigspatial_decider_r3.tar.gz,
                                                                                 raw_characters_uci_decider_r3.tar.gz
  (The r3 decider rows of 2026-09-25 were written to results/ by an earlier runner and are archived already.)
  Members of an existing archive that the new build does not produce (e.g. sigspatial_lmf_original_oom_12gb.txt)
  are carried over unchanged.

    python3 paper_bench/merge_wsl_r3.py <W>     (e.g. //wsl.localhost/Ubuntu-22.04/home/<user>/wsl_r3)
"""
import csv
import glob
import io
import os
import sys
import tarfile

HERE = os.path.dirname(os.path.abspath(__file__)); R = os.path.join(HERE, "results")
W = sys.argv[1]
NL = "\n"


def lmf_members():
    pairs = [tuple(l.split()) for l in open(os.path.join(HERE, "queries", "sigspatial_pairs.txt")) if l.strip()]
    assert len(pairs) == 1000
    out = {}
    for arm in ("original", "candidate5_exact", "candidate5_noslack"):
        hdr = None; rows = []; missing = []
        for i, pair in enumerate(pairs, 1):
            fn = os.path.join(W, "lmf", arm, f"{i}.csv")
            r = None
            if os.path.exists(fn):
                with open(fn, encoding="utf-8") as f:
                    rd = csv.reader(f); h = next(rd, None); r = next(rd, None)
                if h and hdr is None: hdr = h
                if not (h == hdr and r and len(r) == len(hdr) and (r[0], r[1]) == pair): r = None
            if r is None: missing.append(i)
            else: rows.append(r)
        if not rows: continue
        buf = io.StringIO(); w = csv.writer(buf, lineterminator=NL); w.writerow(hdr); w.writerows(rows)
        out[f"sigspatial_lmf_{arm}_r3.csv"] = buf.getvalue().encode()
        shown = missing if len(missing) <= 10 else missing[:5] + ["..."] + missing[-2:]
        print(f"{arm}: {len(rows)} rows, {len(missing)} pairs missing {shown}, non-positive times {sum(float(r[5]) <= 0 for r in rows)}")
    rss = os.path.join(W, "lmf", "rss.txt")
    if out and os.path.exists(rss):
        out["sigspatial_lmf_rss_r3.txt"] = open(rss, "rb").read()
    return out


def decider_members(glob_pat):
    return {os.path.basename(f): open(f, "rb").read() for f in sorted(glob.glob(os.path.join(W, "decider", glob_pat)))}


def write_archive(name, members):
    dst = os.path.join(R, name)
    if not members:
        print(f"{name}: no rows in {W}; existing archive left untouched")
        return
    carried = {}; mtimes = {}
    if os.path.exists(dst):
        with tarfile.open(dst) as old:
            names = old.getnames()
            carried = {n: old.extractfile(n).read() for n in names if n not in members}
            mtimes = {m.name: m.mtime for m in old.getmembers()}
            lost = [n for n in names if n not in members and n.endswith(".csv")]
            # a member may be replaced only by one with at least as many rows (a partial W must not shrink the archive)
            shrunk = [(n, old.extractfile(n).read().count(b"\n"), members[n].count(b"\n")) for n in names
                      if n in members and n.endswith(".csv") and members[n].count(b"\n") < old.extractfile(n).read().count(b"\n")]
        if lost:
            sys.exit(f"{name}: the existing archive has {len(lost)} CSV members that W does not reproduce (e.g. {lost[0]}); refusing to overwrite")
        if shrunk:
            sys.exit(f"{name}: {len(shrunk)} CSV members would lose rows (e.g. {shrunk[0][0]}: {shrunk[0][1]} -> {shrunk[0][2]} lines); refusing to overwrite")
    tmp = dst + ".tmp"
    with tarfile.open(tmp, "w:gz") as tf:
        for n, data in list(members.items()) + list(carried.items()):
            ti = tarfile.TarInfo(n); ti.size = len(data); ti.mode = 0o644; ti.mtime = mtimes.get(n, 0)
            tf.addfile(ti, io.BytesIO(data))
    os.replace(tmp, dst)
    print(f"{name}: {len(members)} members from W, {len(carried)} carried over")


write_archive("raw_sigspatial_lmf_r3.tar.gz", lmf_members())
write_archive("raw_sigspatial_decider_r3.tar.gz", decider_members("sigspatial_paperq*_r3.csv"))
write_archive("raw_characters_uci_decider_r3.tar.gz", decider_members("characters_uci_*_paperq*_r3.csv"))
