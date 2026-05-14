import json
import sys
from x86_build import build_exec, BENCHMARKS_BUILD_DIR
from test_utils import get_bench_path
import time
import subprocess
from dataclasses import dataclass
from typing import List
from collections import defaultdict

@dataclass
class TestCase:
    name: str
    cycles: int

BENCHMARKING_TESTS = [
        TestCase("FibonacciRecursiveMultiple", 10),
        TestCase("ConstantFakeFlowControl",    100000000),
        TestCase("EmptyCalculations",          100000000),
        TestCase("DeadCalculations",           100000000),
        TestCase("LsrBenchmark",               1000),
    ]

OPTIMIZATIONS = [
    "sccp",
    "dce",
    "lsr",
    None
]

if len(sys.argv) < 3:
    raise RuntimeError("Unexpected number of parameters")

if len(sys.argv) == 3:
    benchmarking_tests = BENCHMARKING_TESTS
else:
    benchmarking_tests = []
    for elem in sys.argv[3:]:
        for test_case in BENCHMARKING_TESTS:
            if test_case.name == elem:
                benchmarking_tests.append(test_case)
                break

compiler_path = sys.argv[1]
workdir       = sys.argv[2]

OUTPUT_FILE = f"{workdir}/result.md"

@dataclass
class TestResult:
    name: str
    optimization: str
    time: float

#
# Generate configuration name
#
def config_name(flags: List[str]) -> str:
    if not flags:
        return "no-flags"

    return " ".join(flags)
#
# Build and run single test
#
def run_single_test(test: str, cycles: int, optimization: str) -> int:
    exec_name = build_exec(compiler_path, workdir, test, build_benchmark=True, cycles=cycles, pipeline=optimization, test_name_function=get_bench_path)
    json_output = BENCHMARKS_BUILD_DIR + "/" + test + f".result.json"
    subprocess.run(["hyperfine", "--warmup", "5", "--export-json", json_output, f"'{exec_name}'"])
    with open(json_output, "r") as f:
        data = f.read()
        json_data = json.loads(data)

    optname = optimization
    if optname == None:
        optname = "default-pipeline"

    return TestResult(
        name=test,
        flags=optimization,
        time=json_data['results'][0]['mean'],
    )

#
# Markdown table generator
#
def generate_markdown_table(results: List[TestResult]) -> str:
    tests = sorted({r.name for r in results})
    pipelines = sorted({r.optimization for r in results})

    results_map = defaultdict(dict)
    for r in results:
        results_map[r.name][r.optimization] = r.time

    table = []

    header = ["Test"] + pipelines
    table.append("| " + " | ".join(header) + " |")
    table.append("|" + "|".join(["---"] * len(header)) + "|")

    for test in tests:
        row = [test]
        for pipeline in pipelines:
            time = results_map[test].get(pipeline, "")
            row.append(f"{time:.3f}" if time != "" else "")
        table.append("| " + " | ".join(row) + " |")

    return "\n".join(table)

#
# Running tests
#
all_results: List[TestResult] = []

for test in benchmarking_tests:
    for optimization in OPTIMIZATIONS:
        result = run_single_test(test.name, cycles=test.cycles, optimization=optimization)
        all_results.append(result)

with open(OUTPUT_FILE, "w") as f:
    f.write(generate_markdown_table(all_results))
