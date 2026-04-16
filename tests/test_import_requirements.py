import ast
import os
import re
import sys
import tokenize

import git_file_utils

SCOPE_ENV = "REPO_HYGIENE_SCOPE"
FAST_ENV = "FAST_REPO_HYGIENE"
SKIP_ENV = "SKIP_REPO_HYGIENE"
CHECK_OPTIONAL_IMPORTS_ENV = "CHECK_OPTIONAL_IMPORTS"
REPO_ROOT = git_file_utils.get_repo_root()
SKIP_DIRS = {".git", ".venv", "__pycache__", "old_shell_folder"}
REPORT_NAME = "report_import_requirements.txt"
REQUIREMENT_FILES = (
	"pip_requirements.txt",
	"pip_requirements-dev.txt",
	"pip_extras.txt",
	os.path.join("config_files", "pip_requirements.txt"),
	os.path.join("config_files", "pip_requirements-dev.txt"),
	os.path.join("config_files", "pip_extras.txt"),
)
LOCAL_IMPORT_WHITELIST = {
}
IMPORT_REQUIREMENT_ALIASES = {
	"applescript": "py-applescript",
	"bio": "biopython",
	"bs4": "beautifulsoup4",
	"cairo": "pycairo",
	"colour": "colour-science",
	"crypto": "pycryptodome",
	"cv2": "opencv-python",
	"docx": "python-docx",
	"google": "google-api-python-client",
	"googleapiclient": "google-api-python-client",
	"imdb": "IMDbPY",
	"image": "pillow",
	"imagedraw": "pillow",
	"imagetk": "pillow",
	"pdfminer": "pdfminer.six",
	"pil": "pillow",
	"pptx": "python-pptx",
	"rottentomatoes": "rottentomatoes-python",
	"yaml": "pyyaml",
}
REQ_NAME_RE = re.compile(r"^[A-Za-z0-9._-]+")


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
def resolve_check_optional_imports() -> bool:
	"""
	Resolve whether to enforce imports guarded by ImportError handling.
	"""
	return os.environ.get(CHECK_OPTIONAL_IMPORTS_ENV) == "1"


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
def read_source(path: str) -> str:
	"""
	Read Python source using tokenize.open for encoding correctness.
	"""
	with tokenize.open(path) as handle:
		text = handle.read()
	return text


#============================================
def normalize_name(name: str) -> str:
	"""
	Normalize a module or requirement name for comparisons.
	"""
	return name.lower().replace("-", "_").replace(".", "_")


#============================================
def parse_requirement_name(line: str) -> str:
	"""
	Parse the base requirement name from one requirements.txt line.
	"""
	plain_line = line.split("#", 1)[0].strip()
	if not plain_line:
		return ""
	if plain_line.startswith(("-", "git+", "http://", "https://")):
		return ""
	plain_line = plain_line.split(";", 1)[0].strip()
	if not plain_line:
		return ""
	plain_line = plain_line.split("[", 1)[0].strip()
	match = REQ_NAME_RE.match(plain_line)
	if not match:
		return ""
	return match.group(0)


#============================================
def load_requirement_modules(repo_root: str) -> tuple[set[str], str]:
	"""
	Load normalized requirement names from known requirement files.
	"""
	modules = set()
	source_paths = []
	for rel_path in REQUIREMENT_FILES:
		candidate = os.path.join(repo_root, rel_path)
		if not os.path.isfile(candidate):
			continue
		source_paths.append(rel_path)
		with open(candidate, "r", encoding="utf-8") as handle:
			for raw_line in handle:
				requirement_name = parse_requirement_name(raw_line)
				if not requirement_name:
					continue
				modules.add(normalize_name(requirement_name))
	if not modules:
		raise AssertionError(
			"No requirements file found. Expected one of: "
			+ ", ".join(REQUIREMENT_FILES)
		)
	return modules, ", ".join(source_paths)


