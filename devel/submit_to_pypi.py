#!/usr/bin/env python3

# Standard Library
import os
import re
import sys
import shutil
import pathlib
import tomllib
import argparse
import tempfile
import subprocess

# PIP3 modules
import rich.console

DEFAULT_TESTPYPI_INDEX = "https://test.pypi.org/simple/"
DEFAULT_PYPI_INDEX = "https://pypi.org/simple/"
TESTPYPI_PROJECT_BASE = "https://test.pypi.org/project/"
PYPI_PROJECT_BASE = "https://pypi.org/project/"

console = rich.console.Console()

#============================================

def print_step(message: str) -> None:
	"""Print a step header in cyan.

	Args:
		message: The step message to print.
	"""
	console.print(message, style="bold cyan")

#============================================

def print_info(message: str) -> None:
	"""Print a normal info message.

	Args:
		message: The info message to print.
	"""
	console.print(message)

#============================================

def print_warning(message: str) -> None:
	"""Print a warning message in yellow.

	Args:
		message: The warning message to print.
	"""
	console.print(message, style="yellow")

#============================================

def print_error(message: str) -> None:
	"""Print an error message in red to stderr.

	Args:
		message: The error message to print.
	"""
	console.print(message, style="bold red", file=sys.stderr)

#============================================

def fail(message: str) -> None:
	"""Print an error and exit.

	Args:
		message: The error message to print.
	"""
	print_error(message)
	raise SystemExit(1)

#============================================

def run_command(args: list[str], cwd: str, capture: bool) -> subprocess.CompletedProcess:
	"""Run a command and fail on error.

	Args:
		args: Command arguments.
		cwd: Working directory.
		capture: Whether to capture output.

	Returns:
		The completed process.
	"""
	result = subprocess.run(
		args,
		cwd=cwd,
		text=True,
		capture_output=capture,
	)
	if result.returncode != 0:
		command_text = " ".join(args)
		fail(f"Command failed: {command_text}")
	return result

#============================================

def run_command_allow_fail(args: list[str], cwd: str, capture: bool) -> subprocess.CompletedProcess:
	"""Run a command and return the result, even if it fails.

	Args:
		args: Command arguments.
		cwd: Working directory.
		capture: Whether to capture output.

	Returns:
		The completed process.
	"""
	result = subprocess.run(
		args,
		cwd=cwd,
		text=True,
		capture_output=capture,
	)
	return result

#============================================

def parse_args() -> argparse.Namespace:
	"""Parse command line arguments.

	Returns:
		The parsed arguments.
	"""
	parser = argparse.ArgumentParser(
		description="Build and upload a Python package to PyPI or TestPyPI.",
	)

	project_group = parser.add_argument_group("project")
	project_group.add_argument(
		"-d",
		"--project-dir",
		dest="project_dir",
		default=".",
		help="Project directory with pyproject.toml.",
	)
	project_group.add_argument(
		"-p",
		"--pyproject",
		dest="pyproject_path",
		default="pyproject.toml",
		help="Path to pyproject.toml (relative to project dir).",
	)
	project_group.add_argument(
		"-n",
		"--package-name",
		dest="package_name",
		default="",
		help="Override the package name.",
	)
	project_group.add_argument(
		"-v",
		"--version",
		dest="version",
		default="",
		help="Override the package version.",
	)
	project_group.add_argument(
		"-m",
		"--import-name",
		dest="import_name",
		default="",
		help="Override the import name for the test import.",
	)

	repo_group = parser.add_argument_group("repository")
	repo_group.add_argument(
		"-r",
		"--repo",
		dest="repo",
		default="testpypi",
		choices=["testpypi", "pypi"],
		help="Repository target (default: testpypi).",
	)
	repo_group.add_argument(
		"-u",
		"--repo-url",
		dest="repo_url",
		default="",
		help="Explicit repository upload URL (overrides --repo).",
	)
	repo_group.add_argument(
		"-i",
		"--index-url",
		dest="index_url",
		default="",
		help="Index URL for version checks and test installs.",
	)

	behavior_group = parser.add_argument_group("behavior")
	behavior_group.add_argument(
		"-c",
		"--clean",
		dest="do_clean",
		help="Clean build artifacts before building.",
		action="store_true",
	)
	behavior_group.add_argument(
		"-C",
		"--no-clean",
		dest="do_clean",
		help="Skip cleaning build artifacts.",
		action="store_false",
	)
	behavior_group.set_defaults(do_clean=True)

	behavior_group.add_argument(
		"-g",
		"--upgrade-tools",
		dest="upgrade_tools",
		help="Upgrade build and upload tools.",
		action="store_true",
	)
	behavior_group.add_argument(
		"-G",
		"--no-upgrade-tools",
		dest="upgrade_tools",
		help="Skip upgrading build tools.",
		action="store_false",
	)
	behavior_group.set_defaults(upgrade_tools=True)

	behavior_group.add_argument(
		"-k",
		"--version-check",
		dest="version_check",
		help="Check for an existing version before upload.",
		action="store_true",
	)
	behavior_group.add_argument(
		"-K",
		"--skip-version-check",
		dest="version_check",
		help="Skip the version check.",
		action="store_false",
	)
	behavior_group.set_defaults(version_check=True)

	behavior_group.add_argument(
		"-t",
		"--test-install",
		dest="test_install",
		help="Test install in a temporary venv after upload.",
		action="store_true",
	)
	behavior_group.add_argument(
		"-T",
		"--no-test-install",
		dest="test_install",
		help="Skip the test install step.",
		action="store_false",
	)
	behavior_group.set_defaults(test_install=True)

	behavior_group.add_argument(
		"-o",
		"--open-browser",
		dest="open_browser",
		help="Open the project page after upload.",
		action="store_true",
	)
	behavior_group.add_argument(
		"-O",
		"--no-open-browser",
		dest="open_browser",
		help="Do not open the project page after upload.",
		action="store_false",
	)
	behavior_group.set_defaults(open_browser=True)

	args = parser.parse_args()
	return args

