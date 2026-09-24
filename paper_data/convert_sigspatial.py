#!/usr/bin/env python3
"""Sigspatial (ACM GIS Cup 2017, the authors' `shortest-sf.tgz`, 20,199 curves): strip the
`x y k tid` header of every files/file-NNNNNN.dat into paper_data/sigspatial/data/, exactly
what the authors' fetch_and_convert_data.py does (`tail -n +2`); the parser ignores the
third and fourth columns.  Writes dataset.txt (sorted file names) and SOURCE_SHA256.txt.

    python3 paper_data/convert_sigspatial.py [path/to/shortest-sf.tgz]
"""
import hashlib, os, sys, tarfile
here = os.path.dirname(os.path.abspath(__file__))
src = sys.argv[1] if len(sys.argv) > 1 else os.path.join(here, "sigspatial", "shortest-sf.tgz")
dst = os.path.join(here, "sigspatial", "data"); os.makedirs(dst, exist_ok=True)
names = []
with tarfile.open(src, "r:gz") as tf:
    for m in tf:
        if not m.isfile() or not m.name.endswith(".dat"): continue
        name = os.path.basename(m.name)
        lines = tf.extractfile(m).read().decode().split(chr(10))
        assert lines[0].strip() == "x y k tid", (name, lines[0])
        with open(os.path.join(dst, name), "w", newline=chr(10)) as f: f.write(chr(10).join(lines[1:]))
        names.append(name)
names.sort()
with open(os.path.join(here, "sigspatial", "dataset.txt"), "w", newline=chr(10)) as f: f.write(chr(10).join(names) + chr(10))
h = hashlib.sha256(open(src, "rb").read()).hexdigest()
with open(os.path.join(here, "sigspatial", "SOURCE_SHA256.txt"), "w", newline=chr(10)) as f: f.write(h + "  shortest-sf.tgz" + chr(10))
print("curves", len(names), "sha256", h)
