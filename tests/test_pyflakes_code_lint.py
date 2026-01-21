import os
import random
import re
import shutil
import subprocess

import git_file_utils

SCOPE_ENV = "REPO_HYGIENE_SCOPE"
FAST_ENV = "FAST_REPO_HYGIENE"
SKIP_ENV = "SKIP_REPO_HYGIENE"
REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
ERROR_RE = re.compile(r":[0-9]+:[0-9]+:")
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
def chunked(items: list[str], size: int) -> list[list[str]]:
	"""
	Split items into fixed-size chunks.
	"""
	chunks = []
	for start in range(0, len(items), size):
		chunks.append(items[start:start + size])
	return chunks


#============================================
def shorten_paths(line: str) -> str:
	"""
	Shorten a full error path to just the basename.
	"""
	separator = line.find(":")
	if separator == -1:
		return line
	path = line[:separator]
	remainder = line[separator:]
	return f"{os.path.basename(path)}{remainder}"


#============================================
def sample_errors(lines: list[str], count: int) -> list[str]:
	"""
	Sample up to N error lines.
	"""
	if len(lines) <= count:
		return list(lines)
	return random.sample(lines, count)


#============================================
def classify_line(line: str) -> str:
	"""
	Classify a pyflakes error line.
	"""
	if not ERROR_RE.search(line):
		return ""
	if "imported but unused" in line or "import * used" in line or "could not import" in line:
		return "import"
	if (
		"syntax error" in line
		or "SyntaxError" in line
		or "invalid syntax" in line
		or "Missing parentheses in call to" in line
		or "Did you mean print" in line
		or "from __future__ imports must occur at the beginning" in line
		or "unexpected indent" in line
		or "EOL while scanning string literal" in line
		or "EOF while scanning triple-quoted string literal" in line
		or "unterminated string literal" in line
		or "cannot assign to" in line
		or re.search(r"cannot use .* as ", line)
		or "invalid decimal literal" in line
	):
		return "syntax"
	if "undefined name" in line or "undefined local" in line or "undefined variable" in line:
		return "name"
	if (
		"assigned to but never used" in line
		or "referenced before assignment" in line
		or "redefinition of unused" in line
	):
		return "variable"
	if "Warning:" in line or "DeprecationWarning" in line or "SyntaxWarning" in line:
		return "warning"
	return "other"


#============================================
def summarize_errors(lines: list[str]) -> dict[str, int]:
	"""
	Summarize error categories.
	"""
	counts = {
		"import": 0,
		"syntax": 0,
		"name": 0,
		"variable": 0,
		"warning": 0,
		"other": 0,
	}
	for line in lines:
		label = classify_line(line)
		if not label:
			continue
		counts[label] += 1
	return counts


#============================================
def list_unclassified(lines: list[str], limit: int) -> list[str]:
	"""
	List up to N unclassified error lines.
	"""
	items = []
	for line in lines:
		label = classify_line(line)
		if not label:
			continue
		if label in ("import", "syntax", "name", "variable", "warning"):
			continue
		items.append(line)
		if len(items) >= limit:
			break
	return items


#============================================
def run_pyflakes(repo_root: str, files: list[str]) -> list[str]:
	"""
	Run pyflakes on a file list and return output lines.
	"""
	if not files:
		return []
	pyflakes_bin = shutil.which("pyflakes")
	if not pyflakes_bin:
		raise AssertionError("pyflakes not found on PATH.")
	output_lines = []
	for chunk in chunked(files, CHUNK_SIZE):
		result = subprocess.run(
			[pyflakes_bin] + chunk,
			capture_output=True,
			text=True,
			cwd=repo_root,
		)
		if result.stdout:
			output_lines.extend(result.stdout.splitlines())
		if result.stderr:
			output_lines.extend(result.stderr.splitlines())
	return output_lines


#============================================
def test_pyflakes() -> None:
	"""
	Run pyflakes across the repo.
	"""
	if os.environ.get(SKIP_ENV) == "1":
		return

	# Delete old report file before running
	pyflakes_out = os.path.join(REPO_ROOT, "report_pyflakes.txt")
	if os.path.exists(pyflakes_out):
		os.remove(pyflakes_out)

	scope = resolve_scope()
	if scope == "changed":
		files = gather_changed_files(REPO_ROOT)
	else:
		files = gather_files(REPO_ROOT)

	if not files:
		print(f"pyflakes: no Python files matched scope {scope}.")
		print("No errors found!!!")
		return

	print(f"pyflakes: scanning {len(files)} files...")
	lines = run_pyflakes(REPO_ROOT, files)
	result_count = len(lines)

	if result_count == 0:
		print("No errors found!!!")
		return

	with open(pyflakes_out, "w", encoding="utf-8") as handle:
		for line in lines:
			handle.write(f"{line}\n")

	error_lines = [line for line in lines if ERROR_RE.search(line)]

	if result_count <= SMALL_LIMIT:
		print("")
		print(f"Last {ERROR_SAMPLE_COUNT} errors")
		for line in error_lines[-ERROR_SAMPLE_COUNT:]:
			print(shorten_paths(line))
		print("-------------------------")
		print("")
		print(f"Found {result_count} pyflakes errors written to REPO_ROOT/report_pyflakes.txt")
		raise AssertionError("Pyflakes errors detected.")

	print("")
	print(f"First {ERROR_SAMPLE_COUNT} errors")
	for line in error_lines[:ERROR_SAMPLE_COUNT]:
		print(shorten_paths(line))
	print("-------------------------")
	print("")

	print(f"Random {ERROR_SAMPLE_COUNT} errors")
	for line in sample_errors(error_lines, ERROR_SAMPLE_COUNT):
		print(shorten_paths(line))
	print("-------------------------")
	print("")

	print(f"Last {ERROR_SAMPLE_COUNT} errors")
	for line in error_lines[-ERROR_SAMPLE_COUNT:]:
		print(shorten_paths(line))
	print("-------------------------")
	print("")

	counts = summarize_errors(error_lines)
	print("Error categories")
	print(f"Import errors: {counts['import']}")
	print(f"Syntax errors: {counts['syntax']}")
	print(f"Name errors: {counts['name']}")
	print(f"Variable errors: {counts['variable']}")
	print(f"Warning errors: {counts['warning']}")
	print(f"Other errors: {counts['other']}")
	print("-------------------------")
	print("")

	print(f"Unclassified errors (up to {ERROR_SAMPLE_COUNT})")
	for line in list_unclassified(error_lines, ERROR_SAMPLE_COUNT):
		print(shorten_paths(line))
	print("-------------------------")
	print("")

	print(f"Found {result_count} pyflakes errors written to REPO_ROOT/report_pyflakes.txt")
	raise AssertionError("Pyflakes errors detected.")
