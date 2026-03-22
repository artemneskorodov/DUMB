import os
import pty
import termios
import subprocess
import pytest

BENCHMARKS_DIR       = "benchmarks"
BENCHMARKS_BUILD_DIR = "benchmarks_build"

def parse_test_file(path: str):
    steps = []

    with open(path, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue

            if line.startswith("in:"):
                value = line[len("in:") : ]
                steps.append(("in", eval(value)))
            elif line.startswith("out:"):
                value = line[len("out:") : ]
                steps.append(("out", eval(value)))
            else:
                raise RuntimeError(f"Invalid line in test file: {line}")

    return steps

def get_proc(exec_path: str):
    master_fd, slave_fd = pty.openpty()

    attrs = termios.tcgetattr(slave_fd)
    attrs[3] = attrs[3] & ~termios.ECHO
    termios.tcsetattr(slave_fd, termios.TCSANOW, attrs)

    proc = subprocess.Popen(
        [exec_path],
        stdin=slave_fd,
        stdout=slave_fd,
        stderr=slave_fd,
        text=True,
        close_fds=True
    )

    return proc, master_fd

def build_exec(test_name, compiler_exec, work_dir):
    subprocess.run(["mkdir", "-p", work_dir + "/benchmarks_build"])
    source = f"{work_dir}/benchmarks/{test_name}.dumb"
    asm    = f"{work_dir}/benchmarks_build/{test_name}.s"
    obj    = f"{work_dir}/benchmarks_build/{test_name}.o"
    elf    = f"{work_dir}/benchmarks_build/{test_name}"

    std_asm = f"{work_dir}/std.s"
    std_obj = f"{work_dir}/benchmarks_build/std.o"

    if subprocess.run([compiler_exec, source, asm]).returncode != 0:
        pytest.fail("Compilation failed")

    if subprocess.run(["nasm", "-f", "elf64", asm, "-o", obj]).returncode != 0:
        pytest.fail("Assembly failed")

    if subprocess.run(["nasm", "-f", "elf64", std_asm, "-o", std_obj]).returncode != 0:
        pytest.fail("Std assembly failed")

    if subprocess.run(["ld", std_obj, obj, "-o", elf]).returncode != 0:
        pytest.fail("Linking failed")

    return elf

def run_script(master_fd, steps):
    for operation, value in steps:
        if operation == "in":
            os.write(master_fd, (value + "\n").encode())
        elif operation == "out":
            output = ""
            while not output.endswith(value):
                output += os.read(master_fd, 1).decode()
            output = output.replace("\n", "")
            output = output.replace("\r", "")
            assert output == value, f'Expected: "{value}", got: "{output}"'

@pytest.fixture
def config(request):
    return {
        "compiler": request.config.getoption("--compiler"),
        "workdir": request.config.getoption("--workdir"),
        "tests": request.config.getoption("--tests").split(","),
    }

def tests_on_x86(test_name, config):
    compiler = config["compiler"]
    workdir  = config["workdir"]

    exec_path = build_exec(test_name, compiler, workdir)

    test_file = f"{workdir}/benchmarks/{test_name}.test"
    steps = parse_test_file(test_file)

    proc, master_fd = get_proc(exec_path)

    try:
        run_script(master_fd, steps)
    finally:
        proc.kill()
