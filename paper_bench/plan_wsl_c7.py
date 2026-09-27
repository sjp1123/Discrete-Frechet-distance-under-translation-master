#!/usr/bin/env python3
"""Plan the candidate7 timing run (run_wsl_c7.sh): jobs, costs, and a balanced split over streams.

Jobs (each runs the arms original, candidate7, candidate5 back-to-back on one pinned vCPU):
  lmfc <id>                 Characters LMF, a chunk of 100 pairs of characters_uci_lmf_pairs.txt
  lmfs <id> <i> <f1> <f2> <skip_original>
                            Sigspatial LMF, one pair of sigspatial_pairs.txt (one process per arm);
                            the original is skipped on the 2 pairs where it needs > 12 GB (r3)
  dec  <id> <query file>    decider, one paperq4 file (the paper's (1 +- 4^l) protocol)
Costs are estimated from the archived runs (Characters LMF r2, Sigspatial LMF r3, decider r3) as
original + 2 * candidate5 and the jobs are assigned longest first to the least-loaded stream.

    python paper_bench/plan_wsl_c7.py <out_dir> [streams=8]
"""
import csv, io, os, sys, tarfile

HERE = os.path.dirname(os.path.abspath(__file__))
Q = os.path.join(HERE, "queries")
R = os.path.join(HERE, "results")
SETS = ["characters_uci_all", "characters_uci_same", "sigspatial"]
LEVELS = [(l, "plus") for l in range(-10, 3)] + [(l, "minus") for l in range(-10, 0)]
CPUS = [1, 3, 5, 7, 9, 11, 13, 15]
OOM_ORIGINAL = {125, 432}          # 1-based lines of sigspatial_pairs.txt (r3: > 12 GB)
CHUNK = 100


def member(tar, name):
    with tarfile.open(os.path.join(R, tar)) as tf:
        return list(csv.DictReader(io.TextIOWrapper(tf.extractfile(name), encoding="utf-8")))


def main():
    out = sys.argv[1]
    streams = int(sys.argv[2]) if len(sys.argv) > 2 else 8
    os.makedirs(os.path.join(out, "chunks"), exist_ok=True)
    jobs = []   # (cost_ms, line)

    # Characters LMF: 210 chunks of 100 pairs, costs from the r2 container run (matched by index)
    pairs = [l.split() for l in open(os.path.join(Q, "characters_uci_lmf_pairs.txt")) if l.strip()]
    o = member("raw_characters_uci_lmf_r2.tar.gz", "characters_uci_lmf_original_r2.csv")
    c = member("raw_characters_uci_lmf_r2.tar.gz", "characters_uci_lmf_candidate5_exact_r2.csv")
    assert len(pairs) == len(o) == len(c) == 21000
    for k in range(0, len(pairs), CHUNK):
        cid = "c%03d" % (k // CHUNK)
        with open(os.path.join(out, "chunks", cid + ".txt"), "w", newline="\n") as f:
            for a, b in pairs[k:k + CHUNK]:
                f.write(f"{a} {b}\n")
        cost = sum(float(o[i]["time_ms"]) + 2 * float(c[i]["time_ms"]) for i in range(k, min(k + CHUNK, len(pairs))))
        jobs.append((cost, f"lmfc {cid}"))

    # Sigspatial LMF: one job per pair, costs from r3 (by pair)
    sp = [l.split() for l in open(os.path.join(Q, "sigspatial_pairs.txt")) if l.strip()]
    so = {(r["file1"], r["file2"]): float(r["time_ms"]) for r in member("raw_sigspatial_lmf_r3.tar.gz", "sigspatial_lmf_original_r3.csv")}
    sc = {(r["file1"], r["file2"]): float(r["time_ms"]) for r in member("raw_sigspatial_lmf_r3.tar.gz", "sigspatial_lmf_candidate5_exact_r3.csv")}
    assert len(sp) == 1000
    for i, (a, b) in enumerate(sp, 1):
        skip = 1 if i in OOM_ORIGINAL else 0
        cost = (0 if skip else so.get((a, b), 0)) + 2 * sc.get((a, b), 0)
        jobs.append((cost, f"lmfs s{i:04d} {i} {a} {b} {skip}"))

    # decider, paperq4: costs from r3 per file
    for s in SETS:
        tar = "raw_sigspatial_decider_r3.tar.gz" if s == "sigspatial" else "raw_characters_uci_decider_r3.tar.gz"
        for l, sign in LEVELS:
            name = f"{s}_paperq4_{l}_{sign}"
            assert os.path.exists(os.path.join(Q, name + ".txt")), name
            t = lambda arm: sum(float(r["time_ms"]) for r in member(tar, f"{name}_{arm}_r3.csv"))
            jobs.append((t("original") + 2 * t("candidate5"), f"dec d_{name} {name}"))

    jobs.sort(key=lambda j: -j[0])
    load = [0.0] * streams
    lists = [[] for _ in range(streams)]
    for cost, line in jobs:
        k = min(range(streams), key=lambda s: load[s])
        load[k] += cost
        lists[k].append(line)
    for k in range(streams):
        with open(os.path.join(out, f"s{k}.txt"), "w", newline="\n") as f:
            f.write("\n".join(lists[k]) + "\n")
    with open(os.path.join(out, "cpus.txt"), "w", newline="\n") as f:
        f.write(" ".join(str(c) for c in CPUS[:streams]) + "\n")
    print(f"{len(jobs)} jobs; estimated load per stream (s): " + ", ".join(f"{x/1000:.0f}" for x in load))
    print(f"estimated total (s): {sum(load)/1000:.0f}; largest job (s): {jobs[0][0]/1000:.0f} ({jobs[0][1][:40]})")


if __name__ == "__main__":
    main()