#============================================

def normalize_project_dir(project_dir: str) -> str:
	"""Normalize and validate the project directory.

	Args:
		project_dir: Project directory path.

	Returns:
		The absolute project directory path.
	"""
	resolved = os.path.abspath(os.path.expanduser(project_dir))
	if not os.path.isdir(resolved):
		fail(f"Project directory not found: {resolved}")
	return resolved

#============================================

def resolve_pyproject_path(project_dir: str, pyproject_path: str) -> str:
	"""Resolve and validate the pyproject.toml path.

	Args:
		project_dir: Base project directory.
		pyproject_path: Provided pyproject path.

	Returns:
		The resolved pyproject path.
	"""
	path_value = pyproject_path
	if not os.path.isabs(path_value):
		path_value = os.path.join(project_dir, path_value)
	if not os.path.isfile(path_value):
		fail(f"pyproject.toml not found: {path_value}")
	return path_value

#============================================

def read_pyproject(pyproject_path: str) -> dict:
	"""Load pyproject.toml into a dict.

	Args:
		pyproject_path: Path to pyproject.toml.

	Returns:
		The parsed pyproject data.
	"""
	with open(pyproject_path, "rb") as handle:
		data = tomllib.load(handle)
	return data

#============================================

def extract_project_metadata(pyproject_data: dict) -> tuple[str | None, str | None]:
	"""Extract package name and version from pyproject data.

	Args:
		pyproject_data: Parsed pyproject data.

	Returns:
		A tuple of (name, version) when available.
	"""
	name: str | None = None
	version: str | None = None

	project_data = pyproject_data.get("project", {})
	if project_data:
		name_value = project_data.get("name")
		version_value = project_data.get("version")
		if name_value:
			name = str(name_value)
		if version_value:
			version = str(version_value)

	if name or version:
		result = (name, version)
		return result

	tool_data = pyproject_data.get("tool", {})
	poetry_data = tool_data.get("poetry", {})
	if poetry_data:
		name_value = poetry_data.get("name")
		version_value = poetry_data.get("version")
		if name_value:
			name = str(name_value)
		if version_value:
			version = str(version_value)

	result = (name, version)
	return result

#============================================

def resolve_package_name(arg_value: str, metadata_name: str | None) -> str:
	"""Resolve the package name.

	Args:
		arg_value: Value from arguments.
		metadata_name: Value from pyproject metadata.

	Returns:
		The resolved package name.
	"""
	name = arg_value.strip() if arg_value else ""
	if not name:
		name = metadata_name or ""
	if not name:
		fail("Package name not found. Use --package-name to set it.")
	return name

#============================================

