import subprocess
from test_utils import get_source_path

BENCHMARKS_BUILD_DIR = "benchmarks_build"

def build_exec(compiler_path: str, workdir: str, test_name: str, enable_sccp=True, enable_dce=True, enable_lsr=True, build_benchmark=False, cycles=-1) -> str:
    source  = get_source_path(test_name, workdir)
    asm     = f"{workdir}/{BENCHMARKS_BUILD_DIR}/{test_name}.s"
    obj     = f"{workdir}/{BENCHMARKS_BUILD_DIR}/{test_name}.o"
    elf     = f"{workdir}/{BENCHMARKS_BUILD_DIR}/{test_name}"
    std_asm = f"{workdir}/std.s"
    std_obj = f"{workdir}/{BENCHMARKS_BUILD_DIR}/std.o"

    compile_command = [compiler_path, "--input", source, "--output", asm]
    if enable_sccp:
        compile_command.append("--enable-sccp")
    if enable_dce:
        compile_command.append("--enable-dce")
    if build_benchmark:
        compile_command.append("--benchmark")
    if enable_lsr:
        compile_command.append("--enable-lsr")
    if cycles >= 0:
        compile_command.append("--cycles")
        compile_command.append(str(cycles))

    print(compile_command)

    subprocess.run(["mkdir", "-p", f"{workdir}/{BENCHMARKS_BUILD_DIR}"], check=True)
    subprocess.run(compile_command,                                      check=True)
    subprocess.run(["nasm", "-f", "elf64", asm, "-o", obj],              check=True)
    subprocess.run(["nasm", "-f", "elf64", std_asm, "-o", std_obj],      check=True)
    subprocess.run(["ld", std_obj, obj, "-o", elf],                      check=True)

    return elf

