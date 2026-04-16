import importlib.util
import os
import random
import re
import sys

import git_file_utils

SCOPE_ENV = "REPO_HYGIENE_SCOPE"
FAST_ENV = "FAST_REPO_HYGIENE"
SKIP_ENV = "SKIP_REPO_HYGIENE"
REPO_ROOT = git_file_utils.get_repo_root()
ERROR_RE = re.compile(r":[0-9]+:[0-9]+:")
CODEPOINT_RE = re.compile(r"non-ISO-8859-1 character U\+([0-9A-Fa-f]{4,6})")
SKIP_FILE_PATTERNS = [
	r"mkdocs.yml",
	r"^human_readable-.*\.html$",
]
SKIP_FILE_REGEXES = [re.compile(pattern) for pattern in SKIP_FILE_PATTERNS]
ERROR_SAMPLE_COUNT = 5
PROGRESS_EVERY = 1

EXTENSIONS = {
	".md",
	".txt",
	".py",
	".js",
	".jsx",
	".ts",
	".tsx",
	".html",
	".htm",
	".css",
	".json",
	".yml",
	".yaml",
	".toml",
	".ini",
	".cfg",
	".conf",
	".sh",
	".bash",
	".zsh",
	".fish",
	".csv",
	".tsv",
	".xml",
	".svg",
	".sql",
	".rb",
	".php",
	".java",
	".c",
	".h",
	".cpp",
	".hpp",
	".go",
	".rs",
	".swift",
}
SKIP_DIRS = {".git", ".venv", "old_shell_folder"}
SKIP_FILES = {
	os.path.join("CODEX", "skills", ".system", "skill-creator", "SKILL.md"),
}


#============================================
def load_module(name: str, path: str):
	"""
	Load a module from a file path without sys.path edits.

	Args:
		name: Module name to register.
		path: File path to load.
	"""
	spec = importlib.util.spec_from_file_location(name, path)
	if spec is None or spec.loader is None:
		raise RuntimeError(f"Unable to load module: {path}")
	module = importlib.util.module_from_spec(spec)
	spec.loader.exec_module(module)
	return module


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
def resolve_fix(pytestconfig) -> bool:
	"""
	Resolve whether fixes should be applied.
	"""
	return not pytestconfig.getoption("no_ascii_fix", default=False)


#============================================
def is_emoji_codepoint(codepoint: int) -> bool:
	"""
	Check whether a codepoint is likely an emoji.
	"""
	if 0x1F000 <= codepoint <= 0x1FAFF:
		return True
	if 0x2600 <= codepoint <= 0x27BF:
		return True
	return False


#============================================
def path_has_skip_dir(repo_root: str, path: str) -> bool:
	"""
	Check whether a path includes a skipped directory.
	"""
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
	"""
	Filter candidate paths by extension and skip rules.
	"""
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
		rel_path = os.path.relpath(abs_path, repo_root)
		if rel_path in SKIP_FILES:
			continue
		if path_has_skip_dir(repo_root, abs_path):
			continue
		if not os.path.isfile(abs_path):
			continue
		base_name = os.path.basename(abs_path)
		if any(regex.match(base_name) for regex in SKIP_FILE_REGEXES):
			continue
		ext = os.path.splitext(abs_path)[1].lower()
		if ext not in EXTENSIONS:
			continue
		matches.append(abs_path)
	matches.sort()
	return matches


#============================================
def gather_files(repo_root: str) -> list[str]:
	"""
	Collect tracked files to scan.
	"""
	tracked_paths = []
	for path in git_file_utils.list_tracked_files(repo_root):
		tracked_paths.append(os.path.join(repo_root, path))
	return filter_files(repo_root, tracked_paths)


#============================================
def gather_changed_files(repo_root: str) -> list[str]:
	"""
	Collect changed files using git diff and index lists.
	"""
	changed_paths = []
	for path in git_file_utils.list_changed_files(repo_root):
		changed_paths.append(os.path.join(repo_root, path))
	return filter_files(repo_root, changed_paths)


