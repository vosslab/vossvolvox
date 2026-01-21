import os
import shutil
import subprocess


SCOPE_ENV = "REPO_HYGIENE_SCOPE"
FAST_ENV = "FAST_REPO_HYGIENE"
SKIP_ENV = "SKIP_REPO_HYGIENE"
REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
ERROR_SAMPLE_COUNT = 5
SMALL_LIMIT = 20
CHUNK_SIZE = 200
SKIP_DIRS = {".git", ".venv", "old_shell_folder"}


#============================================
def resolve_scope() -> str:
	"""
	Resolve the scan scope from environment.
	"""
	scope = os.environ.get(SCOPE_ENV, "").strip().lower()
	if not scope and os.environ.get(FAST_ENV) == "1":
		scope = "changed"
	if scope in ("all", "changed"):
		return scope
	return "all"


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
	result = subprocess.run(
		["git", "ls-files", "-z", "--", "*.py"],
		capture_output=True,
		text=True,
		cwd=repo_root,
	)
	if result.returncode != 0:
		message = result.stderr.strip()
		if not message:
			message = "Failed to list tracked Python files."
		raise AssertionError(message)
	paths = []
	for path in result.stdout.split("\0"):
		if not path:
			continue
		paths.append(os.path.join(repo_root, path))
	return filter_py_files(paths)


#============================================
def gather_changed_files(repo_root: str) -> list[str]:
	"""
	Collect changed Python files.
	"""
	commands = [
		["git", "diff", "--name-only", "--diff-filter=ACMRTUXB", "-z"],
		["git", "diff", "--name-only", "--cached", "--diff-filter=ACMRTUXB", "-z"],
	]
	paths = []
	for command in commands:
		result = subprocess.run(
			command,
			capture_output=True,
			text=True,
			cwd=repo_root,
		)
		if result.returncode != 0:
			message = result.stderr.strip()
			if not message:
				message = "Failed to list changed files."
			raise AssertionError(message)
		for path in result.stdout.split("\0"):
			if not path:
				continue
			paths.append(os.path.join(repo_root, path))
	return filter_py_files(paths)


#============================================
def chunked(items: list[str], size: int) -> list[list[str]]:
	"""
	Split items into fixed-size chunks.
	"""
	chunks = []
	for start in range(0, len(items), size):
		chunks.append(items[start:start + size])
	return chunks


#============================================
def run_pyright(repo_root: str, files: list[str]) -> tuple[bool, list[str]]:
	"""
	Run pyright on a file list and return (had_failure, output_lines).
	"""
	if not files:
		return (False, [])
	pyright_bin = shutil.which("pyright")
	if not pyright_bin:
		raise AssertionError("pyright not found on PATH.")
	output_lines = []
	had_failure = False
	pyright_config = os.path.join(REPO_ROOT, "tests", "pyrightconfig.json")
	for chunk in chunked(files, CHUNK_SIZE):
		result = subprocess.run(
			[pyright_bin, "-p", pyright_config] + chunk,
			capture_output=True,
			text=True,
			cwd=repo_root,
		)
		if result.returncode != 0:
			had_failure = True
		if result.stdout:
			output_lines.extend(result.stdout.splitlines())
		if result.stderr:
			output_lines.extend(result.stderr.splitlines())
	return (had_failure, output_lines)


#============================================
def sample_lines(lines: list[str], count: int) -> list[str]:
	"""
	Take the first N non-empty lines.
	"""
	items = []
	for line in lines:
		value = line.strip()
		if not value:
			continue
		items.append(value)
		if len(items) >= count:
			break
	return items


#============================================
def test_pyright_code_lint() -> None:
	"""
	Run pyright across the repo.
	"""
	if os.environ.get(SKIP_ENV) == "1":
		return

	# Delete old report file before running
	pyright_out = os.path.join(REPO_ROOT, "report_pyright.txt")
	if os.path.exists(pyright_out):
		os.remove(pyright_out)

	scope = resolve_scope()
	if scope == "changed":
		files = gather_changed_files(REPO_ROOT)
	else:
		files = gather_files(REPO_ROOT)

	if not files:
		print(f"pyright: no Python files matched scope {scope}.")
		print("No errors found!!!")
		return

	print(f"pyright: scanning {len(files)} files...")
	had_failure, lines = run_pyright(REPO_ROOT, files)

	if not had_failure:
		print("No errors found!!!")
		return

	with open(pyright_out, "w", encoding="utf-8") as handle:
		for line in lines:
			handle.write(f"{line}\n")

	print("")
	print(f"First {ERROR_SAMPLE_COUNT} lines")
	for line in sample_lines(lines, ERROR_SAMPLE_COUNT):
		print(line)
	print("-------------------------")
	print("")
	print("Found pyright issues written to REPO_ROOT/report_pyright.txt")
	raise AssertionError("Pyright errors detected.")
