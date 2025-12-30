#!/usr/bin/env python3

# Standard Library
import os
import re
import shlex
import subprocess
import sys
import tempfile

# PIP3 modules
import rich.console

CHANGELOG_PATH = "docs/CHANGELOG.md"
VERSION_RE = re.compile(r"^##\s*\[?([^\]\s]+)[^\]]*\]?")
console = rich.console.Console()

#============================================

def run_git(args: list[str]) -> subprocess.CompletedProcess:
	"""Run a git command and return the completed process."""
	result = subprocess.run(
		["git"] + args,
		stdout=subprocess.PIPE,
		stderr=subprocess.PIPE,
		text=True,
	)
	return result

#============================================

def get_git_status_lines() -> list[str]:
	"""Return git status porcelain output lines.

	Returns:
		List of status lines.
	"""
	result = run_git(["status", "--porcelain=1"])
	if result.returncode != 0:
		return []
	lines = [line for line in result.stdout.splitlines() if line.strip()]
	return lines

#============================================

def format_status_entry(status_code: str, path: str) -> str:
	"""Format a git status entry.

	Args:
		status_code: Single-letter status code.
		path: File path portion.

	Returns:
		Formatted status entry.
	"""
	status_map = {
		"A": "new file",
		"M": "modified",
		"D": "deleted",
		"R": "renamed",
		"C": "copied",
		"U": "unmerged",
	}
	label = status_map.get(status_code, "modified")
	entry = f"{label}: {path}"
	return entry

#============================================

def build_git_status_block() -> str:
	"""Build a git status comment block for the commit message.

	Returns:
		Git status block as a string (may be empty).
	"""
	status_lines = get_git_status_lines()
	if not status_lines:
		return ""

	tracked: list[str] = []
	tracked_seen: set[str] = set()
	untracked: list[str] = []

	for line in status_lines:
		if line.startswith("?? "):
			untracked.append(line[3:])
			continue

		if len(line) < 3:
			continue

		index_status = line[0]
		worktree_status = line[1]
		path = line[3:]

		status_code = worktree_status
		if status_code in (" ", "?"):
			status_code = index_status
		if status_code in (" ", "?"):
			continue
		if path in tracked_seen:
			continue
		tracked_seen.add(path)
		tracked.append(format_status_entry(status_code, path))

	block_lines: list[str] = []
	block_lines.append("#")
	if tracked:
		block_lines.append("# Changes to be committed:")
		for entry in tracked:
			block_lines.append(f"#\t{entry}")

	if untracked:
		block_lines.append("#")
		block_lines.append("# Untracked files:")
		for entry in untracked:
			block_lines.append(f"#\t{entry}")

	block = "\n".join(block_lines).rstrip() + "\n"
	return block

#============================================

def ensure_in_git_repo() -> None:
	"""Raise if not inside a git work tree."""
	result = run_git(["rev-parse", "--is-inside-work-tree"])
	if result.returncode != 0:
		raise RuntimeError("Not inside a git repository.")
	if result.stdout.strip() != "true":
		raise RuntimeError("Not inside a git work tree.")

#============================================

def get_editor_cmd() -> list[str]:
	"""Return the editor command as argv."""
	editor = os.environ.get("GIT_EDITOR") or os.environ.get("EDITOR") or "nano"
	cmd = shlex.split(editor)
	return cmd

#============================================

def edit_file_in_editor(path: str) -> int:
	"""Open a file in the configured editor."""
	cmd = get_editor_cmd() + [path]
	return subprocess.run(cmd).returncode

#============================================

def build_choice_prompt(prompt: str) -> str:
	"""Build a colored prompt string with y/N choices.

	Args:
		prompt: Base prompt text.

	Returns:
		The prompt with colored y/N choices appended.
	"""
	yes_text = "[bold green]y[/bold green]"
	no_text = "[bold red]N[/bold red]"
	choice_prompt = f"{prompt} [{yes_text}/{no_text}] "
	return choice_prompt

#============================================

def print_error(message: str) -> None:
	"""Print an error message in red to stderr.

	Args:
		message: The error message to print.
	"""
	console.print(message, style="bold red", file=sys.stderr)

#============================================

def print_warning(message: str) -> None:
	"""Print a warning message in yellow.

	Args:
		message: The warning message to print.
	"""
	console.print(message, style="yellow")

