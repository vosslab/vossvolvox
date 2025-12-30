#!/usr/bin/env python3
"""Run both modern and legacy Volume binaries on the same structures.

This script reproduces the full pipeline: download (or reuse) a PDB, convert it
to XYZR using both the modern converter (with --exclude-ions/--exclude-water)
and the legacy converter, and then run Volume.exe and Volume-legacy.exe on each
result.  A consolidated report highlights differences in volume, surface area,
line counts, and sanitized MD5 hashes so regressions are easy to spot.
"""

from __future__ import annotations

import argparse
import gzip
import hashlib
import shutil
import subprocess
import sys
import textwrap
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Tuple
from urllib.request import urlretrieve


TEST_DIR = Path(__file__).resolve().parent
REPO_ROOT = TEST_DIR.parent
BIN_DIR = REPO_ROOT / "bin"
SRC_DIR = REPO_ROOT / "src"
XYZR_DIR = REPO_ROOT / "xyzr"
RESULTS_DIR = TEST_DIR / "volume_results"
REFERENCE_BINARY_NAME = "Volume-1.0.exe"


def run(cmd: List[str], *, cwd: Path | None = None, capture_output: bool = True) -> subprocess.CompletedProcess:
    """Execute a command and raise on failure."""
    result = subprocess.run(
        cmd,
        cwd=str(cwd) if cwd else None,
        capture_output=capture_output,
        text=True,
        check=False,
    )
    if result.returncode != 0:
        stdout = result.stdout or ""
        stderr = result.stderr or ""
        msg = textwrap.dedent(
            f"""\
            Command failed: {' '.join(cmd)}
            Return code: {result.returncode}
            STDOUT:
            {stdout}
            STDERR:
            {stderr}
            """
        )
        raise RuntimeError(msg)
    return result


def ensure_binary(path: Path, target: str) -> None:
    """Build the requested make target if the binary does not exist."""
    if path.exists():
        return
    print(f"Building {target} -> {path}", flush=True)
    run(["make", target], cwd=SRC_DIR, capture_output=False)
    if not path.exists():
        raise FileNotFoundError(f"Expected binary {path} after `make {target}`")


def ensure_dir(path: Path) -> None:
    path.mkdir(parents=True, exist_ok=True)


def download_pdb(pdb_id: str, staging_dir: Path) -> Path:
    """Download (or reuse) a gzipped PDB from RCSB and return the decompressed path."""
    pdb_path = staging_dir / f"{pdb_id}.pdb"
    if pdb_path.exists():
        print(f"Found existing {pdb_path}; skipping download.")
        return pdb_path

    # Reuse previously downloaded files from test/ if available.
    legacy_pdb = TEST_DIR / f"{pdb_id}.pdb"
    legacy_gz = legacy_pdb.with_suffix(".pdb.gz")

    if legacy_pdb.exists():
        print(f"Copying existing {legacy_pdb} -> {pdb_path}")
        shutil.copy2(legacy_pdb, pdb_path)
        return pdb_path

    gz_path = staging_dir / f"{pdb_id}.pdb.gz"
    if gz_path.exists():
        print(f"Found existing {gz_path.name}; reusing local copy.")
    elif legacy_gz.exists():
        print(f"Copying existing {legacy_gz} -> {gz_path}")
        shutil.copy2(legacy_gz, gz_path)
    else:
        url = f"https://files.rcsb.org/download/{pdb_id}.pdb.gz"
        print(f"Downloading {pdb_id} from {url} ...")
        urlretrieve(url, gz_path)

    print(f"Extracting {gz_path.name} -> {pdb_path.name} ...")
    with gzip.open(gz_path, "rb") as src, open(pdb_path, "wb") as dst:
        shutil.copyfileobj(src, dst)
    return pdb_path


def write_atom_only(pdb_path: Path, dest: Path) -> None:
    """Write only ATOM records from pdb_path into dest."""
    with pdb_path.open("r") as src, dest.open("w") as dst:
        for line in src:
            if line.startswith("ATOM  "):
                dst.write(line)


