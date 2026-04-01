import subprocess
from test_utils import get_source_path

BENCHMARKS_BUILD_DIR = "benchmarks_build"

def build_exec(compiler_path: str, workdir: str, test_name: str) -> str:
    source  = get_source_path(workdir, test_name)
    asm     = f"{workdir}/{BENCHMARKS_BUILD_DIR}/{test_name}.s"
    obj     = f"{workdir}/{BENCHMARKS_BUILD_DIR}/{test_name}.o"
    elf     = f"{workdir}/{BENCHMARKS_BUILD_DIR}/{test_name}"
    std_asm = f"{workdir}/std.s"
    std_obj = f"{workdir}/{BENCHMARKS_BUILD_DIR}/std.o"

    subprocess.run(["mkdir", "-p", f"{workdir}/{BENCHMARKS_BUILD_DIR}"],                                 check=True)
    subprocess.run([compiler_path, "--input", source, "--output", asm, "--enable-sccp", "--enable-dce"], check=True)
    subprocess.run(["nasm", "-f", "elf64", asm, "-o", obj],                                              check=True)
    subprocess.run(["nasm", "-f", "elf64", std_asm, "-o", std_obj],                                      check=True)
    subprocess.run(["ld", std_obj, obj, "-o", elf],                                                      check=True)

    return elf

