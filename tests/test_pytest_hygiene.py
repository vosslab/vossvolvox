# Standard Library
import ast

# PIP3 modules
import pytest

# local repo modules
import file_utils

# Banned module-level Name assignments that indicate local discovery scaffold.
# Each entry is a Name node id that must not appear as a module-level assignment.
BANNED_MODULE_ASSIGNMENTS = frozenset({
	"SKIP_DIRS",
})

# Banned function names that indicate local discovery scaffold that was
# centralized into file_utils. These names must not appear as top-level
# FunctionDef nodes in hygiene test files.
BANNED_FUNCTION_NAMES = frozenset({
	"path_has_skip_dir",
	"gather_files",
	"gather_changed_files",
})

REPORT_NAME = file_utils.report_name(__file__)

HEADER = "pytest hygiene violations"

# Module-level dict of repo-relative POSIX key -> list of violation lines.
# Populated by the autouse collect_report fixture before any test runs.
VIOLATIONS_BY_FILE: dict[str, list[str]] = {}

# Discover only the top-level tests/test_*.py files.
# Keep only files whose repo-relative POSIX path matches tests/test_*.py
# (top-level tests/ only, not tests/meta/ or deeper subtrees).

#============================================
def _keep_top_level_test(rel: str) -> bool:
	"""
	Keep only top-level tests/test_*.py files.

	Excludes test files in sub-directories like tests/meta/.

	Args:
		rel: Repo-relative POSIX path.

	Returns:
		bool: True when the path is exactly tests/test_<stem>.py.
	"""
	parts = rel.split("/")
	# Must be exactly two parts: "tests" / "test_*.py".
	if len(parts) != 2:
		return False
	if parts[0] != "tests":
		return False
	return parts[1].startswith("test_") and parts[1].endswith(".py")


FILES = file_utils.discover_files(
	extensions=(".py",),
	extra_filter=_keep_top_level_test,
	test_key="pytest_hygiene",
)


#============================================
def check_no_banned_module_assignments(tree: ast.Module, rel: str) -> list[str]:
	"""
	Fail when a module-level Name assignment duplicates file_utils scaffold.

	Checks for module-level Assign statements whose target is a plain Name
	matching one of the banned names (e.g. SKIP_DIRS). These were centralized
	into file_utils and must not be re-introduced locally.

	Args:
		tree: Parsed AST module.
		rel: Repo-relative POSIX path for error messages.

	Returns:
		list[str]: Violation messages (empty when clean).
	"""
	violations = []
	for node in tree.body:
		if not isinstance(node, ast.Assign):
			continue
		for target in node.targets:
			if not isinstance(target, ast.Name):
				continue
			if target.id in BANNED_MODULE_ASSIGNMENTS:
				violations.append(
					f"{rel}:{node.lineno}: module-level `{target.id}` found; "
					"file_utils is the single owner of this scaffold -- "
					"remove it and use file_utils.discover_files instead."
				)
	return violations


#============================================
def check_no_banned_functions(tree: ast.Module, rel: str) -> list[str]:
	"""
	Fail when a top-level FunctionDef duplicates file_utils scaffold.

	Checks the module body for FunctionDef nodes whose name matches a
	banned scaffold name. These functions were centralized into file_utils
	and must not be re-introduced in hygiene tests.

	Args:
		tree: Parsed AST module.
		rel: Repo-relative POSIX path for error messages.

	Returns:
		list[str]: Violation messages (empty when clean).
	"""
	violations = []
	for node in tree.body:
		if not isinstance(node, ast.FunctionDef):
			continue
		if node.name in BANNED_FUNCTION_NAMES:
			violations.append(
				f"{rel}:{node.lineno}: function `{node.name}` found; "
				"file_utils is the single owner of this scaffold -- "
				"remove it and use file_utils.discover_files instead."
			)
	return violations


#============================================
def check_file(rel: str, tree: ast.Module) -> list[str]:
	"""
	Run all hygiene checks on one parsed module and combine the violations.

	Thin module-specific combiner over check_no_banned_module_assignments and
	check_no_banned_functions. Receives a real ast.Module: the shared
	file_utils.collect_python_violations owns parsing and SyntaxError capture,
	so this is only reached for files that parsed cleanly.

	Args:
		rel: Repo-relative POSIX path for error messages.
		tree: Parsed ast.Module for the file.

	Returns:
		list[str]: Combined violation lines (empty when the file is clean).
	"""
	# Accumulate violations from each hygiene rule check.
	violations = check_no_banned_module_assignments(tree, rel)
	violations += check_no_banned_functions(tree, rel)
	return violations


#============================================
@pytest.fixture(scope="module", autouse=True)
def collect_report() -> None:
	"""
	Autouse fixture: populate VIOLATIONS_BY_FILE and write report when dirty.

	Clears and rebuilds the module-level violations dict via the shared harness,
	then writes the report only when there are violations. The guarded
	clear_stale_reports owns removal of clean-run reports.
	"""
	# Once-per-process guarded cleanup of repo-root report_*.txt (no-op after first call).
	file_utils.clear_stale_reports()
	# Clear any state left from a previous collection in the same process.
	VIOLATIONS_BY_FILE.clear()
	VIOLATIONS_BY_FILE.update(file_utils.collect_python_violations(FILES, check_file))
	lines = file_utils.format_violation_report(HEADER, VIOLATIONS_BY_FILE)
	# Write only when there are violations; cleanup already removed stale reports.
	if lines:
		file_utils.write_report_lines(REPORT_NAME, lines)


#============================================
@pytest.mark.parametrize("path", FILES, ids=file_utils.rel_id)
def test_pytest_hygiene(path: str) -> None:
	"""Guard that hygiene tests do not reintroduce local discovery scaffold."""
	rel = file_utils.rel_to_root(path)
	# Python evaluates an assert's message expression ONLY when the assert fails,
	# so format_violation_assert_message runs on the failing path only -- not per pass.
	assert rel not in VIOLATIONS_BY_FILE, file_utils.format_violation_assert_message(
		rel, VIOLATIONS_BY_FILE.get(rel, []), REPORT_NAME
	)
