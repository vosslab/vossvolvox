# Repository Guidelines

## Project Structure & Module Organization
Source lives in `src/`, which builds the C++17 executables under `bin/` via the shared `argument_helper.*` and converter helpers. Legacy and Python converters are in `xyzr/`, while docs live in `README.md` and `docs/`. Tests and harnesses sit in `tests/`, notably `test_volume.sh` and `test_pdb_to_xyzr.sh`, which stage artifacts under `tests/pdb_to_xyzr_results/`.

## Build, Test, and Development Commands
- `cd src && make` - full toolchain build; use `make <target>` (e.g., `make vol`, `make pdb_to_xyzr`) for incremental work.
- `./bin/<tool>.exe --help` - every executable shares the global CLI; confirm option regressions immediately.
- `./tests/test_volume.sh` - downloads 2LYZ when needed, regenerates XYZR, runs `Volume.exe`, and checks MD5s.
- `./tests/test_pdb_to_xyzr.sh 1A01` - compares cpp/python/sh converters for a given PDB ID, reporting timings, line counts, MD5s, and diffs.
- `clean.sh` - removes build artifacts prior to benchmarks or packaging.

## Coding Style & Naming Conventions
C++ code targets C++17 with four-space indentation, brace-on-newline functions, and Standard Library containers. Favor `std::string_view`, `std::optional`, and range-based loops; new binaries should extend `ArgumentParser` in `argument_helper.*` and reuse its built-in XYZR helper functions. Python adopts PEP 8 naming, argparse, and pathlib-based I/O. Source files use snake_case (`volume-fill_cavities.cpp`), while executables retain historical CamelCase names with the `.exe` suffix (for example `VolumeNoCav.exe`).

Legacy code is frozen: do not edit `src/lib/utils-main-legacy.cpp` or `src/volume-legacy.cpp`.

## Testing Guidelines
Add or extend shell tests under `tests/`. Prefer deterministic fixtures by caching downloads in `tests/pdb_to_xyzr_results/<PDBID>/`, and report MD5 checksums whenever converters are touched. When enhancing a C++ program, at minimum run `make <target>` plus the closest shell test; document any missing coverage in `docs/RELEASE_HISTORY.md`. Name new tests after the tool (`test_<tool>.sh`) and keep runs idempotent (skip downloads if files exist).

## Commit & Pull Request Guidelines
Follow the observed Conventional Commit style (`feat:`, `chore:`, `meta:`). Each commit should touch a coherent set of files, e.g., `git add src/pdb_to_xyzr.cpp xyzr/pdb_to_xyzr.py && git commit -m "feat: add residue-aware filtering"`. Reviews should include a summary, testing commands/output, linked issues, and sample CLI snippets when help text changes. Update `docs/RELEASE_HISTORY.md` and `VERSION` whenever user-facing behavior shifts, and call out Gemmi or atmtypenumbers adjustments explicitly.

## Security & Configuration Tips
`pdb_to_xyzr` auto-detects Gemmi headers; keep the dependency patched (`pip install --upgrade gemmi`) and rebuild. Avoid hardcoding atmtypenumbers paths-include them via `atmtypenumbers_data.h` to keep builds reproducible. When adding new options, ensure `ArgumentParser` exposes quiet-aware banners so `-q/--quiet` suppresses identifying text across binaries.
See Python coding style in docs/PYTHON_STYLE.md.
See Markdown style in docs/MARKDOWN_STYLE.md.
See repo style in docs/REPO_STYLE.md.
When making edits, document them in docs/CHANGELOG.md.
Agents may run programs in the tests folder, including smoke tests and pyflakes/mypy runner scripts.
When in doubt, implement the changes the user asked for rather than waiting for a response; the user is not the best reader and will likely miss your request and then be confused why it was not implemented or fixed.
When changing code always run tests, documentation does not require tests.

## Environment
Codex must run Python using `/opt/homebrew/opt/python@3.12/bin/python3.12` (use Python 3.12 only).
On this user's macOS (Homebrew Python 3.12), Python modules are installed to `/opt/homebrew/lib/python3.12/site-packages/`.
