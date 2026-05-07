import json
import sys
from x86_build import build_exec, BENCHMARKS_BUILD_DIR
import time
import subprocess
from dataclasses import dataclass

json_result = []

@dataclass
class TestCase:
    name: str
    cycles: int

benchmarking_tests = [
        TestCase("FibonacciRecursiveMultiple", 10),
        TestCase("ConstantFakeFlowControl",    100000000),
        TestCase("EmptyCalculations",          100000000),
        TestCase("DeadCalculations",           100000000),
    ]

if len(sys.argv) != 3:
    raise RuntimeError("Unexpected number of parameters")

compiler_path = sys.argv[1]
workdir       = sys.argv[2]

def run_single_test(test: str, sccp: bool, dce: bool, cycles: int) -> int:
    exec_name = build_exec(compiler_path, workdir, test, enable_sccp=sccp, enable_dce=dce, build_benchmark=True, cycles=cycles)
    json_output = BENCHMARKS_BUILD_DIR + "/" + test + f".sccp_{sccp}.dce_{dce}.result.json"
    subprocess.run(["hyperfine", "--warmup", "5", "--export-json", json_output, f"'{exec_name}'"])
    with open(json_output, "r") as f:
        data = f.read()
        json_data = json.loads(data)

    return json_data['results'][0]['mean']

for test in benchmarking_tests:
    result_false_false = run_single_test(test.name, sccp=False, dce=False, cycles=test.cycles)
    result_true_false  = run_single_test(test.name, sccp=True,  dce=False, cycles=test.cycles)
    result_true_true   = run_single_test(test.name, sccp=True,  dce=True , cycles=test.cycles)
    result_false_true  = run_single_test(test.name, sccp=False, dce=True , cycles=test.cycles)

    json_result.append(
        {
            "name":     test.name,
            "no_opt":   f"{result_false_false / result_false_false * 100 : .3f} %",
            "sccp":     f"{result_true_false  / result_false_false * 100 : .3f} %",
            "sccp_dce": f"{result_true_true   / result_false_false * 100 : .3f} %",
            "dce":      f"{result_false_true  / result_false_false * 100 : .3f} %"
        }
    )

OUTPUT_FILE = f"{workdir}/result.json"

with open(OUTPUT_FILE, "w") as f:
    json.dump(json_result, f)
