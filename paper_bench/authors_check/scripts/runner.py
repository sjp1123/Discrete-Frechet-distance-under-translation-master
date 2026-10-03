#!/usr/bin/env python3
"""Run paper_bench (one arm, one mode) over a pair/query list, surviving hangs and crashes.
The harness flushes one CSV row per instance; a watchdog kills the process when no new row appears
for <timeout> seconds (per-instance timeout), the next instance is recorded as HANG (or CRASH on a
non-zero exit) and the run resumes after it.
    runner.py <arm> <lmf|decider> <list> <curve_dir> <out.csv> [per-instance timeout s] [cpu]
Binaries: GITLAB (~/b_pb_authors_gitlab/paper_bench), ORIG (~/b_pb_original/...), C7 (~/b_pb_candidate7/...),
AUTHORS_N6 (~/b_pb_authors_n6/...).  For the r4 verification families use run_verif.sh, which takes the folder
where verification/raw/oracle_instances.tar.gz was extracted.
"""
import csv, os, subprocess, sys, tempfile, time

arm, mode, lst, cdir, out = sys.argv[1:6]
tmo = int(sys.argv[6]) if len(sys.argv) > 6 else 600
tmo = min(tmo, int(os.environ.get("TMO_CAP", "150" if mode == "lmf" else "45")))
cpu = sys.argv[7] if len(sys.argv) > 7 else "1"
H = os.path.expanduser("~")
BIN = {"original": os.environ.get("ORIG", f"{H}/b_pb_original/paper_bench"),
       "candidate7": os.environ.get("C7", f"{H}/b_pb_candidate7/paper_bench"),
       "authors_n6": os.environ.get("AUTHORS_N6", f"{H}/b_pb_authors_n6/paper_bench"),
       "gitlab": os.environ.get("GITLAB", f"{H}/b_pb_authors_gitlab/paper_bench")}[arm]
ENV = dict(os.environ, MAXREGION="cech", MAXREGION_EXACT="1", MAXREGION_SLACK="0")
lines = [l for l in open(lst) if l.strip()]
rows, events = [], []
start = 0
header = None


def nrows(path):
    try:
        with open(path) as f:
            return max(0, sum(1 for _ in f) - 1)
    except FileNotFoundError:
        return 0


while start < len(lines):
    with tempfile.NamedTemporaryFile("w", delete=False, suffix=".txt") as f:
        f.writelines(lines[start:])
        part = f.name
    tmp = out + ".part.csv"
    if os.path.exists(tmp):
        os.unlink(tmp)
    p = subprocess.Popen(["taskset", "-c", cpu, BIN, mode, part, cdir, tmp], env=ENV,
                         stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
    last_n, last_t, status = -1, time.time(), None
    while True:
        try:
            p.wait(timeout=2)
            status = "OK" if p.returncode == 0 else "CRASH"
            break
        except subprocess.TimeoutExpired:
            n = nrows(tmp)
            if n != last_n:
                last_n, last_t = n, time.time()
            elif time.time() - last_t > tmo:
                p.kill(); p.wait()
                status = "HANG"
                break
    err = p.stderr.read().decode(errors="replace")[-300:]
    got = list(csv.reader(open(tmp))) if os.path.exists(tmp) else []
    body = []
    if got:
        header = got[0]
        body = [r for r in got[1:] if header and len(r) == len(header)]
    for r in body:
        rows.append(r + ["OK"])
    os.unlink(part)
    start += len(body)
    if status == "OK" and start >= len(lines):
        break
    if status == "OK":
        events.append(f"SHORT {start}")
        break
    events.append(f"{status} line={start} rc={p.returncode} input={lines[start].strip()} stderr={err.strip()!r}")
    rows.append([""] * (len(header) if header else 1) + [status])
    start += 1
with open(out, "w", newline="") as f:
    w = csv.writer(f)
    w.writerow((header or ["?"]) + ["status"])
    w.writerows(rows)
open(out + ".events", "w").write("\n".join(events) + ("\n" if events else ""))
if os.path.exists(out + ".part.csv"):
    os.unlink(out + ".part.csv")
print(f"{arm} {mode} {os.path.basename(lst)}: {len(rows)} rows, events {len(events)}")
