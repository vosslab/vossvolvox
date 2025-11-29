# Repository Guidelines

## Project Structure & Module Organization
Source lives in `src/`, which builds the C++17 executables under `bin/` via the shared `argument_helper.*` and converter helpers. Legacy and Python converters are in `xyzr/`, while docs live in `README.md`, `QUICKSTART.txt`, `INSTALL`, and `NEWS`. Tests and harnesses sit in `test/`, notably `test_volume.sh` and `test_pdb_to_xyzr.sh`, which stage artifacts under `test/pdb_to_xyzr_results/`.

## Build, Test, and Development Commands
- `cd src && make` – full toolchain build; use `make <target>` (e.g., `make vol`, `make pdb_to_xyzr`) for incremental work.
- `./bin/<tool>.exe --help` – every executable shares the global CLI; confirm option regressions immediately.
- `./test/test_volume.sh` – downloads 2LYZ when needed, regenerates XYZR, runs `Volume.exe`, and checks MD5s.
- `./test/test_pdb_to_xyzr.sh 1A01` – compares cpp/python/sh converters for a given PDB ID, reporting timings, line counts, MD5s, and diffs.
- `clean.sh` – removes build artifacts prior to benchmarks or packaging.

## Coding Style & Naming Conventions
C++ code targets C++17 with four-space indentation, brace-on-newline functions, and Standard Library containers. Favor `std::string_view`, `std::optional`, and range-based loops; new binaries should extend `ArgumentParser` in `argument_helper.*` and reuse its built-in XYZR helper functions. Python adopts PEP 8 naming, argparse, and pathlib-based I/O. Filenames use lowercase with underscores (`pdb_to_xyzr.py`), while executables retain the historical `.exe` suffix.

## Testing Guidelines
Add or extend shell tests under `test/`. Prefer deterministic fixtures by caching downloads in `test/pdb_to_xyzr_results/<PDBID>/`, and report MD5 checksums whenever converters are touched. When enhancing a C++ program, at minimum run `make <target>` plus the closest shell test; document any missing coverage in NEWS. Name new tests after the tool (`test_<tool>.sh`) and keep runs idempotent (skip downloads if files exist).

## Commit & Pull Request Guidelines
Follow the observed Conventional Commit style (`feat:`, `chore:`, `meta:`). Each commit should touch a coherent set of files, e.g., `git add src/pdb_to_xyzr.cpp xyzr/pdb_to_xyzr.py && git commit -m "feat: add residue-aware filtering"`. Reviews should include a summary, testing commands/output, linked issues, and sample CLI snippets when help text changes. Update `NEWS` and `VERSION` whenever user-facing behavior shifts, and call out Gemmi or atmtypenumbers adjustments explicitly.

## Security & Configuration Tips
`pdb_to_xyzr` auto-detects Gemmi headers; keep the dependency patched (`pip install --upgrade gemmi`) and rebuild. Avoid hardcoding atmtypenumbers paths—include them via `atmtypenumbers_data.h` to keep builds reproducible. When adding new options, ensure `ArgumentParser` exposes quiet-aware banners so `-q/--quiet` suppresses identifying text across binaries.
