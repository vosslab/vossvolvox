import pathlib
import tokenize

import pytest

import git_file_utils

REPO_ROOT = pathlib.Path(git_file_utils.get_repo_root())


#============================================
def list_tracked_python_files() -> list[pathlib.Path]:
	"""
	List tracked Python files in the repo.

	Returns:
		list[pathlib.Path]: Absolute paths to tracked .py files.
	"""
	paths: list[pathlib.Path] = []
	for line in git_file_utils.list_tracked_files(
		str(REPO_ROOT),
		patterns=["*.py"],
		error_message="Failed to list tracked Python files.",
	):
		if line.startswith("old_shell_folder/"):
			continue
		path = REPO_ROOT / line
		if not path.exists():
			continue
		paths.append(path)
	return paths


#============================================
def multiline_string_lines(path: pathlib.Path) -> set[int]:
	"""
	Collect lines that are part of multiline string tokens.

	Args:
		path: File path.

	Returns:
		set[int]: Line numbers inside multiline strings.
	"""
	in_string: set[int] = set()
	with tokenize.open(path) as handle:
		tokens = tokenize.generate_tokens(handle.readline)
		for token in tokens:
			if token.type != tokenize.STRING:
				continue
			start_line = token.start[0]
			end_line = token.end[0]
			if end_line > start_line:
				in_string.update(range(start_line, end_line + 1))
	return in_string


#============================================
def inspect_file(path: pathlib.Path) -> list[int]:
	"""
	Check a file for mixed leading indentation.

	Args:
		path: File path.

	Returns:
		list[int]: Line numbers with mixed indentation within a single line.
	"""
	ignore_lines = multiline_string_lines(path)
	with tokenize.open(path) as handle:
		lines = handle.read().splitlines()
	bad_lines = []
	for line_number, line in enumerate(lines, 1):
		if line_number in ignore_lines:
			continue
		if not line.strip():
			continue
		prefix_chars = []
		for ch in line:
			if ch == " " or ch == "\t":
				prefix_chars.append(ch)
				continue
			break
		if not prefix_chars:
			continue
		has_tab = "\t" in prefix_chars
		has_space = " " in prefix_chars
		if has_tab and has_space:
			bad_lines.append(line_number)
			continue
	return bad_lines


#============================================
def summarize_indentation(path: pathlib.Path) -> tuple[int, int] | None:
	"""
	Return first tab line and first space line if both exist.

	Args:
		path: File path.

	Returns:
		tuple or None: First tab line and first space line, or None.
	"""
	ignore_lines = multiline_string_lines(path)
	with tokenize.open(path) as handle:
		lines = handle.read().splitlines()
	first_tab_line = None
	first_space_line = None
	for line_number, line in enumerate(lines, 1):
		if line_number in ignore_lines:
			continue
		if not line.strip():
			continue
		prefix_chars = []
		for ch in line:
			if ch == " " or ch == "\t":
				prefix_chars.append(ch)
				continue
			break
		if not prefix_chars:
			continue
		has_tab = "\t" in prefix_chars
		has_space = " " in prefix_chars
		if has_tab and first_tab_line is None:
			first_tab_line = line_number
		if has_space and first_space_line is None:
			first_space_line = line_number
		if first_tab_line and first_space_line:
			return (first_tab_line, first_space_line)
	return None


_FILES = sorted(list_tracked_python_files())


#============================================
@pytest.mark.parametrize(
	"file_path", _FILES,
	ids=lambda p: str(p.relative_to(REPO_ROOT)),
)
def test_indentation_style(file_path: pathlib.Path) -> None:
	"""Fail on mixed indentation within a line or within a file."""
	display_path = file_path.relative_to(REPO_ROOT)
	bad_lines = inspect_file(file_path)
	if bad_lines:
		details = [f"{display_path}:{ln}: mixed indentation within line" for ln in bad_lines[:5]]
		raise AssertionError("\n".join(details))
	indent_lines = summarize_indentation(file_path)
	if indent_lines is not None:
		tab_line, space_line = indent_lines
		raise AssertionError(
			f"{display_path}: tabs and spaces in file "
			f"(tab line {tab_line}, space line {space_line})"
		)
