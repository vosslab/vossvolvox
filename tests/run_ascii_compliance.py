#!/usr/bin/env python3
import os
import re
import sys
import random
import argparse
import subprocess


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
PYTHON_BIN = sys.executable
ERROR_RE = re.compile(r":[0-9]+:[0-9]+:")
CODEPOINT_RE = re.compile(r"non-ISO-8859-1 character U\+([0-9A-Fa-f]{4,6})")
ERROR_SAMPLE_COUNT = 5
SCOPE_ENV = "REPO_HYGIENE_SCOPE"
FAST_ENV = "FAST_REPO_HYGIENE"


#============================================
def is_emoji_codepoint(codepoint: int) -> bool:
	"""
	Check whether a codepoint is likely an emoji.

	Args:
		codepoint: Integer Unicode codepoint.

	Returns:
		bool: True if the codepoint falls in a common emoji range.
	"""
	if 0x1F000 <= codepoint <= 0x1FAFF:
		return True
	if 0x2600 <= codepoint <= 0x27BF:
		return True
	return False


#============================================
def find_repo_root() -> str:
	"""
	Locate the repository root using git.

	Returns:
		str: Absolute repo root path, or empty string on failure.
	"""
	result = subprocess.run(
		["git", "rev-parse", "--show-toplevel"],
		capture_output=True,
		text=True,
	)
	if result.returncode != 0:
		message = result.stderr.strip()
		if not message:
			message = "Failed to determine repo root."
		print(message, file=sys.stderr)
		return ""
	return result.stdout.strip()


#============================================
def parse_args() -> argparse.Namespace:
	"""
	Parse command-line arguments.
	"""
	parser = argparse.ArgumentParser(description="Run ASCII compliance scans.")
	parser.add_argument(
		"-p", "--progress",
		dest="progress",
		action="store_true",
		help="Print progress markers while scanning files.",
	)
	parser.add_argument(
		"-q", "--quiet",
		dest="progress",
		action="store_false",
		help="Disable progress markers while scanning files.",
	)
	parser.add_argument(
		"-e", "--progress-every",
		dest="progress_every",
		type=int,
		default=1,
		help="Emit progress every N files (default: 1).",
	)
	parser.add_argument(
		"-s", "--scope",
		dest="scope",
		choices=("all", "changed"),
		default="",
		help="Scope to scan (all or changed).",
	)
	parser.set_defaults(progress=True)
	args = parser.parse_args()
	return args


#============================================
def get_progress_colors() -> dict[str, str]:
	"""
	Get ANSI color codes for progress output.

	Returns:
		dict[str, str]: Color code mapping.
	"""
	if sys.stderr.isatty():
		return {
			"reset": "\033[0m",
			"green": "\033[32m",
			"yellow": "\033[33m",
			"red": "\033[31m",
		}
	return {"reset": "", "green": "", "yellow": "", "red": ""}


#============================================
def resolve_scope(arg_scope: str) -> str:
	"""
	Resolve the scan scope from args or environment.

	Args:
		arg_scope: Scope from CLI args.

	Returns:
		str: Resolved scope ("all" or "changed").
	"""
	if arg_scope:
		return arg_scope
	env_scope = os.environ.get(SCOPE_ENV, "").strip().lower()
	if not env_scope and os.environ.get(FAST_ENV) == "1":
		env_scope = "changed"
	if env_scope in ("all", "changed"):
		return env_scope
	return "all"


#============================================
def gather_files(
	repo_root: str,
	ascii_out: str,
	pyflakes_out: str,
) -> tuple[list[str], bool]:
	"""
	Collect tracked files to scan, honoring extension and skip rules.

	Args:
		repo_root: Root path for the repo.
		ascii_out: Output path to skip.
		pyflakes_out: Output path to skip.

	Returns:
		tuple[list[str], bool]: Sorted file list and error flag.
	"""
	result = subprocess.run(
		["git", "ls-files", "-z"],
		capture_output=True,
		text=True,
		cwd=repo_root,
	)
	if result.returncode != 0:
		message = result.stderr.strip()
		if not message:
			message = "Failed to list tracked files."
		print(message, file=sys.stderr)
		return [], True
	tracked_paths = []
	for path in result.stdout.split("\0"):
		if not path:
			continue
		tracked_paths.append(os.path.join(repo_root, path))
	matches = filter_files(repo_root, tracked_paths, ascii_out, pyflakes_out)
	return matches, False