#============================================
def collect_repo_module_names(paths: list[str]) -> set[str]:
	"""
	Collect importable module names from tracked Python files.
	"""
	module_names = set()
	for path in paths:
		rel_path = os.path.relpath(path, REPO_ROOT)
		for part in rel_path.split(os.sep)[:-1]:
			if part:
				module_names.add(part)
		base_name = os.path.basename(path)
		if base_name == "__init__.py":
			package_name = os.path.basename(os.path.dirname(path))
			if package_name:
				module_names.add(package_name)
			continue
		if not base_name.endswith(".py"):
			continue
		module_name = os.path.splitext(base_name)[0]
		if module_name:
			module_names.add(module_name)
	return module_names


#============================================
def is_import_error_type(node: ast.AST | None) -> bool:
	"""
	Check whether an except handler type captures import-related exceptions.
	"""
	if node is None:
		return True
	if isinstance(node, ast.Name):
		return node.id in ("ImportError", "ModuleNotFoundError")
	if isinstance(node, ast.Attribute):
		return node.attr in ("ImportError", "ModuleNotFoundError")
	if isinstance(node, ast.Tuple):
		for element in node.elts:
			if is_import_error_type(element):
				return True
	return False


#============================================
class ImportCollector(ast.NodeVisitor):
	"""
	Collect import roots and mark imports inside optional import guards.
	"""

	def __init__(self, source: str) -> None:
		self.imports: list[tuple[int, str, bool, str]] = []
		self._source = source
		self._optional_depth = 0

	def _statement_text(self, node: ast.AST, line_no: int) -> str:
		"""
		Get best-effort source text for one import statement.
		"""
		statement = ast.get_source_segment(self._source, node)
		if statement:
			return statement.strip()
		lines = self._source.splitlines()
		if 1 <= line_no <= len(lines):
			return lines[line_no - 1].strip()
		return ""

	def _record(self, line_no: int, module_name: str, node: ast.AST) -> None:
		if not module_name:
			return
		statement = self._statement_text(node, line_no)
		self.imports.append(
			(line_no, module_name, self._optional_depth > 0, statement)
		)

	def visit_Import(self, node: ast.Import) -> None:
		line_no = getattr(node, "lineno", 0) or 0
		for alias in node.names:
			root = alias.name.split(".", 1)[0]
			self._record(line_no, root, node)

	def visit_ImportFrom(self, node: ast.ImportFrom) -> None:
		if getattr(node, "level", 0):
			return
		module_name = node.module or ""
		root = module_name.split(".", 1)[0]
		line_no = getattr(node, "lineno", 0) or 0
		self._record(line_no, root, node)

	def visit_Try(self, node: ast.Try) -> None:
		import_guarded = False
		for handler in node.handlers:
			if is_import_error_type(handler.type):
				import_guarded = True
				break
		if import_guarded:
			self._optional_depth += 1
		for statement in node.body:
			self.visit(statement)
		for handler in node.handlers:
			self.visit(handler)
		for statement in node.orelse:
			self.visit(statement)
		for statement in node.finalbody:
			self.visit(statement)
		if import_guarded:
			self._optional_depth -= 1


#============================================
def collect_import_roots(path: str) -> tuple[list[tuple[int, str, bool, str]], str]:
	"""
	Collect imported root module names for one file.
	"""
	source = read_source(path)
	try:
		tree = ast.parse(source, filename=path)
	except SyntaxError as error:
		line_no = getattr(error, "lineno", 0) or 0
		message = error.msg or "Syntax error while parsing imports."
		return [], f"{line_no}: {message}"
	collector = ImportCollector(source)
	collector.visit(tree)
	return collector.imports, ""


#============================================
def normalize_statement_for_report(statement_text: str) -> str:
	"""
	Normalize statement text to one line for readable reports.
	"""
	if not statement_text:
		return "<unavailable>"
	normalized = statement_text.replace("\r\n", "\n").replace("\r", "\n")
	normalized = normalized.replace("\t", "\\t")
	normalized = normalized.replace("\n", "\\n")
	return normalized


