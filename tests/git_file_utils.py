import subprocess


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
	patterns: list[str] | None = None,
	error_message: str | None = None,
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
	error_message: str | None = None,
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
