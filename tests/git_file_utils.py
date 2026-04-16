import os
import subprocess
from typing import Optional

SCOPE_ENV = "REPO_HYGIENE_SCOPE"
FAST_ENV = "FAST_REPO_HYGIENE"
SKIP_ENV = "SKIP_REPO_HYGIENE"


#============================================
def get_repo_root() -> str:
	"""
	Get the repository root using git rev-parse --show-toplevel.

	Returns:
		str: Absolute path to the repository root.
	"""
	return subprocess.check_output(
		["git", "rev-parse", "--show-toplevel"],
		text=True,
	).strip()


#============================================
def _run_git(repo_root: str, args: list[str], error_message: str) -> str:
	"""
	Run a git command and return stdout.

	Args:
		repo_root: Repo root used as the working directory.
		args: Git command argument list.
		error_message: Fallback error message.

	Returns:
		str: Command stdout.
	"""
	result = subprocess.run(
		args,
		capture_output=True,
		text=True,
		cwd=repo_root,
	)
	if result.returncode != 0:
		message = result.stderr.strip() or error_message
		raise AssertionError(message)
	return result.stdout


#============================================
def _split_null(output: str) -> list[str]:
	"""
	Split a NUL-separated stdout string into paths.
	"""
	paths = []
	for path in output.split("\0"):
		if not path:
			continue
		paths.append(path)
	return paths


#============================================
def list_tracked_files(
	repo_root: str,
	patterns: Optional[list[str]] = None,
	error_message: Optional[str] = None,
) -> list[str]:
	"""
	List tracked files using git ls-files.
	"""
	if error_message is None:
		error_message = "Failed to list tracked files."
	command = ["git", "ls-files", "-z"]
	if patterns:
		command += ["--"] + patterns
	output = _run_git(repo_root, command, error_message)
	return _split_null(output)


#============================================
def list_changed_files(
	repo_root: str,
	diff_filter: str = "ACMRTUXB",
	error_message: Optional[str] = None,
) -> list[str]:
	"""
	List changed files using git diff and index lists.
	"""
	if error_message is None:
		error_message = "Failed to list changed files."
	commands = [
		["git", "diff", "--name-only", f"--diff-filter={diff_filter}", "-z"],
		["git", "diff", "--name-only", "--cached", f"--diff-filter={diff_filter}", "-z"],
	]
	paths = []
	for command in commands:
		output = _run_git(repo_root, command, error_message)
		paths.extend(_split_null(output))
	return paths


#============================================
def resolve_scope() -> str:
	"""
	Resolve the scan scope from environment.

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
def collect_files(
	repo_root: str,
	gather_all_fn: callable,
	gather_changed_fn: callable,
) -> list[str]:
	"""
	Collect files based on scope, returning empty list when skipped.

	Args:
		repo_root: Absolute path to the repository root.
		gather_all_fn: Callable that takes repo_root and returns all files.
		gather_changed_fn: Callable that takes repo_root and returns changed files.

	Returns:
		list[str]: File paths to process.
	"""
	if os.environ.get(SKIP_ENV) == "1":
		return []
	scope = resolve_scope()
	if scope == "changed":
		return gather_changed_fn(repo_root)
	return gather_all_fn(repo_root)