#============================================
def write_import_report(
	repo_root: str,
	requirement_source: str,
	check_optional_imports: bool,
	parse_errors: list[str],
	import_issues: list[str],
) -> str:
	"""
	Write import-policy findings to the import-requirements report file.
	"""
	report_path = os.path.join(repo_root, REPORT_NAME)
	lines = [
		"Import requirements report",
		f"Requirements source: {requirement_source}",
		f"CHECK_OPTIONAL_IMPORTS: {int(check_optional_imports)}",
	]
	if parse_errors:
		lines.append("Parse errors:")
		for item in parse_errors:
			lines.append(item)
	if import_issues:
		lines.append("Violations:")
		for item in import_issues:
			lines.append(item)
	with open(report_path, "w", encoding="utf-8") as handle:
		handle.write("\n".join(lines))
		handle.write("\n")
	return report_path


#============================================
def get_stdlib_modules() -> set[str]:
	"""
	Get normalized module names that are part of Python stdlib.
	"""
	modules = set()
	for name in sys.builtin_module_names:
		modules.add(normalize_name(name))
	stdlib_names = getattr(sys, "stdlib_module_names", set())
	for name in stdlib_names:
		modules.add(normalize_name(name))
	return modules


#============================================
def is_allowed_module(
	module_name: str,
	repo_modules: set[str],
	stdlib_modules: set[str],
	requirement_modules: set[str],
) -> bool:
	"""
	Return True when a module passes import policy checks.
	"""
	if module_name in LOCAL_IMPORT_WHITELIST:
		return True
	if module_name in repo_modules:
		return True
	normalized = normalize_name(module_name)
	if normalized in stdlib_modules:
		return True
	if normalized in requirement_modules:
		return True
	alias = IMPORT_REQUIREMENT_ALIASES.get(normalized, "")
	if alias and normalize_name(alias) in requirement_modules:
		return True
	return False


#============================================
def test_import_requirements() -> None:
	"""
	Validate imports against stdlib, repo modules, requirements, and whitelist.
	"""
	if os.environ.get(SKIP_ENV) == "1":
		return
	report_path = os.path.join(REPO_ROOT, REPORT_NAME)
	if os.path.exists(report_path):
		os.remove(report_path)

	scope = resolve_scope()
	check_optional_imports = resolve_check_optional_imports()
	if scope == "changed":
		paths = gather_changed_files(REPO_ROOT)
	else:
		paths = gather_files(REPO_ROOT)

	if not paths:
		print(f"import-requirements: no Python files matched scope {scope}.")
		return

	repo_modules = collect_repo_module_names(paths)
	stdlib_modules = get_stdlib_modules()
	requirement_modules, requirement_source = load_requirement_modules(REPO_ROOT)

	parse_errors = []
	import_issues = []
	for path in paths:
		import_roots, parse_error = collect_import_roots(path)
		rel_path = os.path.relpath(path, REPO_ROOT)
		if parse_error:
			parse_errors.append(f"{rel_path}:{parse_error}")
			continue
		for line_no, module_name, optional_import, statement_text in import_roots:
			if module_name == "__future__":
				continue
			if optional_import and not check_optional_imports:
				continue
			if is_allowed_module(
				module_name,
				repo_modules,
				stdlib_modules,
				requirement_modules,
			):
				continue
			statement = normalize_statement_for_report(statement_text)
			import_issues.append(f"{rel_path}:{line_no}: {module_name}: {statement}")

	if parse_errors:
		report_path = write_import_report(
			REPO_ROOT,
			requirement_source,
			check_optional_imports,
			parse_errors,
			[],
		)
		display_report = os.path.relpath(report_path, REPO_ROOT)
		raise AssertionError(
			"Import rule parse errors:\n"
			+ "\n".join(parse_errors)
			+ f"\nFull report: {display_report}"
		)

	if import_issues:
		import_issues = sorted(set(import_issues))
		report_path = write_import_report(
			REPO_ROOT,
			requirement_source,
			check_optional_imports,
			[],
			import_issues,
		)
		display_report = os.path.relpath(report_path, REPO_ROOT)
		report_lines = [
			"Import policy violations found.",
			f"Requirements source: {requirement_source}",
			"Allowed imports are stdlib, repo-local modules, requirement modules, and LOCAL_IMPORT_WHITELIST.",
			"Violations:",
		]
		report_lines.extend(import_issues)
		report_lines.append(f"Full report: {display_report}")
		raise AssertionError("\n".join(report_lines))
