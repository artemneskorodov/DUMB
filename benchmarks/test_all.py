import sys
import subprocess

def test_addition(proc : subprocess.Popen) -> int:
    proc.stdin.write( "123")
    proc.stdin.write( "123")

    output = proc.stdin.read()
    if output != "246":
        return 1
    return 0

def get_proc(exec : str) -> subprocess.Popen:
    return subprocess.Popen(
        [exec],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        bufsize=1
    )

def build_exec(test_file_name_base, compiler_exec) -> str:
    source_file_name = "benchmarks/"       + test_file_name_base + ".dumb"
    asm_file_name    = "benchmarks_build/" + test_file_name_base + ".s"
    object_file_name = "benchmarks_build/" + test_file_name_base + ".o"
    elf_file_name    = "benchmarks_build/" + test_file_name_base + ""

    result = subprocess.run([compiler_exec, source_file_name, asm_file_name])
    if result != 0:
        raise RuntimeError("Building error")

    result = subprocess.run(["nasm", "-f", "elf64", asm_file_name, "-o", object_file_name])
    if result != 0:
        raise RuntimeError("Assembly failed")

    result = subprocess.run(["nasm", "-f", "elf64", "std.s", "-o", "std.o"])
    if result != 0:
        raise RuntimeError("Library assembly failed")

    result = subprocess.run(["ld", "std.o", object_file_name, "-o", elf_file_name])
    if result != 0:
        raise RuntimeError("Linking failed")
    return elf_file_name

def main() -> int:
    compiler_exec : str = sys.argv[1]
    test_name     : str = sys.argv[2]

    test_map = {
        "addition": test_addition,
    }

    test_exec = build_exec(test_name, compiler_exec)
    test_proc = get_proc(test_exec)

    return test_map[test_name](test_proc)

if __name__ == "__main__":
    sys.exit(main())
