from colorama import Fore, Back, Style

def pytest_addoption(parser):
    parser.addoption("--compiler", action="store", required=True)
    parser.addoption("--workdir" , action="store", required=True)
    parser.addoption("--tests"   , action="store", required=True,
                     help="Comma separated test names")

def pytest_generate_tests(metafunc):
    if "test_name" in metafunc.fixturenames:
        tests = metafunc.config.getoption("--tests").split(",")
        metafunc.parametrize("test_name", tests)

def pytest_runtest_logreport(report):
    if report.when == "call":
        name = report.location[2]

        if report.passed:
            print(f"{name:40}[" + Fore.GREEN + "PASS" + Fore.RESET + "]")
        elif report.failed:
            print(f"{name:40}[" + Fore.RED   + "FAIL" + Fore.RESET + "]")
