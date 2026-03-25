import pytest
import subprocess
from test_utils import CompilerTestBase, run_test_cases

class X86CompilerTest(CompilerTestBase):
    BENCHMARKS_BUILD_DIR = "benchmarks_build"

    def build_executable(self, test_name: str) -> str:
        source = self.get_source_path(test_name)
        asm    = f"{self.workdir}/{self.BENCHMARKS_BUILD_DIR}/{test_name}.s"
        obj    = f"{self.workdir}/{self.BENCHMARKS_BUILD_DIR}/{test_name}.o"
        elf    = f"{self.workdir}/{self.BENCHMARKS_BUILD_DIR}/{test_name}"

        std_asm = f"{self.workdir}/std.s"
        std_obj = f"{self.workdir}/{self.BENCHMARKS_BUILD_DIR}/std.o"

        subprocess.run(["mkdir", "-p", f"{self.workdir}/{self.BENCHMARKS_BUILD_DIR}"], check=True)
        subprocess.run([self.compiler_path, source, asm],                              check=True)
        subprocess.run(["nasm", "-f", "elf64", asm, "-o", obj],                        check=True)
        subprocess.run(["nasm", "-f", "elf64", std_asm, "-o", std_obj],                check=True)
        subprocess.run(["ld", std_obj, obj, "-o", elf],                                check=True)

        return elf

    def run_test(self, test_name: str) -> None:
        exec_path = self.build_executable(test_name)
        test_cases = self.parse_tests(test_name)
        run_test_cases(exec_path, test_cases)

@pytest.fixture
def x86_test(config):
    return X86CompilerTest(config["compiler"], config["workdir"])

def test_x86_compilation(test_name, x86_test):
    x86_test.run_test(test_name)
