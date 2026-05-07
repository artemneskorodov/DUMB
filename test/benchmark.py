import json
import sys
from x86_build import build_exec, BENCHMARKS_BUILD_DIR
import subprocess
from dataclasses import dataclass
import json
import matplotlib.pyplot as plt
from matplotlib.colors import LinearSegmentedColormap
import pandas as pd
import numpy as np

@dataclass
class TestCase:
    name: str
    cycles: int

BENCHMARKING_TESTS = [
        TestCase("FibonacciRecursiveMultiple", 10),
        TestCase("ConstantFakeFlowControl",    100000000),
        TestCase("EmptyCalculations",          100000000),
        TestCase("DeadCalculations",           100000000),
    ]

if len(sys.argv) != 3:
    raise RuntimeError("Unexpected number of parameters")
COMPILER_PATH = sys.argv[1]
WORKDIR       = sys.argv[2]

def run_single_test(test: str, sccp: bool, dce: bool, cycles: int) -> int:
    exec_name = build_exec(COMPILER_PATH, WORKDIR, test, enable_sccp=sccp, enable_dce=dce, build_benchmark=True, cycles=cycles)
    json_output = BENCHMARKS_BUILD_DIR + "/" + test + f".sccp_{sccp}.dce_{dce}.result.json"
    subprocess.run(["hyperfine", "--warmup", "5", "--export-json", json_output, f"'{exec_name}'"])
    with open(json_output, "r") as f:
        data = f.read()
        json_data = json.loads(data)

    return json_data['results'][0]['mean']

def create_advanced_table(result_rows, output_file="performance_table.png"):
    df = pd.DataFrame(result_rows)

    colors = ['#00ff00', '#ffff00', '#ff0000']
    cmap = LinearSegmentedColormap.from_list('custom', colors, N=256)

    display_data = []
    for _, row in df.iterrows():
        display_data.append([
            row['name'],
            f"{row['no_opt'  ] : .1f}%",
            f"{row['sccp'    ] : .1f}%",
            f"{row['sccp_dce'] : .1f}%",
            f"{row['dce'     ] : .1f}%"
        ])

    header_to_name_map = {
        "no optimizations":           "no_opt",
        "--enable-sccp":              "sccp",
        "--enable-sccp\n--enable-dce": "sccp_dce",
        "--enable-dce":               "dce",
    }

    headers = ['Test Name', 'no optimizations', '--enable-sccp', '--enable-dce', '--enable-sccp\n--enable-dce']

    fig_width = len(df.columns)
    fig_height = len(df)

    fig, ax = plt.subplots(figsize=(fig_width, fig_height))
    ax.axis('tight')
    ax.axis('off')
    ax.set_xlim(0, 1)
    ax.set_ylim(0, 1)

    table = ax.table(cellText=display_data, colLabels=headers,
                     cellLoc='center', loc='center',
                     colWidths=[0.25, 0.15, 0.15, 0.15, 0.15],
                     bbox=[0, 0, 1, 1])

    table.auto_set_font_size(False)
    table.set_fontsize(10)
    table.scale(1.0, 1.0)

    for i in range(len(df)):
        for j, col in enumerate(headers[1:], 1):
            value = df.iloc[i][header_to_name_map[col]]

            norm_value = min(max(value / 100.0, 0), 1)

            color = cmap(norm_value)

            cell = table[(i + 1, j)]
            cell.set_facecolor(color)

            cell.set_text_props(color='black', weight='bold')

    for j, header in enumerate(headers):
        cell = table[(0, j)]
        cell.set_facecolor('#40466e')
        cell.set_text_props(color='white', weight='bold', fontsize=5)

    plt.tight_layout()
    plt.savefig(output_file, dpi=200, bbox_inches='tight')
    plt.close()

    print(f"Table is saved to {output_file}")

# Использование
if __name__ == "__main__":
    result_rows = []

    for test in BENCHMARKING_TESTS:
        result_false_false = run_single_test(test.name, sccp=False, dce=False, cycles=test.cycles)
        result_true_false  = run_single_test(test.name, sccp=True,  dce=False, cycles=test.cycles)
        result_true_true   = run_single_test(test.name, sccp=True,  dce=True , cycles=test.cycles)
        result_false_true  = run_single_test(test.name, sccp=False, dce=True , cycles=test.cycles)

        result_rows.append(
            {
                "name":     test.name,
                "no_opt":   result_false_false / result_false_false * 100,
                "sccp":     result_true_false  / result_false_false * 100,
                "sccp_dce": result_true_true   / result_false_false * 100,
                "dce":      result_false_true  / result_false_false * 100,
            }
        )

    create_advanced_table(result_rows)
