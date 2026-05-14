import subprocess
from test_utils import get_source_path

BENCHMARKS_BUILD_DIR = "benchmarks_build"

OPTIONS = [
    "--enable-sccp",
    "--enable-dce",
    "--enable-lsr"
]

def build_exec(compiler_path: str, workdir: str, test_name: str, flags=OPTIONS, build_benchmark=False, cycles=-1) -> str:
    source  = get_source_path(test_name, workdir)
    asm     = f"{workdir}/{BENCHMARKS_BUILD_DIR}/{test_name}.s"
    obj     = f"{workdir}/{BENCHMARKS_BUILD_DIR}/{test_name}.o"
    elf     = f"{workdir}/{BENCHMARKS_BUILD_DIR}/{test_name}"
    std_asm = f"{workdir}/std.s"
    std_obj = f"{workdir}/{BENCHMARKS_BUILD_DIR}/std.o"

    compile_command = [compiler_path, "--input", source, "--output", asm]
    if build_benchmark:
        compile_command.append("--benchmark")
    if cycles >= 0:
        compile_command.append("--cycles")
        compile_command.append(str(cycles))
    for flag in flags:
        compile_command.append( flag)

    print(compile_command)

    subprocess.run(["mkdir", "-p", f"{workdir}/{BENCHMARKS_BUILD_DIR}"], check=True)
    subprocess.run(compile_command,                                      check=True)
    subprocess.run(["nasm", "-f", "elf64", asm, "-o", obj],              check=True)
    subprocess.run(["nasm", "-f", "elf64", std_asm, "-o", std_obj],      check=True)
    subprocess.run(["ld", std_obj, obj, "-o", elf],                      check=True)

    return elf

