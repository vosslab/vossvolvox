import os
import shutil
import subprocess

import git_file_utils


SKIP_ENV = "SKIP_REPO_HYGIENE"
REPO_ROOT = git_file_utils.get_repo_root()
SKIP_DIRS = [".git", ".venv", "old_shell_folder"]


#============================================
def path_has_skip_dir(rel_path: str) -> bool:
	"""
	Check whether a relative path includes a skipped directory.
	"""
	parts = rel_path.split(os.sep)
	for part in parts:
		if part in SKIP_DIRS:
			return True
	return False


#============================================
def list_python_files(repo_root: str) -> list[str]:
	"""
	List tracked Python files, excluding skipped directories.
	"""
	paths = []
	for rel_path in git_file_utils.list_tracked_files(
		repo_root,
		patterns=["*.py"],
		error_message="Failed to list tracked Python files.",
	):
		if path_has_skip_dir(rel_path):
			continue
		path = os.path.join(repo_root, rel_path)
		if not os.path.isfile(path):
			continue
		paths.append(path)
	return paths


#============================================
def run_bandit(repo_root: str) -> tuple[int, str]:
	"""
	Run bandit on tracked Python files and return (exit_code, combined_output).
	"""
	bandit_bin = shutil.which("bandit")
	if not bandit_bin:
		raise AssertionError("bandit not found on PATH.")
	files = list_python_files(repo_root)
	if not files:
		return (0, "")
	command = [
		bandit_bin,
		"--severity-level",
		"medium",
		"--confidence-level",
		"medium",
	] + files
	result = subprocess.run(
		command,
		capture_output=True,
		text=True,
		cwd=repo_root,
	)
	output = result.stdout + result.stderr
	return (result.returncode, output)


#============================================
def test_bandit_security() -> None:
	"""
	Run bandit at severity medium or higher.
	"""
	if os.environ.get(SKIP_ENV) == "1":
		return

	# Delete old report file before running
	bandit_out = os.path.join(REPO_ROOT, "report_bandit.txt")
	if os.path.exists(bandit_out):
		os.remove(bandit_out)

	exit_code, output = run_bandit(REPO_ROOT)
	if exit_code == 0:
		return

	with open(bandit_out, "w", encoding="utf-8") as handle:
		handle.write(output)

	raise AssertionError("Bandit issues detected. See REPO_ROOT/report_bandit.txt")