#============================================
def path_has_skip_dir(repo_root: str, path: str) -> bool:
	"""
	Check whether a path includes a skipped directory.

	Args:
		repo_root: Root path for the repo.
		path: Absolute file path.

	Returns:
		bool: True if the path contains a skipped directory.
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
def filter_files(
	repo_root: str,
	paths: list[str],
	ascii_out: str,
	pyflakes_out: str,
) -> list[str]:
	"""
	Filter candidate paths by extension and skip rules.

	Args:
		repo_root: Root path for the repo.
		paths: Candidate file paths.
		ascii_out: Output path to skip.
		pyflakes_out: Output path to skip.

	Returns:
		list[str]: Sorted filtered files.
	"""
	matches = []
	seen = set()
	skip_files = {ascii_out, pyflakes_out}
	for path in paths:
		abs_path = os.path.abspath(path)
		if abs_path in seen:
			continue
		seen.add(abs_path)
		if abs_path in skip_files:
			continue
		rel_path = os.path.relpath(abs_path, repo_root)
		if rel_path in SKIP_FILES:
			continue
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
def gather_changed_files(
	repo_root: str,
	ascii_out: str,
	pyflakes_out: str,
) -> tuple[list[str], bool]:
	"""
	Collect changed files using git diff and untracked lists.

	Args:
		repo_root: Root path for the repo.
		ascii_out: Output path to skip.
		pyflakes_out: Output path to skip.

	Returns:
		tuple[list[str], bool]: Filtered file list and error flag.
	"""
	commands = [
		["git", "diff", "--name-only", "--diff-filter=ACMRTUXB"],
		["git", "diff", "--name-only", "--cached", "--diff-filter=ACMRTUXB"],
	]
	changed_paths = []
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
			print(message, file=sys.stderr)
			return [], True
		for line in result.stdout.splitlines():
			path = line.strip()
			if not path:
				continue
			changed_paths.append(os.path.join(repo_root, path))
	filtered = filter_files(repo_root, changed_paths, ascii_out, pyflakes_out)
	return filtered, False


#============================================
def shorten_error_path(line: str) -> str:
	"""
	Shorten a full error path to just the basename.

	Args:
		line: Full error line.

	Returns:
		str: Error line with shortened path.
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

	Args:
		lines: Full list of error lines.
		count: Desired sample size.

	Returns:
		list[str]: Sampled error lines.
	"""
	if len(lines) <= count:
		return list(lines)
	return random.sample(lines, count)


#============================================
def list_error_files(lines: list[str]) -> list[str]:
	"""
	Collect unique file paths from error lines.

	Args:
		lines: Error lines with path prefixes.

	Returns:
		list[str]: Sorted unique paths.
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

	Args:
		lines: Error lines with path prefixes.

	Returns:
		tuple[dict[str, int], dict[str, int]]: File and codepoint counts.
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

	Args:
		counts: Map of key to count.
		limit: Maximum number of items to return.

	Returns:
		list[tuple[str, int]]: Sorted key/count pairs.
	"""
	items = sorted(counts.items(), key=lambda item: (-item[1], item[0]))
	return items[:limit]


#============================================
def main() -> int:
	"""
	Run the ASCII compliance check runner.

	Returns:
		int: Process exit code.
	"""
	args = parse_args()
	repo_root = find_repo_root()
	if not repo_root:
		return 1

	ascii_out = os.path.join(repo_root, "ascii_compliance.txt")
	pyflakes_out = os.path.join(repo_root, "pyflakes.txt")
	script_path = os.path.join(repo_root, "tests", "check_ascii_compliance.py")

	if not os.path.isfile(script_path):
		print(f"Missing script: {script_path}", file=sys.stderr)
		return 1

	with open(ascii_out, "w", encoding="utf-8"):
		pass

	scope = resolve_scope(args.scope)
	if scope == "changed":
		files, had_error = gather_changed_files(repo_root, ascii_out, pyflakes_out)
		if had_error:
			print("Falling back to full scan after git errors.", file=sys.stderr)
			files, had_error = gather_files(repo_root, ascii_out, pyflakes_out)
			if had_error:
				return 1
	else:
		files, had_error = gather_files(repo_root, ascii_out, pyflakes_out)
		if had_error:
			return 1
	if not files:
		print("No files matched the requested scope.")
		return 0

	colors = get_progress_colors()
	progress_every = args.progress_every
	if progress_every < 1:
		progress_every = 1
	progress_enabled = args.progress
	if progress_enabled:
		print(f"ascii_compliance: scanning {len(files)} files...", file=sys.stderr)

	with open(ascii_out, "a", encoding="utf-8") as ascii_handle:
		for index, file_path in enumerate(files, start=1):
			status = 0
			result = subprocess.run(
				[PYTHON_BIN, script_path, "-i", file_path],
				stderr=ascii_handle,
			)
			status = result.returncode
			if progress_enabled and (status != 0 or index % progress_every == 0):
				if status == 0:
					sys.stderr.write(f"{colors['green']}.{colors['reset']}")
				elif status == 2:
					sys.stderr.write(f"{colors['yellow']}+{colors['reset']}")
				else:
					sys.stderr.write(f"{colors['red']}!{colors['reset']}")
				sys.stderr.flush()

	if progress_enabled:
		sys.stderr.write("\n")
		sys.stderr.flush()

	with open(ascii_out, "r", encoding="utf-8") as handle:
		all_lines = [line.rstrip("\n") for line in handle]

	if not all_lines:
		print("No errors found!!!")
		return 0

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
			print(f"{os.path.relpath(path, repo_root)}: {count}")
		print("")
	else:
		print(f"Files with errors: {error_file_count}")
		top_files = top_items(file_counts, ERROR_SAMPLE_COUNT)
		if top_files:
			print("")
			print("Top 5 files by error count")
			for path, count in top_files:
				display_path = os.path.relpath(path, repo_root)
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

	print("Found {} ASCII compliance errors written to REPO_ROOT/ascii_compliance.txt".format(
		len(all_lines),
	))
	return 1


if __name__ == "__main__":
	raise SystemExit(main())
