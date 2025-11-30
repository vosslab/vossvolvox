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
import urllib.error
import urllib.request
from pathlib import Path
from typing import Any, Dict, List

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
TEST_DIR = REPO_ROOT / "test"
BIN_DIR = REPO_ROOT / "bin"

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


def sanitized_md5(path: Path) -> str:
    digest = hashlib.md5()
    with path.open("r") as handle:
        for line in handle:
            if line.startswith("REMARK Date"):
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


def convert_xyzr(pdb: Path, output: Path, filters: List[str]) -> None:
    if output.exists():
        return
    ensure_dir(output.parent)
    converter = BIN_DIR / "pdb_to_xyzr.exe"
    cmd = [str(converter), *filters, str(pdb)]
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
        convert_xyzr(pdb_path, output, filters)
    elif kind == "ensure_dir":
        ensure_dir(resolve_path(workdir, action.get("path", ".")))
    else:
        raise TestFailure(f"Unknown prerequisite action: {kind}")


def run_test(test: Dict[str, Any]) -> None:
    name = test["name"]
    rel_workdir = test.get("workdir", ".")
    workdir = (TEST_DIR / rel_workdir).resolve()
    ensure_dir(workdir)

    for prereq in test.get("prerequisites", []):
        handle_prerequisite(prereq, workdir)

    command_entries = test["command"]
    if not isinstance(command_entries, list):
        raise TestFailure(f"Test {name}: command must be a list.")

    import shlex

    command: List[str] = []
    for entry in command_entries:
        if isinstance(entry, list):
            command.extend(entry)
        else:
            command.extend(shlex.split(str(entry)))

    result = run(command, cwd=workdir)
    expect = test.get("expect", {})

    if "summary" in expect:
        summary = extract_summary(result.stdout)
        summary_expect = expect["summary"]
        tol = summary_expect.get("tolerance", 1e-3)
        for key in ("volume", "surface", "atoms"):
            if key in summary_expect:
                compare_float(f"{name}:{key}", summary_expect[key], summary[key], tol)

    pdb_expect = expect.get("pdb")
    if pdb_expect:
        pdb_path = resolve_path(workdir, pdb_expect["path"])
        if not pdb_path.exists():
            raise TestFailure(f"{name}: expected output {pdb_path} is missing.")
        if "lines" in pdb_expect:
            lines = count_lines(pdb_path)
            if lines != pdb_expect["lines"]:
                raise TestFailure(f"{name}: expected {pdb_expect['lines']} lines, found {lines}")
        if "md5" in pdb_expect:
            digest = md5sum(pdb_path)
            if digest != pdb_expect["md5"]:
                raise TestFailure(f"{name}: expected md5 {pdb_expect['md5']}, got {digest}")
        if "md5_sanitized" in pdb_expect:
            digest = sanitized_md5(pdb_path)
            if digest != pdb_expect["md5_sanitized"]:
                raise TestFailure(
                    f"{name}: expected sanitized md5 {pdb_expect['md5_sanitized']}, got {digest}"
                )

    if expect.get("stdout_contains"):
        needle = expect["stdout_contains"]
        combined = result.stdout + result.stderr
        if needle not in combined:
            raise TestFailure(f"{name}: stdout missing expected text: {needle}")


def load_tests(path: Path) -> List[Dict[str, Any]]:
    data = yaml.safe_load(path.read_text())
    return data.get("tests", [])


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", default="test_suite.yml", help="YAML file describing the tests.")
    parser.add_argument("--list", action="store_true", help="List tests without running them.")
    args = parser.parse_args()

    config_path = (TEST_DIR / args.config).resolve()
    tests = load_tests(config_path)
    if not tests:
        error("No tests found.")
        sys.exit(1)

    if args.list:
        for test in tests:
            print(test["name"])
        return

    failures = []
    for test in tests:
        name = test["name"]
        info(f"▶ Running {name} ...")
        try:
            run_test(test)
        except TestFailure as exc:
            error(f"✖ {name}: {exc}")
            failures.append(name)
        else:
            success(f"✔ {name}")

    if failures:
        error(f"{len(failures)} test(s) failed.")
        sys.exit(1)

    success("All YAML-defined tests passed.")


if __name__ == "__main__":
    main()
