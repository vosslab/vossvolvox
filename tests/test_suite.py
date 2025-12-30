#!/usr/bin/env python3
"""Modular test harness that executes commands defined in a YAML file."""

from __future__ import annotations

import argparse
import gzip
import hashlib
import shutil
import subprocess
import sys
import textwrap
import time
import urllib.error
import urllib.request
from pathlib import Path
from typing import Any, Dict, List, Tuple

try:
    import yaml
except ImportError:  # pragma: no cover - handled at runtime
    print("PyYAML is required (pip install pyyaml)", file=sys.stderr)
    sys.exit(1)

def get_git_root() -> Path:
    try:
        root = subprocess.check_output(
            ["git", "rev-parse", "--show-toplevel"],
            cwd=Path(__file__).resolve().parent,
            text=True,
        ).strip()
        return Path(root)
    except subprocess.CalledProcessError:
        return Path(__file__).resolve().parent.parent


REPO_ROOT = get_git_root()
TEST_DIR = REPO_ROOT / "tests"
BIN_DIR = REPO_ROOT / "bin"
SRC_DIR = REPO_ROOT / "src"

def _color(code: str, text: str) -> str:
    if sys.stdout.isatty():
        return f"\033[{code}m{text}\033[0m"
    return text

def info(msg: str) -> None:
    print(_color("34", msg))  # blue

def success(msg: str) -> None:
    print(_color("32", msg))  # green

def warning(msg: str) -> None:
    print(_color("33", msg), file=sys.stderr)  # yellow

def error(msg: str) -> None:
    print(_color("31", msg), file=sys.stderr)  # red


class TestFailure(RuntimeError):
    """Raised when a test expectation is not met."""


class TestFailureWithChecks(TestFailure):
    """Raised when checks are evaluated and at least one failed."""

    def __init__(self, message: str, checks: List[Tuple[str, bool, str]], duration: float) -> None:
        super().__init__(message)
        self.checks = checks
        self.duration = duration
        self.passed = sum(1 for _, ok, _ in checks if ok)
        self.total = len(checks)


def run(cmd: List[str], cwd: Path) -> subprocess.CompletedProcess:
    """Run a command and capture stdout/stderr."""
    try:
        result = subprocess.run(
            cmd,
            cwd=str(cwd),
            capture_output=True,
            text=True,
            check=False,
        )
    except FileNotFoundError as exc:  # pragma: no cover - defensive
        raise TestFailure(f"Command not found: {cmd[0]}") from exc
    if result.returncode != 0:
        raise TestFailure(
            textwrap.dedent(
                f"""\
                Command failed: {' '.join(cmd)}
                cwd: {cwd}
                return code: {result.returncode}
                stdout:
                {result.stdout}
                stderr:
                {result.stderr}
                """
            ).rstrip()
        )
    return result


def compare_float(label: str, expected: float, actual: float, tolerance: float = 1e-3) -> None:
    if abs(expected - actual) > tolerance:
        raise TestFailure(f"{label} mismatch: expected {expected}, got {actual}")


def count_lines(path: Path) -> int:
    with path.open("r") as handle:
        return sum(1 for _ in handle)


def md5sum(path: Path) -> str:
    return hashlib.md5(path.read_bytes()).hexdigest()

def md5_hetatm(path: Path) -> str:
    digest = hashlib.md5()
    with path.open("r") as handle:
        for line in handle:
            if line.startswith("HETATM"):
                digest.update(line.encode("utf-8"))
    return digest.hexdigest()

def count_hetatm_lines(path: Path) -> int:
    with path.open("r") as handle:
        return sum(1 for line in handle if line.startswith("HETATM"))


def sanitized_md5(path: Path) -> str:
    digest = hashlib.md5()
    with path.open("r") as handle:
        for line in handle:
            if line.startswith("REMARK"):
                continue
            digest.update(line.encode("utf-8"))
    return digest.hexdigest()


