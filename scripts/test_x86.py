import pytest
import subprocess
from test_utils import CompilerTestBase, run_test_cases, get_source_path
from x86_build import build_exec

class X86CompilerTest(CompilerTestBase):
    def build_executable(self, test_name: str) -> str:
        return build_exec(self.compiler_path, self.workdir, test_name)

    def run_test(self, test_name: str) -> None:
        exec_path = self.build_executable(test_name)
        test_cases = self.parse_tests(test_name)
        run_test_cases([exec_path], test_cases)

@pytest.fixture
def x86_test(config):
    return X86CompilerTest(config["compiler"], config["workdir"])

def test_x86_compilation(test_name, x86_test):
    x86_test.run_test(test_name)
