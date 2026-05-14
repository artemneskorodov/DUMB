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
        TestCase("LsrBenchmark",               100000000),
    ]

if len(sys.argv) != 3:
    raise RuntimeError("Unexpected number of parameters")

compiler_path = sys.argv[1]
workdir       = sys.argv[2]

def run_single_test(test: str, sccp: bool, dce: bool, lsr: bool, cycles: int) -> int:
    exec_name = build_exec(compiler_path, workdir, test, enable_sccp=sccp, enable_dce=dce, enable_lsr=lsr, build_benchmark=True, cycles=cycles)
    json_output = BENCHMARKS_BUILD_DIR + "/" + test + f".sccp_{sccp}.dce_{dce}.lsr_{lsr}.result.json"
    subprocess.run(["hyperfine", "--warmup", "5", "--export-json", json_output, f"'{exec_name}'"])
    with open(json_output, "r") as f:
        data = f.read()
        json_data = json.loads(data)

    return json_data['results'][0]['mean']

for test in benchmarking_tests:
    result_fff = run_single_test(test.name, sccp=False, dce=False, lsr=False, cycles=test.cycles)
    result_tff = run_single_test(test.name, sccp=True,  dce=False, lsr=False, cycles=test.cycles)
    result_ttf = run_single_test(test.name, sccp=True,  dce=True , lsr=False, cycles=test.cycles)
    result_ftf = run_single_test(test.name, sccp=False, dce=True , lsr=False, cycles=test.cycles)
    result_fft = run_single_test(test.name, sccp=False, dce=False, lsr=True , cycles=test.cycles)
    result_tft = run_single_test(test.name, sccp=True,  dce=False, lsr=True , cycles=test.cycles)
    result_ttt = run_single_test(test.name, sccp=True,  dce=True , lsr=True , cycles=test.cycles)
    result_ftt = run_single_test(test.name, sccp=False, dce=True , lsr=True , cycles=test.cycles)

    json_result.append(
        {
            "name":         test.name,
            "no_opt":       f"{result_fff / result_fff * 100 : .3f} %",
            "sccp":         f"{result_tff / result_fff * 100 : .3f} %",
            "sccp_dce":     f"{result_ttf / result_fff * 100 : .3f} %",
            "dce":          f"{result_ftf / result_fff * 100 : .3f} %",
            "lsr":          f"{result_fft / result_fff * 100 : .3f} %",
            "sccp_lsr":     f"{result_tft / result_fff * 100 : .3f} %",
            "sccp_dce_lsr": f"{result_ttt / result_fff * 100 : .3f} %",
            "dce_lsr":      f"{result_ftt / result_fff * 100 : .3f} %"
        }
    )

OUTPUT_FILE = f"{workdir}/result.json"

with open(OUTPUT_FILE, "w") as f:
    json.dump(json_result, f)