def extract_summary(output: str) -> Dict[str, float]:
    summary = None
    for line in output.splitlines():
        if "probe,grid,volume" in line:
            summary = line.strip()
    if not summary:
        raise TestFailure("Unable to locate summary line in program output.")
    tokens = summary.split()
    if len(tokens) < 5:
        raise TestFailure(f"Unexpected summary format: {summary}")
    return {
        "probe": float(tokens[0]),
        "grid": float(tokens[1]),
        "volume": float(tokens[2]),
        "surface": float(tokens[3]),
        "atoms": float(tokens[4]),
    }


def resolve_path(base: Path, value: str) -> Path:
    path = Path(value)
    if not path.is_absolute():
        return (base / path).resolve()
    return path


def ensure_dir(path: Path) -> None:
    path.mkdir(parents=True, exist_ok=True)


def copy_if_exists(src: Path, dest: Path) -> bool:
    if src.exists():
        ensure_dir(dest.parent)
        dest.write_bytes(src.read_bytes())
        return True
    return False


def download_pdb(pdb_id: str, dest: Path) -> None:
    if dest.exists():
        return
    ensure_dir(dest.parent)

    fallback_plain_candidates = [
        TEST_DIR / f"{pdb_id}.pdb",
        TEST_DIR / "volume_results" / pdb_id / f"{pdb_id}.pdb",
    ]
    for candidate in fallback_plain_candidates:
        if copy_if_exists(candidate, dest):
            return

    gz_dest = dest.with_suffix(".pdb.gz")
    if not gz_dest.exists():
        fallback_gz_candidates = [
            TEST_DIR / f"{pdb_id}.pdb.gz",
            TEST_DIR / "volume_results" / pdb_id / f"{pdb_id}.pdb.gz",
        ]
        copied = False
        for candidate in fallback_gz_candidates:
            if copy_if_exists(candidate, gz_dest):
                copied = True
                break
        if not copied:
            url = f"https://files.rcsb.org/download/{pdb_id}.pdb.gz"
            try:
                urllib.request.urlretrieve(url, gz_dest)
            except urllib.error.URLError as exc:
                raise TestFailure(f"Unable to download {pdb_id} from RCSB: {exc}") from exc

    with gzip.open(gz_dest, "rb") as stream, dest.open("wb") as out:
        shutil.copyfileobj(stream, out)


def convert_xyzr(pdb: Path, output: Path, filters: List[str], overwrite: bool = False) -> None:
    if output.exists():
        if not overwrite:
            return
        output.unlink()
    ensure_dir(output.parent)
    converter = BIN_DIR / "pdb_to_xyzr.exe"
    cmd = [str(converter), *filters, "-i", str(pdb)]
    result = run(cmd, cwd=output.parent)
    output.write_text(result.stdout)


def handle_prerequisite(action: Dict[str, Any], workdir: Path) -> None:
    kind = action.get("action")
    if kind == "download_pdb":
        pdb_id = action["pdb_id"]
        dest = resolve_path(workdir, action.get("dest", f"{pdb_id}.pdb"))
        download_pdb(pdb_id, dest)
    elif kind == "convert_xyzr":
        pdb_path = resolve_path(workdir, action["pdb"])
        output = resolve_path(workdir, action["output"])
        filters = action.get("filters", [])
        overwrite = action.get("overwrite", False)
        convert_xyzr(pdb_path, output, filters, overwrite=overwrite)
    elif kind == "ensure_dir":
        ensure_dir(resolve_path(workdir, action.get("path", ".")))
    elif kind == "remove":
        target = resolve_path(workdir, action["path"])
        if target.exists():
            target.unlink()
    else:
        raise TestFailure(f"Unknown prerequisite action: {kind}")


