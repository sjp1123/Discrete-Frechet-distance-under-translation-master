import json
import subprocess
import time
import sys
import os

if len(sys.argv) < 2:
    print("usage: python3 run_to_json.py original_results.json")
    sys.exit(1)

input_json = sys.argv[1]
output_json = "candidate_results.json"

exe = "./build/calc_frechet_distance_under_translation"

if not os.path.exists(exe):
    print("executable not found:", exe)
    print("run ./build.sh first")
    sys.exit(1)

with open(input_json, "r") as f:
    cases = json.load(f)

results = []

for idx, case in enumerate(cases):
    name = case["name"]
    a_file = case["a_file"]
    b_file = case["b_file"]

    print(f"[{idx + 1}/{len(cases)}] running {name}")

    start = time.perf_counter()

    completed = subprocess.run(
        [exe, a_file, b_file, "n6"],
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
print("saved to", output_json)
