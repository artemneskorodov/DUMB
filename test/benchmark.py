import json
import sys
from x86_build import build_exec, BENCHMARKS_BUILD_DIR
import time
import subprocess

json_result = []

benchmarking_tests = [
        "FibonacciRecursiveMultiple"
    ]

if len(sys.argv) != 3:
    raise RuntimeError("Unexpected number of parameters")

compiler_path = sys.argv[1]
workdir       = sys.argv[2]

def run_single_test(test: str, sccp: bool, dce: bool) -> int:
    exec_name = build_exec(compiler_path, workdir, test, enable_sccp=sccp, enable_dce=dce)
    json_output = BENCHMARKS_BUILD_DIR + "/" + test + f".sccp_{sccp}.dce_{dce}.result.json"
    subprocess.run(["hyperfine", "--warmup", "5", "--export-json", json_output, f"'{exec_name}'"])
    with open(json_output, "r") as f:
        data = f.read()
        json_data = json.loads(data)

    return json_data['results'][0]['mean']

for test in benchmarking_tests:
    result_false_false = run_single_test(test, sccp=False, dce=False)
    result_true_false  = run_single_test(test, sccp=True,  dce=False)
    result_true_true   = run_single_test(test, sccp=True,  dce=True)

    json_result.append(
        {
            "name":     test,
            "no_opt":   f"{result_false_false}",
            "sccp":     f"{result_true_false}",
            "sccp_dce": f"{result_true_true}"
        }
    )

OUTPUT_FILE = f"{workdir}/result.json"

with open(OUTPUT_FILE, "w") as f:
    json.dump(json_result, f)
