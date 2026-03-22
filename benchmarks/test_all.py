import sys
import subprocess
import pty
import os
import termios

def check_out(fd, string):
    output = os.read(fd, len(string)).decode()
    print(output)
    if output != string:
        raise RuntimeError(f"Expected: \"{string}\", but got: \"{output}\"")

def write(fd, string):
    os.write(fd, (string + "\n").encode())

def test_addition(proc : subprocess.Popen, fd : int):
    check_out(fd, "First = ")
    write(fd, "10")
    check_out(fd, "Second = ")
    write(fd, "10")
    check_out(fd, "First + Second = ")
    check_out(fd, "20")

def get_proc(exec : str):
    master_fd, slave_fd = pty.openpty()

    attrs = termios.tcgetattr(slave_fd)
    attrs[3] = attrs[3] & ~termios.ECHO
    termios.tcsetattr(slave_fd, termios.TCSANOW, attrs)

    proc = subprocess.Popen(
        [exec],
        stdin=slave_fd,
        stdout=slave_fd,
        stderr=slave_fd,
        text=True,
        close_fds=True
    )
    return proc, master_fd

def build_exec(test_file_name_base, compiler_exec, work_dir) -> str:
    source_file_name    = work_dir + "/benchmarks/"       + test_file_name_base + ".dumb"
    asm_file_name       = work_dir + "/benchmarks_build/" + test_file_name_base + ".s"
    object_file_name    = work_dir + "/benchmarks_build/" + test_file_name_base + ".o"
    elf_file_name       = work_dir + "/benchmarks_build/" + test_file_name_base + ""
    asm_std_file_name   = work_dir + "/std.s"
    object_std_file_name = work_dir + "/benchmarks_build/std.o"

    result = subprocess.run([compiler_exec, source_file_name, asm_file_name])
    if result.returncode != 0:
        raise RuntimeError("Building error")

    result = subprocess.run(["nasm", "-f", "elf64", asm_file_name, "-o", object_file_name])
    if result.returncode != 0:
        raise RuntimeError("Assembly failed")

    result = subprocess.run(["nasm", "-f", "elf64", asm_std_file_name, "-o", object_std_file_name])
    if result.returncode != 0:
        raise RuntimeError("Library assembly failed")

    result = subprocess.run(["ld", object_std_file_name, object_file_name, "-o", elf_file_name])
    if result.returncode != 0:
        raise RuntimeError("Linking failed")
    return elf_file_name

def print_results_table(results):
    print("+------------+---------+")
    print("| Test       | Result  |")
    print("+------------+---------+")
    for test_name, passed in results.items():
        status = "PASS" if passed else "FAIL"
        print(f"| {test_name:<10} | {status:<7} |")
    print("+------------+---------+")

def main() -> int:
    compiler_exec : str = sys.argv[1]
    work_dir      : str = sys.argv[2]
    test_name     : str = sys.argv[3]

    test_map = {
        "addition": test_addition,
    }

    test_exec = build_exec(test_name, compiler_exec, work_dir)
    print("Running test " + test_name)
    test_proc, master_fd = get_proc(test_exec)

    return 0

if __name__ == "__main__":
    sys.exit(main())
