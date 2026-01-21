import os

import pytest


REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
SKIP_ENV = "SKIP_REPO_HYGIENE"


#============================================
@pytest.fixture
def repo_root() -> str:
	"""
	Provide the repository root path.
	"""
	return REPO_ROOT


#============================================
def pytest_addoption(parser) -> None:
	"""
	Add repo hygiene options.
	"""
	group = parser.getgroup("repo-hygiene")
	group.addoption(
		"--no-ascii-fix",
		action="store_true",
		help="Disable auto-fix for ASCII compliance tests.",
	)


#============================================
@pytest.fixture
def skip_repo_hygiene() -> bool:
	"""
	Check whether repo hygiene tests should be skipped.
	"""
	return os.environ.get(SKIP_ENV) == "1"


#============================================
@pytest.fixture
def ascii_fix_enabled(request) -> bool:
	"""
	Check whether ASCII compliance auto-fix is enabled.
	"""
	return not request.config.getoption("--no-ascii-fix")