def resolve_version(arg_value: str, metadata_version: str | None) -> str:
	"""Resolve the package version.

	Args:
		arg_value: Value from arguments.
		metadata_version: Value from pyproject metadata.

	Returns:
		The resolved package version.
	"""
	version = arg_value.strip() if arg_value else ""
	if not version:
		version = metadata_version or ""
	if not version:
		fail("Package version not found. Use --version to set it.")
	return version

#============================================

def resolve_import_name(arg_value: str, package_name: str) -> str:
	"""Resolve the import name for the test import.

	Args:
		arg_value: Value from arguments.
		package_name: The package name.

	Returns:
		The resolved import name.
	"""
	import_name = arg_value.strip() if arg_value else ""
	if not import_name:
		import_name = re.sub(r"[-.]", "_", package_name)
	return import_name

#============================================

def resolve_index_url(repo: str, index_url: str) -> str:
	"""Resolve the index URL based on inputs.

	Args:
		repo: Repo name.
		index_url: Optional override.

	Returns:
		The index URL to use.
	"""
	url = index_url.strip() if index_url else ""
	if url:
		return url
	if repo == "pypi":
		return DEFAULT_PYPI_INDEX
	return DEFAULT_TESTPYPI_INDEX

#============================================

def format_bytes(size_bytes: int) -> str:
	"""Format byte counts for human-readable output.

	Args:
		size_bytes: Size in bytes.

	Returns:
		The formatted size.
	"""
	size = float(size_bytes)
	units = ["B", "KB", "MB", "GB"]
	unit_index = 0
	while size >= 1024.0 and unit_index < len(units) - 1:
		size = size / 1024.0
		unit_index += 1
	formatted = f"{size:.1f} {units[unit_index]}"
	return formatted

#============================================

def list_dist_files(dist_dir: str) -> list[pathlib.Path]:
	"""List distribution files in dist/.

	Args:
		dist_dir: Path to dist/.

	Returns:
		List of dist file paths.
	"""
	dist_path = pathlib.Path(dist_dir)
	if not dist_path.exists():
		return []
	files = sorted([path for path in dist_path.iterdir() if path.is_file()])
	return files

#============================================

def show_dist_files(dist_dir: str) -> None:
	"""Print dist files with sizes.

	Args:
		dist_dir: Path to dist/.
	"""
	files = list_dist_files(dist_dir)
	if not files:
		print_warning("No distribution files found in dist/.")
		return
	for path in files:
		size_text = format_bytes(path.stat().st_size)
		print_info(f"dist/{path.name} ({size_text})")

#============================================

def clean_build_artifacts(project_dir: str) -> None:
	"""Remove build, dist, and egg-info artifacts.

	Args:
		project_dir: Project directory.
	"""
	paths = ["build", "dist"]
	for name in paths:
		full_path = os.path.join(project_dir, name)
		if os.path.isdir(full_path):
			shutil.rmtree(full_path)

	for entry in pathlib.Path(project_dir).iterdir():
		if entry.name.endswith(".egg-info"):
			if entry.is_dir():
				shutil.rmtree(entry)
			elif entry.is_file():
				entry.unlink()

#============================================

def upgrade_build_tools(python_exe: str, project_dir: str) -> None:
	"""Upgrade build and upload tools.

	Args:
		python_exe: Python executable.
		project_dir: Project directory.
	"""
	run_command([python_exe, "-m", "pip", "install", "--upgrade", "pip"], project_dir, False)
	run_command(
		[
			python_exe,
			"-m",
			"pip",
			"install",
			"--upgrade",
			"setuptools",
			"wheel",
			"build",
			"twine",
			"packaging",
		],
		project_dir,
		False,
	)

#============================================

def parse_pip_versions_output(output: str) -> tuple[list[str], str | None]:
	"""Parse pip index versions output.

	Args:
		output: Combined stdout and stderr from pip.

	Returns:
		Tuple of (available_versions, latest_version).
	"""
	available_versions: list[str] = []
	latest_version: str | None = None

	for line in output.splitlines():
		if "LATEST:" in line:
			match = re.search(r"LATEST:\s*([^\s]+)", line)
			if match:
				latest_version = match.group(1).strip()

	for line in output.splitlines():
		if "Available versions:" in line:
			match = re.search(r"Available versions:\s*(.+)", line)
			if match:
				version_text = match.group(1)
				version_list = [item.strip() for item in version_text.split(",") if item.strip()]
				available_versions = version_list

	if not available_versions and latest_version:
		available_versions = [latest_version]

	result = (available_versions, latest_version)
	return result

