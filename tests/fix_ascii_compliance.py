#!/usr/bin/env python3
import argparse
import os
import re
import sys
import unicodedata


#============================================
def parse_args() -> argparse.Namespace:
	"""
	Parse command-line arguments.

	Returns:
		argparse.Namespace: Parsed arguments.
	"""
	parser = argparse.ArgumentParser(
		description="Fix a file for ISO-8859-1/ASCII compliance.",
	)
	parser.add_argument(
		"-i",
		"--input",
		dest="input_file",
		required=True,
		help="Input file to fix",
	)
	args = parser.parse_args()
	return args


#============================================
def read_text(input_file: str) -> tuple[str, str]:
	"""
	Read UTF-8 text from a file.

	Args:
		input_file: Path to the file.

	Returns:
		tuple[str, str]: File contents and an error string (empty if ok).
	"""
	content = ""
	error = ""
	try:
		with open(input_file, "r", encoding="utf-8") as handle:
			content = handle.read()
	except UnicodeDecodeError as exc:
		byte_index = exc.start
		error = f"{input_file}:0:0: invalid utf-8 sequence at byte {byte_index}"
	return content, error


#============================================
def normalize_line_endings(text: str) -> str:
	"""
	Normalize line endings to LF.

	Args:
		text: Input text.

	Returns:
		str: Text with LF line endings.
	"""
	normalized = text.replace("\r\n", "\n")
	normalized = normalized.replace("\r", "\n")
	return normalized


#============================================
def remove_combining_marks(text: str) -> str:
	"""
	Remove Unicode combining marks.

	Args:
		text: Input text.

	Returns:
		str: Text without combining marks.
	"""
	chars = []
	for char in text:
		if unicodedata.combining(char):
			continue
		chars.append(char)
	result = "".join(chars)
	return result


#============================================
def apply_simple_fixes(text: str) -> tuple[str, bool]:
	"""
	Apply simple replacements for ASCII/ISO-8859-1 compliance.

	Args:
		text: Input text.

	Returns:
		tuple[str, bool]: Updated text and a change flag.
	"""
	original = text

	# Normalize line endings to a consistent LF style.
	fixed_text = normalize_line_endings(original)

	# Remove accents by decomposing and stripping combining marks.
	normalized_text = unicodedata.normalize("NFD", fixed_text)
	fixed_text = remove_combining_marks(normalized_text)

	# Replace non-breaking spaces with regular spaces.
	fixed_text = fixed_text.replace("\u00A0", " ")
	fixed_text = fixed_text.replace("\u2004", " ")
	fixed_text = fixed_text.replace("\u2005", " ")
	fixed_text = fixed_text.replace("\uFEFF", " ")

	# Replace curly quotes and apostrophes with straight equivalents.
	fixed_text = re.sub(r"[\u201C\u201D]", '"', fixed_text)
	fixed_text = re.sub(r"[\u2018\u2019]", "'", fixed_text)

	# Replace angle quotes with straight double quotes.
	fixed_text = re.sub(r"[\u00AB\u00BB]", '"', fixed_text)

	# Replace dash-like characters with a simple hyphen.
	fixed_text = re.sub(
		r"[\u2010-\u2015\u2212\u2043\u2E3A\u2E3B\uFE58\uFE63\uFF0D]",
		"-",
		fixed_text,
	)

	# Replace ellipsis with three dots.
	fixed_text = fixed_text.replace("\u2026", "...")

	# Replace arrow characters with ASCII equivalents.
	fixed_text = fixed_text.replace("\u2192", "->")
	fixed_text = fixed_text.replace("\u2190", "<-")

	# Replace bullet points and middot with an asterisk.
	fixed_text = fixed_text.replace("\u2022", "*")
	fixed_text = fixed_text.replace("\u00B7", "*")

	# Replace multiplication and division signs.
	fixed_text = fixed_text.replace("\u00D7", "x")
	fixed_text = fixed_text.replace("\u00F7", "/")

	# Replace common mathematical symbols.
	fixed_text = fixed_text.replace("\u2260", "!=")
	fixed_text = fixed_text.replace("\u2264", "<=")
	fixed_text = fixed_text.replace("\u2265", ">=")
	fixed_text = fixed_text.replace("\u00B1", "+/-")
	fixed_text = fixed_text.replace("\u2248", "~")

	# Replace or drop additional symbol-like characters.
	fixed_text = fixed_text.replace("\u037C", "(c)")
	fixed_text = fixed_text.replace("\u200E", "")

	# Remove object replacement characters.
	fixed_text = fixed_text.replace("\uFFFC", "")

	changed = fixed_text != original
	return fixed_text, changed


#============================================
def fix_ascii_compliance(text: str) -> tuple[str, bool]:
	"""
	Fix text for ISO-8859-1 compliance using simple replacements.

	Args:
		text: Input text.

	Returns:
		tuple[str, bool]: Updated text and a change flag.
	"""
	fixed_text, changed = apply_simple_fixes(text)
	return fixed_text, changed


#============================================
def find_non_latin1_chars(text: str) -> list[tuple[int, int, int]]:
	"""
	Find non-ISO-8859-1 characters in text.

	Args:
		text: Input text.

	Returns:
		list[tuple[int, int, int]]: Line, column, and codepoint for each issue.
	"""
	issues = []
	line_number = 1
	column_number = 0
	for char in text:
		if char == "\n":
			line_number += 1
			column_number = 0
			continue
		column_number += 1
		codepoint = ord(char)
		if codepoint > 255:
			issues.append((line_number, column_number, codepoint))
	return issues


#============================================
def write_text(output_file: str, text: str) -> None:
	"""
	Write UTF-8 text back to a file.

	Args:
		output_file: Path to write.
		text: Text to write.
	"""
	with open(output_file, "w", encoding="utf-8", newline="\n") as handle:
		handle.write(text)


#============================================
def main() -> int:
	"""
	Run the ISO-8859-1/ASCII compliance fixer.

	Returns:
		int: Process exit code.
	"""
	args = parse_args()
	input_file = args.input_file

	if not os.path.isfile(input_file):
		message = f"{input_file}:0:0: file not found"
		print(message, file=sys.stderr)
		return 1

	content, read_error = read_text(input_file)
	if read_error:
		print(read_error, file=sys.stderr)
		return 1

	fixed_text, changed = fix_ascii_compliance(content)
	issues = find_non_latin1_chars(fixed_text)

	for line_number, column_number, codepoint in issues:
		display_char = chr(codepoint)
		if not display_char.isprintable():
			display_char = "?"
		codepoint_text = f"U+{codepoint:04X}"
		message = (
			f"{input_file}:{line_number}:{column_number}: "
			f"non-ISO-8859-1 character {codepoint_text} {display_char}"
		)
		print(message, file=sys.stderr)

	if issues:
		total = len(issues)
		total_message = f"{input_file}:0:0: found {total} non-ISO-8859-1 characters"
		print(total_message, file=sys.stderr)

	if changed:
		write_text(input_file, fixed_text)

	if issues:
		return 1
	if changed:
		return 2
	return 0


if __name__ == "__main__":
	raise SystemExit(main())
