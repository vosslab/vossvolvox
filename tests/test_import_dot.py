import ast
import os
import tokenize

import pytest

import git_file_utils

REPO_ROOT = git_file_utils.get_repo_root()
SKIP_DIRS = {".git", ".venv", "__pycache__", "old_shell_folder"}
REPORT_NAME = "report_import_dot.txt"


#============================================
def path_has_skip_dir(path: str) -> bool:
	"""
	Check whether a relative path includes a skipped directory.
	"""
	parts = path.split(os.sep)
	for part in parts:
		if part in SKIP_DIRS:
			return True
	return False


#============================================
def filter_py_files(paths: list[str]) -> list[str]:
	"""
	Filter candidate paths to Python files that exist.
	"""
	matches = []
	seen = set()
	for path in paths:
		if path in seen:
			continue
		seen.add(path)
		if path_has_skip_dir(path):
			continue
		if "TEMPLATE" in path:
			continue
		if not path.endswith(".py"):
			continue
		if not os.path.isfile(path):
			continue
		matches.append(path)
	matches.sort()
	return matches


#============================================
def gather_files(repo_root: str) -> list[str]:
	"""
	Collect tracked Python files.
	"""
	paths = []
	for path in git_file_utils.list_tracked_files(
		repo_root,
		patterns=["*.py"],
		error_message="Failed to list tracked Python files.",
	):
		paths.append(os.path.join(repo_root, path))
	return filter_py_files(paths)


#============================================
def gather_changed_files(repo_root: str) -> list[str]:
	"""
	Collect changed Python files.
	"""
	paths = []
	for path in git_file_utils.list_changed_files(repo_root):
		paths.append(os.path.join(repo_root, path))
	return filter_py_files(paths)


#============================================
def read_source(path: str) -> str:
	"""
	Read Python source using tokenize.open for encoding correctness.
	"""
	with tokenize.open(path) as handle:
		text = handle.read()
	return text


#============================================
def find_relative_imports(path: str) -> list[tuple[int, str]]:
	"""
	Return line numbers for from-import statements using relative imports.
	"""
	source = read_source(path)
	try:
		tree = ast.parse(source, filename=path)
	except SyntaxError:
		return []
	matches = []
	for node in ast.walk(tree):
		if not isinstance(node, ast.ImportFrom):
			continue
		if getattr(node, "level", 0) <= 0:
			continue
		line_no = getattr(node, "lineno", 0) or 0
		module_name = node.module or ""
		import_root = f"{'.' * node.level}{module_name}"
		matches.append((line_no, import_root))
	return matches


#============================================
def format_issue(rel_path: str, line_no: int, import_root: str) -> str:
	"""
	Format a report line for a relative from-import statement.
	"""
	return f"{rel_path}:{line_no}: relative import from {import_root}"


_FILES = git_file_utils.collect_files(REPO_ROOT, gather_files, gather_changed_files)


#============================================
@pytest.fixture(scope="module", autouse=True)
def reset_import_dot_report() -> None:
	"""
	Remove stale report file before this module runs.
	"""
	report_path = os.path.join(REPO_ROOT, REPORT_NAME)
	if os.path.exists(report_path):
		os.remove(report_path)


#============================================
def append_import_dot_report(issues: list[str]) -> str:
	"""
	Append relative import violations to the dedicated report file.
	"""
	report_path = os.path.join(REPO_ROOT, REPORT_NAME)
	file_exists = os.path.exists(report_path)
	with open(report_path, "a", encoding="utf-8") as handle:
		if not file_exists:
			handle.write("Import dot report\n")
			handle.write("Violations:\n")
		for issue in issues:
			handle.write(issue + "\n")
	return report_path


#============================================
@pytest.mark.parametrize(
	"file_path", _FILES,
	ids=lambda p: os.path.relpath(p, REPO_ROOT),
)
def test_import_dot(file_path: str) -> None:
	"""Report relative from-import usage in a single Python file."""
	matches = find_relative_imports(file_path)
	if not matches:
		return
	rel_path = os.path.relpath(file_path, REPO_ROOT)
	issues = [format_issue(rel_path, line_no, import_root) for line_no, import_root in matches]
	issues = sorted(set(issues))
	report_path = append_import_dot_report(issues)
	display_report = os.path.relpath(report_path, REPO_ROOT)
	raise AssertionError(
		"relative import usage detected:\n"
		+ "\n".join(issues)
		+ f"\nFull report: {display_report}"
	)