#============================================
def is_ascii_bytes(file_path: str, chunk_size: int = 1024 * 1024) -> bool:
	"""
	Check whether a file contains only ASCII bytes.
	"""
	with open(file_path, "rb") as handle:
		while True:
			chunk = handle.read(chunk_size)
			if not chunk:
				break
			if not chunk.isascii():
				return False
	return True


#============================================
def shorten_error_path(line: str) -> str:
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
def list_error_files(lines: list[str]) -> list[str]:
	"""
	Collect unique file paths from error lines.
	"""
	paths = set()
	for line in lines:
		separator = line.find(":")
		if separator == -1:
			continue
		paths.add(line[:separator])
	return sorted(paths)


#============================================
def count_error_details(lines: list[str]) -> tuple[dict[str, int], dict[str, int]]:
	"""
	Count errors by file and by Unicode codepoint.
	"""
	file_counts = {}
	codepoint_counts = {}
	for line in lines:
		match = CODEPOINT_RE.search(line)
		if not match:
			continue
		separator = line.find(":")
		if separator == -1:
			continue
		file_path = line[:separator]
		file_counts[file_path] = file_counts.get(file_path, 0) + 1
		codepoint = match.group(1).upper()
		codepoint_counts[codepoint] = codepoint_counts.get(codepoint, 0) + 1
	return file_counts, codepoint_counts


#============================================
def top_items(counts: dict[str, int], limit: int) -> list[tuple[str, int]]:
	"""
	Sort a count dictionary by descending count.
	"""
	items = sorted(counts.items(), key=lambda item: (-item[1], item[0]))
	return items[:limit]


#============================================
def format_issue_line(
	file_path: str,
	line_number: int,
	column_number: int,
	codepoint: int,
) -> str:
	"""
	Format an ASCII compliance issue line.
	"""
	display_char = chr(codepoint)
	if not display_char.isprintable():
		display_char = "?"
	codepoint_text = f"U+{codepoint:04X}"
	return (
		f"{file_path}:{line_number}:{column_number}: "
		f"non-ISO-8859-1 character {codepoint_text} {display_char}"
	)


#============================================
def scan_file(
	file_path: str,
	check_module,
	fix_module,
	apply_fix: bool,
) -> tuple[int, list[str], bool]:
	"""
	Check a file and optionally apply fixes.
	"""
	if is_ascii_bytes(file_path):
		return 0, [], False

	content, read_error = check_module.read_text(file_path)
	if read_error:
		return 1, [read_error], False

	issues = check_module.check_ascii_compliance(content)
	if not issues:
		return 0, [], False

	changed = False
	text_to_check = content
	if apply_fix:
		fixed_text, changed = fix_module.fix_ascii_compliance(content)
		if changed:
			fix_module.write_text(file_path, fixed_text)
		text_to_check = fixed_text

	issues = fix_module.find_non_latin1_chars(text_to_check)
	if not issues:
		if changed:
			return 2, [], True
		return 0, [], False

	error_lines = []
	for line_number, column_number, codepoint in issues:
		error_lines.append(
			format_issue_line(file_path, line_number, column_number, codepoint)
		)
	total_message = f"{file_path}:0:0: found {len(issues)} non-ISO-8859-1 characters"
	error_lines.append(total_message)
	return 1, error_lines, changed