def run_test(test: Dict[str, Any]) -> Tuple[float, int, int]:
    name = test["name"]
    rel_workdir = test.get("workdir", ".")
    workdir = (TEST_DIR / rel_workdir).resolve()
    ensure_dir(workdir)

    for prereq in test.get("prerequisites", []):
        handle_prerequisite(prereq, workdir)

    program = test.get("program")
    import shlex

    if program:
        program_path = BIN_DIR / program
        if not program_path.exists():
            raise TestFailure(f"{name}: program {program_path} not found. Build the binaries first.")
        command = [str(program_path)]
        args_entries = test.get("args", [])
        for entry in args_entries:
            command.extend(shlex.split(str(entry)))
    else:
        command_entries = test.get("command")
        if not isinstance(command_entries, list):
            raise TestFailure(f"Test {name}: command must be a list.")
        command = []
        for entry in command_entries:
            if isinstance(entry, list):
                command.extend(entry)
            else:
                command.extend(shlex.split(str(entry)))

    start = time.perf_counter()
    result = run(command, cwd=workdir)
    duration = time.perf_counter() - start
    expect = test.get("expect", {})
    checks: List[Tuple[str, bool, str]] = []

    def record_check(label: str, passed: bool, detail: str = "") -> None:
        checks.append((label, passed, detail))

    if "summary" in expect:
        summary_expect = expect["summary"]
        tol = summary_expect.get("tolerance", 1e-3)
        try:
            summary = extract_summary(result.stdout)
        except TestFailure as exc:
            record_check("summary", False, str(exc))
        else:
            for key in ("volume", "surface", "atoms"):
                if key in summary_expect:
                    expected_value = summary_expect[key]
                    actual_value = summary[key]
                    try:
                        compare_float(key, expected_value, actual_value, tol)
                    except TestFailure as exc:
                        record_check(key, False, str(exc))
                    else:
                        record_check(key, True)

    pdb_expect = expect.get("pdb")
    if pdb_expect:
        pdb_path = resolve_path(workdir, pdb_expect["path"])
        if not pdb_path.exists():
            record_check("pdb_exists", False, f"expected output {pdb_path} is missing.")
        else:
            record_check("pdb_exists", True)
            if "lines" in pdb_expect:
                lines = count_lines(pdb_path)
                if lines != pdb_expect["lines"]:
                    record_check("lines", False, f"expected {pdb_expect['lines']} lines, found {lines}")
                else:
                    record_check("lines", True)
            if "md5" in pdb_expect:
                digest = md5sum(pdb_path)
                if digest != pdb_expect["md5"]:
                    record_check("md5", False, f"expected {pdb_expect['md5']}, got {digest}")
                else:
                    record_check("md5", True)
            if "hetatm_lines" in pdb_expect:
                hetatm_lines = count_hetatm_lines(pdb_path)
                if hetatm_lines != pdb_expect["hetatm_lines"]:
                    record_check(
                        "hetatm_lines",
                        False,
                        f"expected {pdb_expect['hetatm_lines']} HETATM lines, found {hetatm_lines}",
                    )
                else:
                    record_check("hetatm_lines", True)
            if "hetatm_md5" in pdb_expect:
                digest = md5_hetatm(pdb_path)
                if digest != pdb_expect["hetatm_md5"]:
                    record_check(
                        "hetatm_md5",
                        False,
                        f"expected {pdb_expect['hetatm_md5']}, got {digest}",
                    )
                else:
                    record_check("hetatm_md5", True)
            if "md5_sanitized" in pdb_expect:
                digest = sanitized_md5(pdb_path)
                if digest != pdb_expect["md5_sanitized"]:
                    record_check("md5_sanitized", False, f"expected {pdb_expect['md5_sanitized']}, got {digest}")
                else:
                    record_check("md5_sanitized", True)

    if expect.get("stdout_contains"):
        needle = expect["stdout_contains"]
        combined = result.stdout + result.stderr
        if needle not in combined:
            record_check("stdout_contains", False, f"stdout missing expected text: {needle}")
        else:
            record_check("stdout_contains", True)

    total = len(checks)
    passed = sum(1 for _, ok, _ in checks if ok)
    if passed != total:
        failed_details = "; ".join(
            detail if detail else label for label, ok, detail in checks if not ok
        )
        raise TestFailureWithChecks(f"{name}: {failed_details}", checks, duration)

    return duration, passed, total


