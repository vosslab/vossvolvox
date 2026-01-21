import os
import stat

import git_file_utils


REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
SKIP_DIRS = {
	".git",
	".venv",
	"__pycache__",
	".pytest_cache",
	".mypy_cache",
	"old_shell_folder",
}
PYTHON_SHEBANG = "#!/usr/bin/env python3"
REPORT_NAME = "report_shebang.txt"


#============================================
def iter_repo_files() -> list[str]:
	"""
	Collect tracked, non-deleted regular files under the repo root.

	Returns:
		list[str]: Absolute file paths.
	"""
	paths = []
	for rel_path in git_file_utils.list_tracked_files(REPO_ROOT):
		if path_has_skip_dir(rel_path):
			continue
		path = os.path.join(REPO_ROOT, rel_path)
		if os.path.islink(path):
			continue
		if not os.path.isfile(path):
			continue
		paths.append(path)
	return paths


#============================================
def path_has_skip_dir(rel_path: str) -> bool:
	"""
	Check whether a relative path includes a skipped directory.

	Args:
		rel_path: Path relative to the repo root.

	Returns:
		bool: True if any skipped directory is part of the path.
	"""
	parts = rel_path.split(os.sep)
	for part in parts:
		if part in SKIP_DIRS:
			return True
	return False


#============================================
def read_shebang(path: str) -> str:
	"""
	Read the shebang line if present.

	Args:
		path: File path.

	Returns:
		str: Shebang line without newline, or empty string if none.
	"""
	try:
		with open(path, "rb") as handle:
			line = handle.readline(200)
	except OSError:
		return ""
	if not line.startswith(b"#!"):
		return ""
	try:
		return line.decode("utf-8").rstrip("\n")
	except UnicodeDecodeError:
		return line.decode("utf-8", errors="replace").rstrip("\n")


#============================================
def is_executable(path: str) -> bool:
	"""
	Check whether any executable bit is set on a file.

	Args:
		path: File path.

	Returns:
		bool: True if executable for user/group/other.
	"""
	try:
		mode = os.stat(path).st_mode
	except OSError:
		return False
	return bool(mode & (stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH))


#============================================
def categorize_errors() -> dict[str, list[str]]:
	"""
	Scan repo for shebang and executable mismatches.

	Returns:
		dict[str, list[str]]: Error categories to path lists.
	"""
	errors = {
		"shebang_not_executable": [],
		"executable_no_shebang": [],
		"python_shebang_invalid": [],
	}
	for path in iter_repo_files():
		shebang = read_shebang(path)
		exec_flag = is_executable(path)
		if shebang and not exec_flag:
			errors["shebang_not_executable"].append(path)
		if exec_flag and not shebang:
			errors["executable_no_shebang"].append(path)
		if shebang and "python" in shebang:
			if shebang != PYTHON_SHEBANG:
				errors["python_shebang_invalid"].append(path)
	return errors


#============================================
def format_errors(errors: dict[str, list[str]], limit: int | None = 10) -> str:
	"""
	Format error categories for assertion output.

	Args:
		errors: Error categories and paths.
		limit: Max paths to include per category (None for all).

	Returns:
		str: Formatted error summary.
	"""
	lines = []
	for key in sorted(errors.keys()):
		paths = errors[key]
		if not paths:
			continue
		lines.append(f"{key}: {len(paths)}")
		ordered = sorted(paths)
		if limit is not None:
			ordered = ordered[:limit]
		for path in ordered:
			display_path = os.path.relpath(path, REPO_ROOT)
			lines.append(f"  {display_path}")
	return "\n".join(lines)


#============================================
def write_error_report(errors: dict[str, list[str]]) -> str:
	"""
	Write a full shebang report to a repo file.

	Args:
		errors: Error categories and paths.

	Returns:
		str: Absolute path to the report file.
	"""
	report_path = os.path.join(REPO_ROOT, REPORT_NAME)
	content = format_errors(errors, limit=None)
	with open(report_path, "w", encoding="utf-8") as handle:
		handle.write(content)
		handle.write("\n")
	return report_path


#============================================
def test_shebang_executable_alignment() -> None:
	"""
	Ensure shebangs and executable bits are aligned.
	"""
	# Delete old report file before running
	report_path = os.path.join(REPO_ROOT, REPORT_NAME)
	if os.path.exists(report_path):
		os.remove(report_path)

	errors = categorize_errors()
	if all(not values for values in errors.values()):
		return
	report_path = write_error_report(errors)
	message = format_errors(errors, limit=10)
	display_report = os.path.relpath(report_path, REPO_ROOT)
	raise AssertionError(
		"Shebang issues found:\n"
		f"{message}\n"
		f"Full report: {display_report}"
	)