#============================================

def check_version_exists(
	python_exe: str,
	project_dir: str,
	package_name: str,
	version: str,
	index_url: str,
) -> None:
	"""Check if a version already exists on the repository.

	Args:
		python_exe: Python executable.
		project_dir: Project directory.
		package_name: Package name.
		version: Version string.
		index_url: Index URL to query.
	"""
	print_step("Checking for existing versions...")
	cmd = [
		python_exe,
		"-m",
		"pip",
		"index",
		"versions",
		package_name,
		"--index-url",
		index_url,
	]
	result = run_command_allow_fail(cmd, project_dir, True)
	output = "\n".join([result.stdout, result.stderr])
	if result.returncode != 0:
		print_warning("Unable to check versions with pip index. Skipping version check.")
		return

	available_versions, latest_version = parse_pip_versions_output(output)
	if latest_version:
		print_info(f"Latest version on index: {latest_version}")
	if available_versions and version in available_versions:
		fail(f"Version {version} already exists on the index.")
	if not available_versions:
		print_warning("No versions reported by pip index.")

#============================================

def verify_dist_contents(dist_dir: str) -> None:
	"""Verify dist/ contains both wheel and sdist.

	Args:
		dist_dir: Dist directory.
	"""
	files = list_dist_files(dist_dir)
	wheel_ok = any(path.name.endswith(".whl") for path in files)
	sdist_ok = any(path.name.endswith(".tar.gz") for path in files)
	if not wheel_ok or not sdist_ok:
		fail("dist/ is missing a .whl or .tar.gz file.")

#============================================

def get_dist_args(dist_dir: str) -> list[str]:
	"""Return dist files as arguments for twine.

	Args:
		dist_dir: Dist directory.

	Returns:
		List of dist file paths.
	"""
	files = list_dist_files(dist_dir)
	if not files:
		fail("No distribution files found in dist/.")
	args = [str(path) for path in files]
	return args

#============================================

def build_package(python_exe: str, project_dir: str) -> None:
	"""Build the package.

	Args:
		python_exe: Python executable.
		project_dir: Project directory.
	"""
	print_step("Building the package...")
	run_command([python_exe, "-m", "build"], project_dir, False)

#============================================

def check_metadata(python_exe: str, project_dir: str) -> None:
	"""Run twine check on dist artifacts.

	Args:
		python_exe: Python executable.
		project_dir: Project directory.
	"""
	print_step("Checking package metadata...")
	dist_dir = os.path.join(project_dir, "dist")
	dist_args = get_dist_args(dist_dir)
	command = [python_exe, "-m", "twine", "check"]
	command.extend(dist_args)
	run_command(command, project_dir, False)

#============================================

def upload_package(
	python_exe: str,
	project_dir: str,
	repo: str,
	repo_url: str,
) -> None:
	"""Upload the package with twine.

	Args:
		python_exe: Python executable.
		project_dir: Project directory.
		repo: Repo name.
		repo_url: Optional repo URL override.
	"""
	print_step("Uploading the package...")
	dist_dir = os.path.join(project_dir, "dist")
	dist_args = get_dist_args(dist_dir)
	if repo_url:
		cmd = [python_exe, "-m", "twine", "upload", "--repository-url", repo_url]
	else:
		cmd = [python_exe, "-m", "twine", "upload", "--repository", repo]
	cmd.extend(dist_args)
	run_command(cmd, project_dir, False)

#============================================

def get_venv_python(venv_dir: str) -> str:
	"""Get the python executable in a venv.

	Args:
		venv_dir: Path to the venv directory.

	Returns:
		The python executable path.
	"""
	if os.name == "nt":
		python_path = os.path.join(venv_dir, "Scripts", "python.exe")
		return python_path
	python_path = os.path.join(venv_dir, "bin", "python")
	return python_path

#============================================