def load_tests(path: Path) -> List[Dict[str, Any]]:
    data = yaml.safe_load(path.read_text())
    return data.get("tests", [])


def resolve_config_path(config_value: str) -> Path:
    candidate = Path(config_value)
    if candidate.is_absolute():
        return candidate
    return (Path.cwd() / candidate).resolve()


def find_default_config() -> Path:
    candidates = [
        Path.cwd() / "test_suite.yml",
        Path(__file__).resolve().parent / "test_suite.yml",
        REPO_ROOT / "tests" / "test_suite.yml",
    ]
    for candidate in candidates:
        if candidate.exists():
            return candidate
    return candidates[0].resolve()

def collect_required_binaries(tests: List[Dict[str, Any]]) -> List[Path]:
    required = set()
    for test in tests:
        program = test.get("program")
        if program:
            required.add(BIN_DIR / program)
        for prereq in test.get("prerequisites", []):
            if prereq.get("action") == "convert_xyzr":
                required.add(BIN_DIR / "pdb_to_xyzr.exe")
    return sorted(required)


def find_missing_binaries(required: List[Path]) -> List[Path]:
    return [path for path in required if not path.exists()]


def build_binaries(target: str = "all") -> None:
    result = subprocess.run(
        ["make", target],
        cwd=str(SRC_DIR),
        check=False,
    )
    if result.returncode != 0:
        raise TestFailure(f"`make {target}` failed with exit code {result.returncode}")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", default="test_suite.yml", help="YAML file describing the tests.")
    parser.add_argument("--list", action="store_true", help="List tests without running them.")
    parser.add_argument(
        "--build-missing",
        choices=("all", "none"),
        default="all",
        help="Build missing binaries before running tests (default: all).",
    )
    args = parser.parse_args()

    if args.config == "test_suite.yml":
        config_path = find_default_config()
    else:
        config_path = resolve_config_path(args.config)
    if not config_path.exists():
        error(f"Test config not found: {config_path}")
        sys.exit(1)
    tests = load_tests(config_path)
    if not tests:
        error("No tests found.")
        sys.exit(1)

    if args.list:
        for test in tests:
            print(test["name"])
        return

    required = collect_required_binaries(tests)
    missing = find_missing_binaries(required)
    if missing:
        if args.build_missing == "all":
            info("Missing binaries detected; running `make all`.")
            try:
                build_binaries("all")
            except TestFailure as exc:
                error(str(exc))
                sys.exit(1)
            missing = find_missing_binaries(required)
        if missing:
            error("Missing binaries:")
            for path in missing:
                error(f"- {path}")
            sys.exit(1)

    failures = []
    durations = []
    for test in tests:
        name = test["name"]
        info(f"▶ Running {name} ...")
        try:
            elapsed, passed, total = run_test(test)
            durations.append((name, elapsed))
        except TestFailureWithChecks as exc:
            error(f"✖ {name}: {exc}")
            for label, ok, detail in exc.checks:
                if ok:
                    success(f"  ✔ {label}")
                else:
                    error(f"  ✖ {label}: {detail if detail else 'failed'}")
            error(f"✖ {name} [{exc.duration:.3f}s] ({exc.passed}/{exc.total} checks)")
            failures.append(name)
        except TestFailure as exc:
            error(f"✖ {name}: {exc}")
            failures.append(name)
        else:
            if total:
                success(f"✔ {name} [{elapsed:.3f}s] ({passed}/{total} checks)")
            else:
                success(f"✔ {name} [{elapsed:.3f}s]")

    if failures:
        error(f"{len(failures)} test(s) failed.")
        sys.exit(1)

    success("All YAML-defined tests passed.")


if __name__ == "__main__":
    main()
