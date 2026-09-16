#!/usr/bin/env python3
"""Convert the ORIGINAL UCI file mixoutALL_shifted.mat into the tree's curve format,
exactly like the authors' test_data/benchmark/character_converter.m:

    A = cell2mat(mixout(1,i)); A(3,:) = []; A = A';    % x,y velocity, pen force dropped
    B = tril(ones(l,l),-1) * A;                         % exclusive prefix sum -> positions
    save(sprintf('data/%d.txt', i), 'B', '-ascii');     % 8 significant digits

Curve i is data/<i>.txt with i = 1..2858 in the file's own order, so the authors'
query files (test_data/fut_*_benchmark_queries/characters_*) apply unchanged.
labels.txt = consts.charlabels mapped through consts.key.

Usage: convert_characters_uci.py <mixoutALL_shifted.mat> [out_dir]
"""
import sys, os, hashlib, numpy as np, scipy.io as sio
src = sys.argv[1]
out = sys.argv[2] if len(sys.argv) > 2 else os.path.join(os.path.dirname(os.path.abspath(__file__)), "characters_uci")
os.makedirs(os.path.join(out, "data"), exist_ok=True)
m = sio.loadmat(src); mix = m["mixout"]; consts = m["consts"][0, 0]
key = [k[0] for k in consts["key"].flat]; lab = consts["charlabels"].flatten()
raw, dd = [], []
with open(os.path.join(out, "dataset.txt"), "w") as ds, open(os.path.join(out, "labels.txt"), "w") as lf:
    for i in range(mix.shape[1]):
        A = mix[0, i][:2, :].T.astype(np.float64)
        B = np.vstack([np.zeros((1, 2)), np.cumsum(A, axis=0)[:-1]])   # == tril(ones,-1)*A
        with open(os.path.join(out, "data", f"{i+1}.txt"), "w") as f:
            for x, y in B: f.write(" %.8e %.8e\n" % (x, y))
        ds.write(f"{i+1}.txt\n"); lf.write(f"{i+1}.txt {key[lab[i]-1]}\n")
        raw.append(len(B)); k = 1
        for j in range(1, len(B)):
            if not (B[j, 0] == B[j-1, 0] and B[j, 1] == B[j-1, 1]): k += 1
        dd.append(k)
with open(os.path.join(out, "SOURCE_SHA256.txt"), "w") as f:
    f.write(hashlib.sha256(open(src, "rb").read()).hexdigest() + "  mixoutALL_shifted.mat\n")
print("curves", len(raw), "rows mean %.2f ; vertices after duplicate removal mean %.2f (min %d max %d)" % (np.mean(raw), np.mean(dd), min(dd), max(dd)))