#============================================
def test_ascii_compliance(pytestconfig) -> None:
	"""
	Run ASCII compliance checks across the repo.
	"""
	if os.environ.get(SKIP_ENV) == "1":
		return

	check_path = os.path.join(REPO_ROOT, "tests", "check_ascii_compliance.py")
	fix_path = os.path.join(REPO_ROOT, "tests", "fix_ascii_compliance.py")
	if not os.path.isfile(check_path):
		raise AssertionError(f"Missing script: {check_path}")
	if not os.path.isfile(fix_path):
		raise AssertionError(f"Missing script: {fix_path}")

	check_module = load_module("check_ascii_compliance", check_path)
	fix_module = load_module("fix_ascii_compliance", fix_path)

	scope = resolve_scope()
	if scope == "changed":
		files = gather_changed_files(REPO_ROOT)
	else:
		files = gather_files(REPO_ROOT)

	# Delete old report file before running
	ascii_out = os.path.join(REPO_ROOT, "report_ascii_compliance.txt")
	if os.path.exists(ascii_out):
		os.remove(ascii_out)

	if not files:
		print("No files matched the requested scope.")
		print("No errors found!!!")
		return

	apply_fix = resolve_fix(pytestconfig)
	progress_enabled = sys.stderr.isatty()
	if progress_enabled:
		print(f"ascii_compliance: scanning {len(files)} files...", file=sys.stderr)

	all_lines = []
	for index, file_path in enumerate(files, start=1):
		status, file_lines, _ = scan_file(
			file_path,
			check_module,
			fix_module,
			apply_fix,
		)
		all_lines.extend(file_lines)
		if progress_enabled and (status != 0 or index % PROGRESS_EVERY == 0):
			if status == 0:
				sys.stderr.write(".")
			elif status == 2:
				sys.stderr.write("+")
			else:
				sys.stderr.write("!")
			sys.stderr.flush()

	if progress_enabled:
		sys.stderr.write("\n")
		sys.stderr.flush()

	if not all_lines:
		print("No errors found!!!")
		return

	with open(ascii_out, "w", encoding="utf-8") as handle:
		for line in all_lines:
			handle.write(f"{line}\n")

	error_lines = [line for line in all_lines if ERROR_RE.search(line)]

	print("")
	print(f"First {ERROR_SAMPLE_COUNT} errors")
	for line in error_lines[:ERROR_SAMPLE_COUNT]:
		print(shorten_error_path(line))
	print("-------------------------")
	print("")

	print(f"Random {ERROR_SAMPLE_COUNT} errors")
	for line in sample_errors(error_lines, ERROR_SAMPLE_COUNT):
		print(shorten_error_path(line))
	print("-------------------------")
	print("")

	print(f"Last {ERROR_SAMPLE_COUNT} errors")
	for line in error_lines[-ERROR_SAMPLE_COUNT:]:
		print(shorten_error_path(line))
	print("-------------------------")
	print("")

	error_files = list_error_files(error_lines)
	error_file_count = len(error_files)
	file_counts, codepoint_counts = count_error_details(error_lines)
	emoji_count = 0
	for codepoint in codepoint_counts:
		codepoint_int = int(codepoint, 16)
		if is_emoji_codepoint(codepoint_int):
			emoji_count += codepoint_counts[codepoint]

	if error_file_count <= 5:
		print(f"Files with errors ({error_file_count})")
		for path in error_files:
			count = file_counts.get(path, 0)
			print(f"{os.path.relpath(path, REPO_ROOT)}: {count}")
		print("")
	else:
		print(f"Files with errors: {error_file_count}")
		top_files = top_items(file_counts, ERROR_SAMPLE_COUNT)
		if top_files:
			print("")
			print("Top 5 files by error count")
			for path, count in top_files:
				display_path = os.path.relpath(path, REPO_ROOT)
				print(f"{display_path}: {count}")
	top_codepoints = top_items(codepoint_counts, ERROR_SAMPLE_COUNT)
	if top_codepoints:
		print("")
		print("Top 5 Unicode characters by frequency")
		for codepoint, count in top_codepoints:
			display_char = chr(int(codepoint, 16))
			if not display_char.isprintable() or display_char.isspace():
				display_char = "?"
			print(f"U+{codepoint} {display_char}: {count}")
	if emoji_count:
		print("")
		print(f"Found {emoji_count} emoji codepoints; handle them case by case.")

	print("Found {} ASCII compliance errors written to REPO_ROOT/report_ascii_compliance.txt".format(
		len(all_lines),
	))
	raise AssertionError("ASCII compliance errors detected.")
