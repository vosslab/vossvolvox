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

CHANGELOG_PATHSPEC = "docs/CHANGELOG.md"
VERSION_RE = re.compile(r"^##\s*\[?([^\]\s]+)[^\]]*\]?")
console = rich.console.Console()
err_console = rich.console.Console(stderr=True)

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

def get_git_root() -> str:
	"""Return the git repository root path."""
	result = run_git(["rev-parse", "--show-toplevel"])
	if result.returncode != 0:
		raise RuntimeError("Unable to determine git repository root.")
	root = result.stdout.strip()
	if not root:
		raise RuntimeError("Git repository root is empty.")
	return root

#============================================

def get_git_status_lines() -> list[str]:
	"""Return git status porcelain output lines.

	Returns:
		List of status lines.
	"""
	result = run_git(["status", "--porcelain=1"])
	if result.returncode != 0:
		raise RuntimeError(result.stderr.strip() or "git status failed.")
	lines = [line for line in result.stdout.splitlines() if line.strip()]
	return lines

#============================================

def get_untracked_files() -> list[str]:
	"""Return a list of untracked file paths."""
	status_lines = get_git_status_lines()
	untracked: list[str] = []
	for line in status_lines:
		if line.startswith("?? "):
			untracked.append(line[3:])
	return untracked

#============================================

def get_unmerged_paths() -> list[str]:
	"""Return a list of paths with merge conflicts."""
	result = run_git(["diff", "--name-only", "--diff-filter=U"])
	if result.returncode != 0:
		raise RuntimeError(result.stderr.strip() or "git diff failed.")
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
	result = subprocess.run(cmd).returncode
	return result

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
	err_console.print(message, style="bold red")

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
	confirmed = ans in ("y", "yes")
	return confirmed

#============================================

def build_action_prompt(prompt: str) -> str:
	"""Build a colored prompt string with yes/no/commit choices.

	Args:
		prompt: Base prompt text.

	Returns:
		The prompt with colored choices appended.
	"""
	yes_text = "[bold green]yes[/bold green]"
	no_text = "[bold red]no[/bold red]"
	commit_text = "[bold cyan]commit[/bold cyan]"
	choice_prompt = f"{prompt} [{yes_text}/{no_text}/{commit_text}] "
	return choice_prompt

#============================================

def prompt_message_action(prompt: str) -> str:
	"""Ask whether to edit, exit, or commit.

	Args:
		prompt: Prompt text shown before the choices.

	Returns:
		One of: "yes", "no", or "commit".
	"""
	choice_prompt = build_action_prompt(prompt)
	while True:
		ans = console.input(choice_prompt).strip().lower()
		if ans == "":
			return "yes"
		if ans in ("y", "yes"):
			return "yes"
		if ans in ("n", "no"):
			return "no"
		if ans in ("c", "commit"):
			return "commit"
		print_warning("Please enter yes, no, or commit.")

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

def print_diff_to_stderr(diff_text: str, path: str) -> None:
	"""Print a diff to stderr with a header."""
	if not diff_text:
		return
	err_console.print(f"Diff for {path}:", style="bold")
	for line in diff_text.splitlines():
		if line.startswith("+++"):
			err_console.print(line, style="bold cyan", markup=False)
			continue
		if line.startswith("---"):
			err_console.print(line, style="bold cyan", markup=False)
			continue
		if line.startswith("@@"):
			err_console.print(line, style="bold yellow", markup=False)
			continue
		if line.startswith("+"):
			err_console.print(line, style="green", markup=False)
			continue
		if line.startswith("-"):
			err_console.print(line, style="red", markup=False)
			continue
		sys.stderr.write(line + "\n")

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

def make_seed_message(diff_text: str) -> str | None:
	"""Create the initial commit message from the changelog diff."""
	if not diff_text:
		return None
	added_lines = extract_added_lines(diff_text)
	if not added_lines:
		raise RuntimeError("Changelog diff had no added lines.")

	message = build_message(added_lines, max_body_lines=25)
	return message

#============================================

def write_message_file(seed_message: str, include_comments: bool) -> str:
	"""Write a commit message file and return its path.

	Args:
		seed_message: Initial commit message text.
		include_comments: True to include editor guidance and status.

	Returns:
		Path to the message file.
	"""
	with tempfile.NamedTemporaryFile("w", delete=False, encoding="utf-8") as tf:
		tf.write(seed_message)
		tf.write("\n")
		if include_comments:
			tf.write("# Save and exit to use this message.\n")
			tf.write("# You will confirm the commit after editing.\n")
			status_block = build_git_status_block()
			if status_block:
				tf.write(status_block)
		return tf.name

#============================================

def edit_message(seed_message: str) -> str | None:
	"""Edit a commit message and return the message file path."""
	msg_path = write_message_file(seed_message, include_comments=True)
	rc = edit_file_in_editor(msg_path)
	if rc != 0:
		print_error("Editor exited non-zero. Aborting.")
		os.unlink(msg_path)
		return None

	with open(msg_path, "r", encoding="utf-8") as f:
		edited_raw = f.read()

	edited = strip_git_style_comments(edited_raw)
	if not edited:
		print_error("Empty commit message. Aborting.")
		os.unlink(msg_path)
		return None

	with open(msg_path, "w", encoding="utf-8") as f:
		f.write(edited)
		f.write("\n")

	return msg_path

#============================================

def commit_with_message_file(msg_path: str) -> int:
	"""Commit with a prepared message file."""
	result = subprocess.run(["git", "commit", "-a", "-F", msg_path]).returncode
	return result

#============================================

def main() -> None:
	ensure_in_git_repo()
	repo_root = get_git_root()
	os.chdir(repo_root)
	changelog_path = CHANGELOG_PATHSPEC

	unmerged = get_unmerged_paths()
	if unmerged:
		print_error("Merge conflicts detected. Resolve before committing.")
		for path in unmerged:
			sys.stderr.write(f"  {path}\n")
		return

	untracked = get_untracked_files()
	if untracked:
		sys.stderr.write("Untracked files:\n")
		for path in untracked:
			sys.stderr.write(f"  {path}\n")
		if not confirm("Keep untracked files untracked?"):
			print_warning("Aborted.")
			return

	diff_text = get_diff(CHANGELOG_PATHSPEC)
	if not diff_text:
		message = f"No changes in {changelog_path}. Nothing to commit."
		console.print(message, style="yellow")
		return

	print_diff_to_stderr(diff_text, changelog_path)
	seed_message = make_seed_message(diff_text)
	if seed_message is None:
		return

	action = prompt_message_action("Add to the commit message?")
	if action == "no":
		print_warning("Aborted.")
		return

	if action == "yes":
		msg_path = edit_message(seed_message)
		if msg_path is None:
			return
		if not confirm("Commit now?"):
			print_warning("Aborted.")
			os.unlink(msg_path)
			return
		rc = commit_with_message_file(msg_path)
		os.unlink(msg_path)
	elif action == "commit":
		msg_path = write_message_file(seed_message, include_comments=False)
		rc = commit_with_message_file(msg_path)
		os.unlink(msg_path)
	else:
		print_error("Unknown action. Aborting.")
		return

	if rc != 0:
		raise SystemExit(rc)
	console.print("Reminder: run git pull and git push.", style="yellow")

#============================================

if __name__ == "__main__":
	main()
