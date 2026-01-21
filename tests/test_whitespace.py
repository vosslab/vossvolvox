import os
import subprocess

import git_file_utils


SCOPE_ENV = "REPO_HYGIENE_SCOPE"
FAST_ENV = "FAST_REPO_HYGIENE"
SKIP_ENV = "SKIP_REPO_HYGIENE"
REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))

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
def resolve_scope() -> str:
	"""Resolve the scan scope from environment.

	Returns:
		str: "all" or "changed".
	"""
	scope = os.environ.get(SCOPE_ENV, "").strip().lower()
	if not scope and os.environ.get(FAST_ENV) == "1":
		scope = "changed"
	if scope in ("all", "changed"):
		return scope
	return "all"


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


#============================================
def test_whitespace_hygiene(pytestconfig) -> None:
	"""Fail on whitespace issues. Auto-fix unless --no-ascii-fix is set."""
	if os.environ.get(SKIP_ENV) == "1":
		return
	apply_fix = not pytestconfig.getoption("no_ascii_fix", default=False)

	scope = resolve_scope()
	if scope == "changed":
		paths = gather_changed_files(REPO_ROOT)
	else:
		paths = gather_files(REPO_ROOT)

	errors = []
	to_fix = []
	for path in paths:
		issues = check_whitespace(path)
		if not issues:
			continue
		rel_path = os.path.relpath(path, REPO_ROOT)
		errors.append(f"{rel_path}:0:0: " + ", ".join(sorted(set(issues))))
		to_fix.append(path)

	if not errors:
		return

	if apply_fix:
		for path in to_fix:
			run_fixer(path)
		raise AssertionError(
			"Whitespace issues were fixed. Please re-run pytest.\n" + "\n".join(errors)
		)

	raise AssertionError("Whitespace issues found:\n" + "\n".join(errors))
