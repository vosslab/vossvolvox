import ast
import os
import tokenize

import pytest

import git_file_utils

REPO_ROOT = git_file_utils.get_repo_root()
SKIP_DIRS = {".git", ".venv", "__pycache__", "old_shell_folder"}
REPORT_NAME = "report_init.txt"
_MIN_SUBSTANTIVE_LINES = 20
_MIN_CONTENT_CHARS = 100


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
def filter_init_files(paths: list[str]) -> list[str]:
	"""
	Filter candidate paths to tracked __init__.py files that exist.
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
		if os.path.basename(path) != "__init__.py":
			continue
		if not os.path.isfile(path):
			continue
		matches.append(path)
	matches.sort()
	return matches


#============================================
def gather_files(repo_root: str) -> list[str]:
	"""
	Collect tracked __init__.py files.
	"""
	paths = []
	for path in git_file_utils.list_tracked_files(
		repo_root,
		patterns=["**/__init__.py", "__init__.py"],
		error_message="Failed to list tracked __init__.py files.",
	):
		paths.append(os.path.join(repo_root, path))
	return filter_init_files(paths)


#============================================
def gather_changed_files(repo_root: str) -> list[str]:
	"""
	Collect changed __init__.py files.
	"""
	paths = []
	for path in git_file_utils.list_changed_files(repo_root):
		paths.append(os.path.join(repo_root, path))
	return filter_init_files(paths)


#============================================
def read_source(path: str) -> str:
	"""
	Read Python source using tokenize.open for encoding correctness.
	"""
	with tokenize.open(path) as handle:
		text = handle.read()
	return text


#============================================
def count_substantive_lines(source: str) -> int:
	"""
	Count non-empty, non-comment lines.
	"""
	count = 0
	for line in source.splitlines():
		stripped = line.strip()
		if not stripped:
			continue
		if stripped.startswith("#"):
			continue
		count += 1
	return count


#============================================
def should_check_file(source: str) -> bool:
	"""
	Check whether file content is substantial enough for linting.
	"""
	if count_substantive_lines(source) >= _MIN_SUBSTANTIVE_LINES:
		return True
	if len(source.strip()) >= _MIN_CONTENT_CHARS:
		return True
	return False


#============================================
def is_module_docstring(node: ast.stmt) -> bool:
	"""
	Check whether a module-level node is a literal docstring expression.
	"""
	if not isinstance(node, ast.Expr):
		return False
	value = getattr(node, "value", None)
	if not isinstance(value, ast.Constant):
		return False
	return isinstance(value.value, str)


#============================================
def extract_target_names(node: ast.stmt) -> list[str]:
	"""
	Collect direct assignment target names for simple top-level checks.
	"""
	names = []
	if isinstance(node, ast.Assign):
		targets = node.targets
	elif isinstance(node, ast.AnnAssign):
		targets = [node.target]
	elif isinstance(node, ast.AugAssign):
		targets = [node.target]
	else:
		return names
	for target in targets:
		if isinstance(target, ast.Name):
			names.append(target.id)
	return names


#============================================
def find_init_issues(path: str) -> list[tuple[int, str]]:
	"""
	Return line-numbered style violations in one __init__.py file.
	"""
	source = read_source(path)
	if not should_check_file(source):
		return []
	try:
		tree = ast.parse(source, filename=path)
	except SyntaxError as error:
		line_no = getattr(error, "lineno", 1) or 1
		return [(line_no, "syntax error in __init__.py")]
	body = list(tree.body)
	if not body:
		return []
	if len(body) == 1 and is_module_docstring(body[0]):
		return []
	issues = []
	for node in body:
		if is_module_docstring(node):
			continue
		line_no = getattr(node, "lineno", 1) or 1
		if isinstance(node, (ast.Import, ast.ImportFrom)):
			issues.append((line_no, "imports are not allowed in __init__.py"))
			continue
		if isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef, ast.ClassDef)):
			issues.append((line_no, "definitions are not allowed in __init__.py"))
			continue
		if isinstance(node, (ast.Assign, ast.AnnAssign, ast.AugAssign)):
			target_names = extract_target_names(node)
			if "__version__" in target_names:
				issues.append((line_no, "__version__ must not be assigned in __init__.py"))
				continue
			if "__all__" in target_names:
				issues.append((line_no, "__all__ is not allowed in __init__.py"))
				continue
			if any("EXPORTED_MODULES" in name for name in target_names):
				issues.append((line_no, "manual export lists are not allowed in __init__.py"))
				continue
			if any(name.endswith("_MAP") for name in target_names):
				issues.append((line_no, "function/class maps are not allowed in __init__.py"))
				continue
			issues.append((line_no, "global assignments are not allowed in __init__.py"))
			continue
		if isinstance(node, ast.If):
			issues.append((line_no, "conditional logic is not allowed in __init__.py"))
			continue
		issues.append((line_no, "implementation code is not allowed in __init__.py"))
	return issues


#============================================
def format_issue(rel_path: str, line_no: int, message: str) -> str:
	"""
	Format a report line for one __init__.py violation.
	"""
	return f"{rel_path}:{line_no}: {message}"


_FILES = git_file_utils.collect_files(REPO_ROOT, gather_files, gather_changed_files)
_PARAMS = []
for path in _FILES:
	_PARAMS.append(pytest.param(path, id=os.path.relpath(path, REPO_ROOT)))
if not _PARAMS:
	_PARAMS.append(pytest.param("", id="no-init-files"))


#============================================
@pytest.fixture(scope="module", autouse=True)
def reset_init_report() -> None:
	"""
	Remove stale report file before this module runs.
	"""
	report_path = os.path.join(REPO_ROOT, REPORT_NAME)
	if os.path.exists(report_path):
		os.remove(report_path)


#============================================
def append_init_report(issues: list[str]) -> str:
	"""
	Append __init__.py violations to the dedicated report file.
	"""
	report_path = os.path.join(REPO_ROOT, REPORT_NAME)
	file_exists = os.path.exists(report_path)
	with open(report_path, "a", encoding="utf-8") as handle:
		if not file_exists:
			handle.write("__init__.py style report\n")
			handle.write("Violations:\n")
		for issue in issues:
			handle.write(issue + "\n")
	return report_path


#============================================
@pytest.mark.parametrize(
	"file_path", _PARAMS,
)
def test_init_files(file_path: str) -> None:
	"""Report obvious __init__.py style violations in one file."""
	if not file_path:
		return
	matches = find_init_issues(file_path)
	if not matches:
		return
	rel_path = os.path.relpath(file_path, REPO_ROOT)
	issues = [format_issue(rel_path, line_no, message) for line_no, message in matches]
	issues = sorted(set(issues))
	report_path = append_init_report(issues)
	display_report = os.path.relpath(report_path, REPO_ROOT)
	raise AssertionError(
		"__init__.py style violations detected:\n"
		+ "\n".join(issues)
		+ f"\nFull report: {display_report}"
	)
