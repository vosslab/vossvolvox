# Standard Library
import ast
import os
import subprocess
import sys
import tokenize


SCOPE_ENV = "REPO_HYGIENE_SCOPE"
FAST_ENV = "FAST_REPO_HYGIENE"
SKIP_ENV = "SKIP_REPO_HYGIENE"
REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))

IMPORT_RULES_MIN_SEVERITY_ENV = "IMPORT_RULES_MIN_SEVERITY"

SEVERITY_LOW = "low"
SEVERITY_MEDIUM = "medium"
SEVERITY_HIGH = "high"

SEVERITY_RANK = {
	SEVERITY_LOW: 1,
	SEVERITY_MEDIUM: 2,
	SEVERITY_HIGH: 3,
}

SKIP_PREFIXES = (
	"old_shell_folder/",
)

GROUP_FUTURE = "future"
GROUP_STDLIB = "stdlib"
GROUP_THIRD = "third_party"
GROUP_LOCAL = "local"

CANONICAL_ALIASES = {
	("numpy", "np"),
	("pandas", "pd"),
}


#============================================
def resolve_scope() -> str:
	"""Resolve the scan scope from environment.

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
def min_severity_threshold() -> str:
	"""Resolve minimum severity that fails the test.

	Returns:
		str: one of "low", "medium", "high"
	"""
	value = os.environ.get(IMPORT_RULES_MIN_SEVERITY_ENV, "").strip().lower()
	if value in (SEVERITY_LOW, SEVERITY_MEDIUM, SEVERITY_HIGH):
		return value
	return SEVERITY_MEDIUM


#============================================
def run_git(repo_root: str, args: list[str]) -> str:
	"""Run a git command and return stdout."""
	result = subprocess.run(
		["git", "-C", repo_root] + args,
		capture_output=True,
		text=True,
	)
	if result.returncode != 0:
		message = result.stderr.strip() or f"git failed: {' '.join(args)}"
		raise AssertionError(message)
	return result.stdout


#============================================
def gather_tracked_python_files(repo_root: str) -> list[str]:
	"""Collect tracked .py files."""
	stdout = run_git(repo_root, ["ls-files", "-z", "--", "*.py"])
	paths = []
	for rel_path in stdout.split("\0"):
		if not rel_path:
			continue
		if _has_skip_prefix(rel_path):
			continue
		abs_path = os.path.join(repo_root, rel_path)
		if os.path.isfile(abs_path):
			paths.append(abs_path)
	paths.sort()
	return paths


#============================================
def gather_changed_python_files(repo_root: str) -> list[str]:
	"""Collect changed .py files (working tree and index)."""
	commands = [
		["diff", "--name-only", "--diff-filter=ACMRTUXB", "-z", "--", "*.py"],
		["diff", "--name-only", "--cached", "--diff-filter=ACMRTUXB", "-z", "--", "*.py"],
	]
	paths = []
	seen = set()
	for args in commands:
		stdout = run_git(repo_root, args)
		for rel_path in stdout.split("\0"):
			if not rel_path:
				continue
			if rel_path in seen:
				continue
			seen.add(rel_path)
			if _has_skip_prefix(rel_path):
				continue
			abs_path = os.path.join(repo_root, rel_path)
			if os.path.isfile(abs_path):
				paths.append(abs_path)
	paths.sort()
	return paths


#============================================
def _has_skip_prefix(rel_path: str) -> bool:
	"""Return True if rel_path is skipped by prefix."""
	for prefix in SKIP_PREFIXES:
		if rel_path.startswith(prefix):
			return True
	return False


#============================================
def read_source(path: str) -> str:
	"""Read Python source using tokenize.open for encoding correctness."""
	with tokenize.open(path) as handle:
		text = handle.read()
	return text


#============================================
def local_top_level_modules(repo_root: str) -> set[str]:
	"""Collect plausible local top-level module names from repo root."""
	names: set[str] = set()
	for entry in os.listdir(repo_root):
		if entry.startswith("."):
			continue
		if entry in ("tests", "test", "__pycache__"):
			continue
		full_path = os.path.join(repo_root, entry)
		if os.path.isfile(full_path) and entry.endswith(".py"):
			names.add(entry[:-3])
			continue
		if os.path.isdir(full_path):
			init_path = os.path.join(full_path, "__init__.py")
			if os.path.isfile(init_path):
				names.add(entry)
				continue
	return names


#============================================
def is_stdlib_top_name(top_name: str) -> bool:
	"""Check whether a top-level name is part of the standard library."""
	if top_name == "__future__":
		return True
	stdlib_names = getattr(sys, "stdlib_module_names", None)
	if stdlib_names is None:
		return False
	return top_name in stdlib_names


#============================================
def group_for_module(top_name: str, local_names: set[str]) -> str:
	"""Assign an import group."""
	if top_name == "__future__":
		return GROUP_FUTURE
	if is_stdlib_top_name(top_name):
		return GROUP_STDLIB
	if top_name in local_names:
		return GROUP_LOCAL
	return GROUP_THIRD


#============================================
def import_sort_key(name: str) -> tuple[int, str]:
	"""Sort key: shortest first, then alphabetical."""
	return (len(name), name)


#============================================
def first_top_level_name(module_name: str | None) -> str:
	"""Extract the first name in a dotted module path."""
	if not module_name:
		return ""
	return module_name.split(".", 1)[0]


#============================================
def find_module_docstring_end(tree: ast.AST) -> int:
	"""Return the last line of the module docstring, or 0 if none."""
	if not isinstance(tree, ast.Module):
		return 0
	if not tree.body:
		return 0
	first = tree.body[0]
	if not isinstance(first, ast.Expr):
		return 0
	value = getattr(first, "value", None)
	if isinstance(value, ast.Constant) and isinstance(value.value, str):
		return getattr(first, "end_lineno", None) or getattr(first, "lineno", 0) or 0
	return 0


#============================================
def check_headings(source: str, first_import_line_by_group: dict[str, int]) -> list[tuple[str, int, str]]:
	"""Enforce group headings if 2+ groups are present (excluding __future__).

	Returns:
		list: list of (severity, line_no, message)
	"""
	lines = source.splitlines()
	issues: list[tuple[str, int, str]] = []

	present_groups = [g for g in (GROUP_STDLIB, GROUP_THIRD, GROUP_LOCAL) if g in first_import_line_by_group]
	if len(present_groups) < 2:
		return issues

	expected = {
		GROUP_STDLIB: "# Standard Library",
		GROUP_THIRD: "# PIP3 modules",
		GROUP_LOCAL: "# local repo modules",
	}

	for group in present_groups:
		line_no = first_import_line_by_group[group]
		idx = line_no - 2
		found = None
		while idx >= 0:
			raw = lines[idx]
			stripped = raw.strip()
			if not stripped:
				idx -= 1
				continue
			found = stripped
			break
		if found != expected[group]:
			issues.append(
				(SEVERITY_LOW, line_no, f"missing or wrong heading (expected {expected[group]!r})")
			)

	return issues


#============================================
def severity_at_least(severity: str, threshold: str) -> bool:
	"""Return True if severity >= threshold."""
	return SEVERITY_RANK[severity] >= SEVERITY_RANK[threshold]


#============================================
def inspect_imports(path: str, local_names: set[str]) -> list[tuple[str, int, str]]:
	"""Inspect one file for import style issues.

	Returns:
		list: list of (severity, line_no, message)
	"""
	source = read_source(path)
	try:
		tree = ast.parse(source, filename=path)
	except SyntaxError as exc:
		line_no = exc.lineno or 0
		return [(SEVERITY_HIGH, line_no, f"syntax error: {exc.msg}")]

	issues: list[tuple[str, int, str]] = []
	module_doc_end = find_module_docstring_end(tree)

	import_nodes: list[ast.AST] = []
	seen_non_import = False
	for node in getattr(tree, "body", []):
		if isinstance(node, (ast.Import, ast.ImportFrom)):
			if seen_non_import:
				line_no = getattr(node, "lineno", 0) or 0
				issues.append((SEVERITY_HIGH, line_no, "import not at top of file"))
			import_nodes.append(node)
			continue
		if isinstance(node, ast.Expr):
			value = getattr(node, "value", None)
			if isinstance(value, ast.Constant) and isinstance(value.value, str):
				continue
		seen_non_import = True

	entries = []
	first_import_line_by_group: dict[str, int] = {}
	last_group = None
	last_key_by_group: dict[str, tuple[int, str]] = {}

	for node in import_nodes:
		line_no = getattr(node, "lineno", 0) or 0
		if line_no and line_no <= module_doc_end:
			issues.append((SEVERITY_MEDIUM, line_no, "import inside module docstring region"))
			continue

		if isinstance(node, ast.Import):
			if len(node.names) != 1:
				issues.append((SEVERITY_MEDIUM, line_no, "multiple imports on one line"))
				continue
			alias = node.names[0]
			module_name = alias.name
			top_name = first_top_level_name(module_name)
			group = group_for_module(top_name, local_names)
			sort_name = module_name

			if alias.asname is not None:
				if (module_name, alias.asname) in CANONICAL_ALIASES:
					issues.append((SEVERITY_LOW, line_no, f"canonical alias used: {module_name} as {alias.asname}"))
				else:
					issues.append((SEVERITY_MEDIUM, line_no, f"avoid alias imports: {module_name} as {alias.asname}"))
		else:
			if node.level and node.level > 0:
				issues.append((SEVERITY_HIGH, line_no, "relative import not allowed"))
				continue
			if len(node.names) != 1:
				issues.append((SEVERITY_MEDIUM, line_no, "multiple from-imports on one line"))
				continue
			alias = node.names[0]
			if alias.name == "*":
				issues.append((SEVERITY_HIGH, line_no, "import * is not allowed"))
				continue
			module_name = node.module or ""
			top_name = first_top_level_name(module_name)
			group = group_for_module(top_name, local_names)
			sort_name = module_name

			if alias.asname is not None:
				issues.append((SEVERITY_MEDIUM, line_no, f"avoid alias in from-import: {alias.name} as {alias.asname}"))

			if group not in (GROUP_FUTURE, GROUP_STDLIB):
				issues.append((SEVERITY_MEDIUM, line_no, "avoid 'from' imports for non-stdlib modules"))

		if group not in first_import_line_by_group:
			first_import_line_by_group[group] = line_no

		entries.append((line_no, group, sort_name))

	issues.extend(check_headings(source, first_import_line_by_group))

	for line_no, group, sort_name in entries:
		if last_group is not None:
			order = {
				GROUP_FUTURE: 0,
				GROUP_STDLIB: 1,
				GROUP_THIRD: 2,
				GROUP_LOCAL: 3,
			}
			if order[group] < order[last_group]:
				issues.append((SEVERITY_LOW, line_no, f"import group order wrong ({group} after {last_group})"))

		key = import_sort_key(sort_name)
		prev_key = last_key_by_group.get(group)
		if prev_key is not None and key < prev_key:
			issues.append((SEVERITY_LOW, line_no, f"imports not sorted in {group} group ({sort_name})"))
		last_key_by_group[group] = key
		last_group = group

	return issues


#============================================
def test_import_hygiene(pytestconfig) -> None:
	"""Fail on import style issues."""
	if os.environ.get(SKIP_ENV) == "1":
		return

	scope = resolve_scope()
	if scope == "changed":
		paths = gather_changed_python_files(REPO_ROOT)
	else:
		paths = gather_tracked_python_files(REPO_ROOT)

	local_names = local_top_level_modules(REPO_ROOT)
	threshold = min_severity_threshold()

	all_issues = []
	failing_issues = []
	for path in paths:
		issues = inspect_imports(path, local_names)
		if not issues:
			continue
		rel_path = os.path.relpath(path, REPO_ROOT)
		for severity, line_no, message in issues[:12]:
			line = f"{rel_path}:{line_no}: {severity}: {message}"
			all_issues.append(line)
			if severity_at_least(severity, threshold):
				failing_issues.append(line)

	if failing_issues:
		raise AssertionError(
			f"Import issues found (threshold: {threshold}):\n" + "\n".join(all_issues)
		)
