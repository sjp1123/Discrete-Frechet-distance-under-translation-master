#!/usr/bin/env python3
"""Seeded pair lists for the paper-data benchmark (replaces the time-seeded RNG of the
authors' fut_create_benchmark_{decider,val_computation}.cpp with a fixed seed so the
sets are reproducible).

  characters_all   : 2000 random pairs of distinct curves over all 2858 characters
                     (the paper's "all-characters" pair model)
  characters_same  : 1000 random pairs of distinct curves with the same letter, drawn
                     from the 1429 curves whose letter is known (paper_data/characters/
                     labels_train.txt) -- the paper's "same-characters" pair model
  sigspatial_subset: 1000 random pairs of distinct curves over the 101 available
                     GIS Cup 2017 sample files (see paper_data/sigspatial_subset/README.md)
"""
import random, os, collections
here = os.path.dirname(os.path.abspath(__file__))
root = os.path.dirname(here)
out = os.path.join(here, "queries"); os.makedirs(out, exist_ok=True)

def pairs(names, n, rng):
    seen=set(); res=[]
    while len(res) < n:
        a, b = rng.choice(names), rng.choice(names)
        if a == b or (a,b) in seen: continue
        seen.add((a,b)); res.append((a,b))
    return res

def write(fn, ps):
    with open(os.path.join(out, fn), "w") as f:
        for a,b in ps: f.write(f"{a} {b}\n")
    print(fn, len(ps))

chars = [l.strip() for l in open(os.path.join(root,"paper_data/characters/dataset.txt")) if l.strip()]
write("characters_all_pairs.txt", pairs(chars, 2000, random.Random(20200825)))

by_letter = collections.defaultdict(list)
for l in open(os.path.join(root,"paper_data/characters/labels_train.txt")):
    f, c = l.split(); by_letter[c].append(f)
rng = random.Random(20200826); letters = sorted(by_letter); same=[]; seen=set()
while len(same) < 1000:
    c = rng.choice(letters); a, b = rng.choice(by_letter[c]), rng.choice(by_letter[c])
    if a == b or (a,b) in seen: continue
    seen.add((a,b)); same.append((a,b))
write("characters_same_pairs.txt", same)

sig = [l.strip() for l in open(os.path.join(root,"paper_data/sigspatial_subset/dataset.txt")) if l.strip()]
write("sigspatial_subset_pairs.txt", pairs(sig, 1000, random.Random(20200827)))