def convert_modern(pdb_path: Path, output_xyzr: Path, converter: Path) -> None:
    """Run the modern converter with the default filters."""
    cmd = [
        str(converter),
        "--exclude-ions",
        "--exclude-water",
        "-i",
        str(pdb_path),
    ]
    print(f"Converting {pdb_path.name} -> {output_xyzr.name} using modern converter...")
    with output_xyzr.open("w") as out_handle:
        result = run(cmd, cwd=TEST_DIR)
        out_handle.write(result.stdout)


def convert_legacy(atom_only_path: Path, output_xyzr: Path, legacy_script: Path) -> None:
    """Run the legacy Python converter on the ATOM-only PDB."""
    cmd = ["python3", str(legacy_script), str(atom_only_path)]
    print(f"Converting {atom_only_path.name} -> {output_xyzr.name} using legacy converter...")
    with output_xyzr.open("w") as out_handle:
        result = run(cmd, cwd=TEST_DIR)
        out_handle.write(result.stdout)


def sanitize_md5(pdb_path: Path) -> Tuple[str, str]:
    """Return (raw_md5, sanitized_md5_without_remark_date)."""
    raw_md5 = hashlib.md5(pdb_path.read_bytes()).hexdigest()
    sanitized = hashlib.md5()
    with pdb_path.open("r") as handle:
        for line in handle:
            if line.startswith("REMARK Date"):
                continue
            sanitized.update(line.encode("utf-8"))
    return raw_md5, sanitized.hexdigest()


@dataclass
class VolumeResult:
    binary: str
    xyzr_label: str
    xyzr_path: Path
    output_pdb: Path
    probe: float
    grid: float
    volume: float
    surface: float
    atoms: int
    log: str
    line_count: int
    md5_raw: str
    md5_sanitized: str


def parse_summary(log_text: str) -> Tuple[float, float, float, float, int, str]:
    """Extract probe, grid, volume, surface, atoms, and input file from the Volume log."""
    target = None
    for line in log_text.splitlines():
        if "probe,grid,volume" in line:
            target = line.strip()
    if not target:
        raise ValueError("Volume output missing summary line.")
    tokens = target.split()
    if len(tokens) < 6:
        raise ValueError(f"Unexpected summary format: {target}")
    probe = float(tokens[0])
    grid = float(tokens[1])
    volume = float(tokens[2])
    surface = float(tokens[3])
    atoms = int(tokens[4])
    input_file = tokens[5]
    return probe, grid, volume, surface, atoms, input_file


def run_volume(binary: Path, xyzr_path: Path, output_dir: Path, tag: str, probe: float, grid: float) -> VolumeResult:
    """Run a Volume binary on the specified XYZR file."""
    output_pdb = output_dir / f"{xyzr_path.stem}-{tag}.pdb"
    output_mrc = output_dir / f"{xyzr_path.stem}-{tag}.mrc"
    cmd = [
        str(binary),
        "-i",
        str(xyzr_path),
        "-p",
        f"{probe}",
        "-g",
        f"{grid}",
        "-o",
        str(output_pdb),
        "-m",
        str(output_mrc),
    ]
    print(f"Running {binary.name} on {xyzr_path.name} -> {output_pdb.name}")
    result = run(cmd, cwd=TEST_DIR)
    probe, grid, volume, surface, atoms, _ = parse_summary(result.stdout)
    line_count = sum(1 for _ in output_pdb.open("r"))
    md5_raw, md5_sanitized = sanitize_md5(output_pdb)
    return VolumeResult(
        binary=binary.name,
        xyzr_label=xyzr_path.stem,
        xyzr_path=xyzr_path,
        output_pdb=output_pdb,
        probe=probe,
        grid=grid,
        volume=volume,
        surface=surface,
        atoms=atoms,
        log=result.stdout,
        line_count=line_count,
        md5_raw=md5_raw,
        md5_sanitized=md5_sanitized,
    )


def format_float(value: float) -> str:
    return f"{value:10.3f}"


