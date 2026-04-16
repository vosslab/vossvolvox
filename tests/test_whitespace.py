import os
import subprocess

import pytest

import git_file_utils

REPO_ROOT = git_file_utils.get_repo_root()

EXTENSIONS = {
	".md",
	".txt",
	".py",
	".sh",
	".bash",
	".zsh",
	".yml",
	".yaml",
	".json",
	".toml",
	".ini",
	".cfg",
	".conf",
	".csv",
	".tsv",
	".html",
	".htm",
	".css",
}
SKIP_DIRS = {".git", ".venv", "old_shell_folder"}


#============================================
def path_has_skip_dir(repo_root: str, path: str) -> bool:
	"""Check whether a path includes a skipped directory."""
	rel_path = os.path.relpath(path, repo_root)
	if rel_path.startswith(".."):
		return True
	parts = rel_path.split(os.sep)
	for part in parts:
		if part in SKIP_DIRS:
			return True
	return False


#============================================
def filter_files(repo_root: str, paths: list[str]) -> list[str]:
	"""Filter candidate paths by extension and skip rules."""
	matches = []
	seen = set()
	for path in paths:
		abs_path = path
		if not os.path.isabs(abs_path):
			abs_path = os.path.join(repo_root, path)
		abs_path = os.path.abspath(abs_path)
		if abs_path in seen:
			continue
		seen.add(abs_path)
		if path_has_skip_dir(repo_root, abs_path):
			continue
		if not os.path.isfile(abs_path):
			continue
		ext = os.path.splitext(abs_path)[1].lower()
		if ext not in EXTENSIONS:
			continue
		matches.append(abs_path)
	matches.sort()
	return matches


#============================================
def gather_files(repo_root: str) -> list[str]:
	"""Collect tracked files to scan."""
	tracked_paths = []
	for path in git_file_utils.list_tracked_files(repo_root):
		tracked_paths.append(os.path.join(repo_root, path))
	return filter_files(repo_root, tracked_paths)


#============================================
def gather_changed_files(repo_root: str) -> list[str]:
	"""Collect changed files using git diff and index lists."""
	changed_paths = []
	for path in git_file_utils.list_changed_files(repo_root):
		changed_paths.append(os.path.join(repo_root, path))
	return filter_files(repo_root, changed_paths)


#============================================
def check_whitespace(path: str) -> list[str]:
	"""Check one file for whitespace issues."""
	issues = []
	with open(path, "rb") as handle:
		data = handle.read()
	if data.startswith(b"\xef\xbb\xbf"):
		issues.append("utf-8 BOM")
	if b"\r\n" in data:
		issues.append("CRLF line endings")
	elif b"\r" in data:
		issues.append("CR line endings")

	normalized = data.replace(b"\r\n", b"\n").replace(b"\r", b"\n")
	for line in normalized.split(b"\n"):
		if line.endswith(b" ") or line.endswith(b"\t"):
			issues.append("trailing whitespace")
			break

	if normalized and not normalized.endswith(b"\n"):
		issues.append("missing final newline")
	return issues


#============================================
def run_fixer(path: str) -> None:
	"""Run fix_whitespace.py on a file."""
	script_path = os.path.join(REPO_ROOT, "tests", "fix_whitespace.py")
	if not os.path.isfile(script_path):
		script_path = os.path.join(os.path.dirname(__file__), "fix_whitespace.py")
	result = subprocess.run(
		["python3", script_path, "-i", path],
		capture_output=True,
		text=True,
		cwd=REPO_ROOT,
	)
	if result.returncode != 0:
		message = result.stderr.strip() or "Whitespace fixer failed."
		raise AssertionError(message)


_FILES = git_file_utils.collect_files(REPO_ROOT, gather_files, gather_changed_files)


#============================================
@pytest.mark.parametrize(
	"file_path", _FILES,
	ids=lambda p: os.path.relpath(p, REPO_ROOT),
)
def test_whitespace_hygiene(file_path: str, pytestconfig) -> None:
	"""Fail on whitespace issues. Auto-fix unless --no-ascii-fix is set."""
	apply_fix = not pytestconfig.getoption("no_ascii_fix", default=False)
	issues = check_whitespace(file_path)
	if not issues:
		return
	rel_path = os.path.relpath(file_path, REPO_ROOT)
	message = f"{rel_path}: " + ", ".join(sorted(set(issues)))
	if apply_fix:
		run_fixer(file_path)
		raise AssertionError(f"Whitespace issues were fixed. Please re-run pytest.\n{message}")
	raise AssertionError(f"Whitespace issues found:\n{message}")
