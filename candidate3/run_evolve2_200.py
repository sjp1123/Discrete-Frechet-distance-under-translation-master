import json
import subprocess
import time
import sys
import os

input_json = "/home/sj10132/frechet_evolve/results/cases_pair_501_1500.json"
output_json = "/home/sj10132/FRECHET_EVOLVE2/candidate/candidate_results.json"
exe = "/home/sj10132/FRECHET_EVOLVE2/candidate/build/calc_frechet_distance_under_translation"

if not os.path.exists(exe):
    print("executable not found:", exe)
    sys.exit(1)

old_prefix = "/home/sj10132/frechet_evolve/original/test_data/benchmark/Geolife Trajectories 1.3/data/"
new_prefix = "/home/sj10132/FRECHET_EVOLVE2/test_data/benchmark/Geolife Trajectories 1.3/data/"

with open(input_json) as f:
    all_cases = json.load(f)

cases = all_cases[:200]
results = []

for idx, case in enumerate(cases):
    name = case["name"]
    a_file_actual = case["a_file"]
    b_file_actual = case["b_file"]

    print(f"[{idx + 1}/200] running {name}")

    start = time.perf_counter()
    completed = subprocess.run(
        [exe, a_file_actual, b_file_actual, "n6"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    end = time.perf_counter()
    candidate_elapsed = end - start

    original_elapsed = case.get("original_elapsed")
    speedup = None
    reduction_percent = None
    if original_elapsed is not None and candidate_elapsed > 0:
        speedup = original_elapsed / candidate_elapsed
        reduction_percent = (original_elapsed - candidate_elapsed) / original_elapsed * 100.0

    result = dict(case)
    result["a_file"] = a_file_actual.replace(old_prefix, new_prefix)
    result["b_file"] = b_file_actual.replace(old_prefix, new_prefix)
    result["candidate_elapsed"] = candidate_elapsed
    result["speedup"] = speedup
    result["reduction_percent"] = reduction_percent
    result["returncode"] = completed.returncode
    result["candidate_stdout"] = completed.stdout
    result["candidate_stderr"] = completed.stderr

    results.append(result)

    with open(output_json, "w") as f:
        json.dump(results, f, indent=2)

print("done")
print(f"saved {len(results)} results to {output_json}")
