import pytest
from colorama import Fore, Style

def pytest_addoption(parser):
    if not hasattr(parser, '_added_compiler_option'):
        parser.addoption("--compiler", action="store", required=True,
                         help="Path to compiler executable")
        parser.addoption("--workdir", action="store", required=True,
                         help="Working directory with benchmarks")
        parser.addoption("--tests", action="store", required=True,
                         help="Comma separated test names")
        parser.addoption("--test-type", action="store", default="x86",
                         choices=["x86", "ast", "ir"],
                         help="Type of tests to run")
        parser._added_compiler_option = True

def pytest_generate_tests(metafunc):
    if "test_name" in metafunc.fixturenames:
        tests = metafunc.config.getoption("--tests").split(",")
        metafunc.parametrize("test_name", tests)

@pytest.fixture
def config(request):
    return {
        "compiler":  request.config.getoption("--compiler"),
        "workdir":   request.config.getoption("--workdir"),
        "tests":     request.config.getoption("--tests").split(","),
        "test_type": request.config.getoption("--test-type"),
    }

def pytest_runtest_logreport(report):
    if report.when == "call":
        name = report.location[2]

        if report.passed:
            print(f"{name:50} [" + Fore.GREEN + "PASS" + Fore.RESET + "]")
        elif report.failed:
            print(f"{name:50} [" + Fore.RED   + "FAIL" + Fore.RESET + "]")
