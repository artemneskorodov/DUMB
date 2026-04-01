import json
import sys
from x86_build import build_exec, BENCHMARKS_BUILD_DIR
import time
import subprocess

data = [
    {
        "name": "test1",
        "--enable-dce": "val11",
        "metric2": "val12"
    }, {
        "name": "test1",
        "metric1": "val11",
        "metric2": "val12"
    }, {
        "name": "test1",
        "metric1": "val11",
        "metric2": "val12"
    }
]

data = []

benchmarking_tests = [
        "FibbonacciRecursiveMulltiple"
    ]

if len(sys.argv) != 2:
    raise RuntimeError("Unexpected number of parameters")

compiler_path = sys.argv[1]
workdir       = sys.argv[2]

def run_single_test(test: str, sccp: bool, dce: bool):
    exec = build_exec(compiler_path, workdir, test, enable_sccp=sccp, enable_dce=dce)
    json_output = BENCHMARKS_BUILD_DIR + "/" + test + f".sccp_{sccp}.dce_{dce}result.json"
    subprocess.run(["hyperfine", "--export-json", json_output])
    with open(json_output, "r") as f:
        data = f.read()
        json_data = json.loads(data)

    print(json_data)
    data.append(
        {
            "name": test,
            "--(none)": 0,
            "--enable-sccp": 0,
            "--enable-sccp --enable-dce": 0
        }
    )

for test in benchmarking_tests:
    run_single_test(test, sccp=False, dce=False)
    run_single_test(test, sccp=True,  dce=False)
    run_single_test(test, sccp=True,  dce=True)

json.dump(data, sys.stdout)