#============================================

def confirm(prompt: str) -> bool:
	"""Ask the user to confirm.

	Args:
		prompt: The prompt to show before the choices.

	Returns:
		True if the user confirms.
	"""
	choice_prompt = build_choice_prompt(prompt)
	ans = console.input(choice_prompt).strip().lower()
	return ans in ("y", "yes")

#============================================

def strip_git_style_comments(message: str) -> str:
	"""Remove comment lines (starting with '#'), then trim."""
	out_lines: list[str] = []
	for line in message.splitlines():
		if line.startswith("#"):
			continue
		out_lines.append(line)
	cleaned = "\n".join(out_lines).strip()
	return cleaned

#============================================

def get_diff(path: str) -> str:
	"""Get diff for a path with minimal context and no color."""
	result = run_git(["diff", "--no-color", "--unified=0", "--", path])
	if result.returncode != 0:
		raise RuntimeError(result.stderr.strip() or f"git diff failed for {path}")
	return result.stdout.strip()

#============================================

def extract_added_lines(diff_text: str) -> list[str]:
	"""Extract added lines from a git diff (no headers, no blanks)."""
	added: list[str] = []
	for line in diff_text.splitlines():
		if not line.startswith("+"):
			continue
		if line.startswith("+++"):
			continue
		text = line[1:].rstrip()
		if text.strip() == "":
			continue
		added.append(text)
	return added

#============================================

def build_message(added_lines: list[str], max_body_lines: int) -> str:
	"""Build a subject and body from added changelog lines."""
	version: str | None = None
	for line in added_lines:
		m = VERSION_RE.match(line.strip())
		if m:
			version = m.group(1)
			break

	if version:
		subject = f"docs: update changelog for {version}"
	else:
		subject = "docs: update changelog"

	body_lines: list[str] = []
	for line in added_lines:
		s = line.strip()
		if s.startswith("##"):
			continue
		body_lines.append(s)
		if len(body_lines) >= max_body_lines:
			break

	body = "\n".join(body_lines).strip()
	if body:
		message = subject + "\n\n" + body + "\n"
	else:
		message = subject + "\n"
	return message

#============================================

def make_seed_message() -> str | None:
	"""Create the initial commit message from the changelog diff."""
	diff_text = get_diff(CHANGELOG_PATH)
	if not diff_text:
		return None

	added_lines = extract_added_lines(diff_text)
	if not added_lines:
		raise RuntimeError(f"Diff for {CHANGELOG_PATH} had no added lines.")

	message = build_message(added_lines, max_body_lines=25)
	return message

#============================================

def commit_with_editor_gate(seed_message: str) -> int:
	"""Edit the message, confirm, then commit with -a."""
	with tempfile.NamedTemporaryFile("w", delete=False, encoding="utf-8") as tf:
		tf.write(seed_message)
		tf.write("\n")
		tf.write("# Save and exit to use this message.\n")
		tf.write("# Exiting without changes will abort by default.\n")
		status_block = build_git_status_block()
		if status_block:
			tf.write(status_block)
		msg_path = tf.name

	original = strip_git_style_comments(seed_message)

	try:
		rc = edit_file_in_editor(msg_path)
		if rc != 0:
			print_error("Editor exited non-zero. Aborting.")
			return rc

		with open(msg_path, "r", encoding="utf-8") as f:
			edited_raw = f.read()

		edited = strip_git_style_comments(edited_raw)
		if not edited:
			print_error("Empty commit message. Aborting.")
			return 2

		if edited == original:
			if not confirm("Message unchanged. Commit anyway?"):
				print_warning("Aborted.")
				return 3

		cmd_str = "git commit -a -F " + shlex.quote(msg_path)
		if not confirm(f"Proceed with: {cmd_str} ?"):
			print_warning("Aborted.")
			return 4

		return subprocess.run(["git", "commit", "-a", "-F", msg_path]).returncode
	finally:
		os.unlink(msg_path)

#============================================

def main() -> None:
	ensure_in_git_repo()

	seed_message = make_seed_message()
	if seed_message is None:
		console.print(f"No changes in {CHANGELOG_PATH}. Nothing to commit.", style="yellow")
		return
	rc = commit_with_editor_gate(seed_message)
	if rc != 0:
		raise SystemExit(rc)

#============================================

if __name__ == "__main__":
	main()
