# test_utils.py
import os
import pty
import termios
import subprocess
from typing import List, Tuple, Dict, Any

class TestCase:
    def __init__(self, steps: List[Tuple[str, str]]):
        self.steps = steps

    def add_input(self, value: str):
        self.steps.append(("in", value))

    def add_output(self, value: str):
        self.steps.append(("out", value))

def create_pty_process(run_exec_command: List[str]):
    master_fd, slave_fd = pty.openpty()

    attrs = termios.tcgetattr(slave_fd)
    attrs[3] = attrs[3] & ~termios.ECHO
    termios.tcsetattr(slave_fd, termios.TCSANOW, attrs)

    proc = subprocess.Popen(
        run_exec_command,
        stdin=slave_fd,
        stdout=slave_fd,
        stderr=slave_fd,
        text=True,
        close_fds=True
    )

    return proc, master_fd

def read_from_pty(master_fd, expected_length: int) -> str:
    output = ""
    while len(output) < expected_length:
        data = os.read(master_fd, 1).decode('ascii')
        output += data.replace("\r", "").replace("\n", "")
    return output

def run_test_cases(run_exec_command: List[str], test_cases: List[TestCase]) -> None:
    for case in test_cases:
        proc, master_fd = create_pty_process(run_exec_command)
        all_output = ""
        try:
            for operation, value in case.steps:
                if operation == "in":
                    os.write(master_fd, (value + "\n").encode())
                elif operation == "out":
                    output = ""
                    while len(output) < len(value):
                        data = os.read(master_fd, 1).decode('ascii')
                        output += data.replace("\r", "").replace("\n", "")
                        all_output += data
                    assert output == value, f'Expected: "{value}", got: "{output}".\nOutput:\n{all_output}'
        finally:
            proc.kill()

def get_test_path(test_name: str, workdir: str) -> str:
    return f"{workdir}/benchmarks/{test_name}.test"

def get_source_path(test_name: str, workdir: str) -> str:
    return f"{workdir}/benchmarks/{test_name}.dumb"

class CompilerTestBase:
    def __init__(self, compiler_path: str, workdir: str):
        self.compiler_path = compiler_path
        self.workdir = workdir

    def parse_tests(self, test_name: str) -> List[TestCase]:
        path = self.get_test_path(test_name)

        cases = []
        current_case = None

        with open(path, "r", encoding="utf-8") as f:
            for line in f:
                line = line.strip()
                if not line:
                    continue

                if line.startswith("case:"):
                    if current_case is not None:
                        cases.append(current_case)
                    current_case = TestCase([])
                elif line.startswith("in:"):
                    if current_case is None:
                        current_case = TestCase([])
                    value = line[len("in:"):].strip()
                    current_case.add_input(eval(value))
                elif line.startswith("out:"):
                    if current_case is None:
                        raise RuntimeError(f"Output without case in: {line}")
                    value = line[len("out:"):].strip()
                    current_case.add_output(eval(value))
                else:
                    raise RuntimeError(f"Invalid line in test file: {line}")

            if current_case is not None:
                cases.append(current_case)

        return cases
