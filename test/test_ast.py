import pytest
import subprocess
from test_utils import CompilerTestBase, run_test_cases

class ASTCompilerTest(CompilerTestBase):
    BENCHMARKS_BUILD_DIR = "benchmarks_build"

    def run_test(self, test_name: str) -> None:
        source = self.get_source_path(test_name)
        test_cases = self.parse_tests(test_name)
        exec_path = self.compiler_path
        run_test_cases([exec_path, source], test_cases)

@pytest.fixture
def ast_test(config):
    return ASTCompilerTest(config["compiler"], config["workdir"])

def test_ast_interpretation(test_name, ast_test):
    ast_test.run_test(test_name)