def test_install(
	python_exe: str,
	project_dir: str,
	package_name: str,
	import_name: str,
	index_url: str,
) -> None:
	"""Test install the package in a temporary venv.

	Args:
		python_exe: Python executable.
		project_dir: Project directory.
		package_name: Package name.
		import_name: Import name.
		index_url: Index URL.
	"""
	print_step("Testing install in a temporary venv...")
	with tempfile.TemporaryDirectory(prefix="pypi_upload_") as temp_dir:
		venv_dir = os.path.join(temp_dir, "venv")
		run_command([python_exe, "-m", "venv", venv_dir], project_dir, False)

		venv_python = get_venv_python(venv_dir)
		run_command([venv_python, "-m", "pip", "install", "--upgrade", "pip"], project_dir, False)
		run_command(
			[
				venv_python,
				"-m",
				"pip",
				"install",
				"--no-deps",
				"--no-cache-dir",
				"--force-reinstall",
				"--index-url",
				index_url,
				package_name,
			],
			project_dir,
			False,
		)

		import_command = f"import {import_name}; print('{import_name} successfully installed')"
		run_command([venv_python, "-c", import_command], project_dir, False)

#============================================

def resolve_project_url(repo: str, repo_url: str, package_name: str) -> str:
	"""Resolve the project page URL.

	Args:
		repo: Repo name.
		repo_url: Custom repo URL.
		package_name: Package name.

	Returns:
		The project URL or an empty string.
	"""
	if repo_url:
		return ""
	if repo == "pypi":
		return f"{PYPI_PROJECT_BASE}{package_name}/"
	return f"{TESTPYPI_PROJECT_BASE}{package_name}/"

#============================================

def open_project_url(url: str) -> None:
	"""Open the project URL in a browser when possible.

	Args:
		url: The URL to open.
	"""
	if not url:
		print_warning("No project URL to open.")
		return

	cmd: list[str] | None = None
	if sys.platform.startswith("darwin"):
		if shutil.which("open"):
			cmd = ["open", url]
	elif sys.platform.startswith("linux"):
		if shutil.which("xdg-open"):
			cmd = ["xdg-open", url]
	elif os.name == "nt":
		cmd = ["cmd", "/c", "start", "", url]

	if not cmd:
		print_warning("No browser opener found. Skipping.")
		return

	result = run_command_allow_fail(cmd, os.getcwd(), False)
	if result.returncode != 0:
		print_warning("Browser open command failed.")

#============================================

def main() -> None:
	args = parse_args()
	project_dir = normalize_project_dir(args.project_dir)
	pyproject_path = resolve_pyproject_path(project_dir, args.pyproject_path)

	pyproject_data = read_pyproject(pyproject_path)
	metadata_name, metadata_version = extract_project_metadata(pyproject_data)

	package_name = resolve_package_name(args.package_name, metadata_name)
	version = resolve_version(args.version, metadata_version)
	import_name = resolve_import_name(args.import_name, package_name)
	index_url = resolve_index_url(args.repo, args.index_url)

	print_step("Project info")
	print_info(f"Project dir: {project_dir}")
	print_info(f"pyproject: {pyproject_path}")
	print_info(f"Package name: {package_name}")
	print_info(f"Version: {version}")
	print_info(f"Import name: {import_name}")
	print_info(f"Repository: {args.repo}")
	if args.repo_url:
		print_info(f"Repository URL: {args.repo_url}")
	print_info(f"Index URL: {index_url}")

	if args.version_check:
		check_version_exists(sys.executable, project_dir, package_name, version, index_url)

	if args.upgrade_tools:
		print_step("Upgrading build tools...")
		upgrade_build_tools(sys.executable, project_dir)

	if args.do_clean:
		print_step("Cleaning build artifacts...")
		clean_build_artifacts(project_dir)

	build_package(sys.executable, project_dir)

	print_step("Verifying dist/ contents...")
	verify_dist_contents(os.path.join(project_dir, "dist"))
	show_dist_files(os.path.join(project_dir, "dist"))

	check_metadata(sys.executable, project_dir)
	upload_package(sys.executable, project_dir, args.repo, args.repo_url)

	if args.test_install:
		test_install(sys.executable, project_dir, package_name, import_name, index_url)

	project_url = resolve_project_url(args.repo, args.repo_url, package_name)
	if project_url:
		print_info(f"Project URL: {project_url}")
	if args.open_browser:
		open_project_url(project_url)

	if args.repo == "testpypi":
		print_step("Next step")
		print_info("If everything looks good, upload to PyPI with:")
		print_info("python3 -m twine upload --repository pypi dist/*")

#============================================

if __name__ == "__main__":
	main()