def report(results: Dict[str, List[VolumeResult]]) -> None:
    """Print a comparison summary for each XYZR input."""
    for label, runs in results.items():
        print()
        print(f"=== Input: {label} ({runs[0].xyzr_path.name}) ===")
        header = f"{'Binary':12} {'Volume (A^3)':>14} {'Surface (A^2)':>14} {'Atoms':>8} {'Lines':>8} {'MD5 (sanitized)':>34}"
        print(header)
        print("-" * len(header))
        for res in runs:
            print(
                f"{res.binary:12} {format_float(res.volume)} {format_float(res.surface)} "
                f"{res.atoms:8d} {res.line_count:8d} {res.md5_sanitized}"
            )
        reference = next((r for r in runs if r.binary == REFERENCE_BINARY_NAME), runs[0])
        print("Differences vs reference:")
        for res in runs:
            if res is reference:
                continue
            dv = res.volume - reference.volume
            ds = res.surface - reference.surface
            da = res.atoms - reference.atoms
            dl = res.line_count - reference.line_count
            md5_match = res.md5_sanitized == reference.md5_sanitized
            print(
                f"  {res.binary:12}: ΔVolume={dv:+.3f}  ΔSurface={ds:+.3f}  ΔAtoms={da:+d}  ΔLines={dl:+d}  MD5 match={md5_match}"
            )


def main(argv: List[str]) -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pdb-id", default="2LYZ", help="RCSB PDB identifier (default: 2LYZ)")
    parser.add_argument("--probe-radius", type=float, default=2.1, help="Probe radius in Angstroms (default: 2.1)")
    parser.add_argument("--grid-spacing", type=float, default=0.9, help="Grid spacing in Angstroms (default: 0.9)")
    parser.add_argument("--skip-build", action="store_true", help="Assume binaries already exist; skip make targets.")
    parser.add_argument("--keep-intermediates", action="store_true", help="Do not delete generated intermediate files.")
    args = parser.parse_args(argv)

    pdb_id = args.pdb_id.upper()

    modern_bin = BIN_DIR / "Volume.exe"
    legacy_bin = BIN_DIR / "Volume-legacy.exe"
    reference_bin = BIN_DIR / "Volume-1.0.exe"
    modern_converter = BIN_DIR / "pdb_to_xyzr.exe"

    if not args.skip_build:
        ensure_binary(modern_bin, "vol")
        ensure_binary(legacy_bin, "volume_original")
        ensure_binary(reference_bin, "volume_reference")
        ensure_binary(modern_converter, "pdbxyzr")
    else:
        for binary, target in [
            (modern_bin, "vol"),
            (legacy_bin, "volume_original"),
            (reference_bin, "volume_reference"),
            (modern_converter, "pdbxyzr"),
        ]:
            if not binary.exists():
                raise FileNotFoundError(f"Missing {binary}. Re-run without --skip-build or build via `make {target}`.")

    results_dir = RESULTS_DIR / pdb_id
    ensure_dir(results_dir)

    pdb_path = download_pdb(pdb_id, results_dir)
    atom_only_path = results_dir / f"{pdb_id}-atom-only.pdb"
    write_atom_only(pdb_path, atom_only_path)

    xyzr_path = results_dir / f"{pdb_id}-modern.xyzr"
    convert_modern(pdb_path, xyzr_path, modern_converter)

    scenarios: Dict[str, Path] = {
        "modern_xyzr": xyzr_path,
    }
    results: Dict[str, List[VolumeResult]] = {}

    for label, xyzr in scenarios.items():
        runs = [
            run_volume(reference_bin, xyzr, results_dir, f"{label}-reference", args.probe_radius, args.grid_spacing),
            run_volume(modern_bin, xyzr, results_dir, f"{label}-fast", args.probe_radius, args.grid_spacing),
            run_volume(legacy_bin, xyzr, results_dir, f"{label}-legacy", args.probe_radius, args.grid_spacing),
        ]
        results[label] = runs

    report(results)

    if not args.keep_intermediates:
        for path in [atom_only_path, xyzr_path]:
            path.unlink(missing_ok=True)


if __name__ == "__main__":
    main(sys.argv[1:])
