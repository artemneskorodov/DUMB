import json
import sys
from x86_build import build_exec, BENCHMARKS_BUILD_DIR, OPTIONS
import time
import subprocess
from dataclasses import dataclass
from itertools import product
from typing import Dict, List

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

OUTPUT_FILE = f"{workdir}/result.json"

@dataclass
class TestResult:
    name: str
    flags: Dict[str, bool]
    time: float

#
# Generate configuration name
#
def config_name(flags: Dict[str, bool]) -> str:
    enabled = [k for k, v in flags.items() if v]

    if not enabled:
        return "no-flags"

    return " ".join(enabled)

#
# Build and run single test
#
def run_single_test(test: str, cycles: int, flags) -> int:
    exec_name = build_exec(compiler_path, workdir, test, build_benchmark=True, cycles=cycles, flags=flags)
    json_output = BENCHMARKS_BUILD_DIR + "/" + test + f".result.json"
    subprocess.run(["hyperfine", "--warmup", "5", "--export-json", json_output, f"'{exec_name}'"])
    with open(json_output, "r") as f:
        data = f.read()
        json_data = json.loads(data)

    return TestResult(
        name=test,
        flags=flags,
        time=json_data['results'][0]['mean'],
    )

#
# Markdown table generator
#
def generate_markdown_table(results: List[TestResult]) -> str:
    configs = []

    for r in results:
        name = config_name(r.flags)

        if name not in configs:
            configs.append(name)

    lines = []

    header = ["Test", *configs]

    lines.append("| " + " | ".join(header) + " |")
    lines.append("| " + " | ".join(["---"] * len(header)) + " |")

    grouped = {}

    for r in results:
        grouped.setdefault(r.test_name, {})
        grouped[r.test_name][config_name(r.flags)] = r.execution_time

    for test_name, values in grouped.items():
        row = [test_name]

        for cfg in configs:
            value = values.get(cfg)

            if value is None:
                row.append("-")
            else:
                row.append(f"{value:.2f}s")

        lines.append("| " + " | ".join(row) + " |")

    return "\n".join(lines)

#
# Running tests
#
all_results: List[TestResult] = []

for test in benchmarking_tests:
    for values in product([False, True], repeat=len(OPTIONS)):
        enabled = [
            option
            for option, is_enabled in zip(OPTIONS, values)
            if is_enabled
        ]
        result = run_single_test(test.name, cycles=test.cycles, flags=enabled)
        all_results.append(result)

with open(OUTPUT_FILE, "w") as f:
    f.write(generate_markdown_table(all_results))

