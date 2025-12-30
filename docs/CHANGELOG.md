# Changelog

Chronological record of user-facing and maintenance changes.

## 2025-12-30
OpenAI Codex
- Converted `docs/CHANGELOG.md` to Markdown headings and bullet lists.
- Reformatted `docs/INSTALL.md`, `docs/QUICKSTART.md`, `docs/TODO.md`, and
  `docs/RELEASE_HISTORY.md` into Markdown-first layouts.
- Collapsed `VERSION` to a single `26.01b6` entry and merged prior history into
  `docs/RELEASE_HISTORY.md`.
- Linked all `docs/*.md` files from `README.md` and refreshed the Quickstart and
  utilities references.
- Added `docs/FILE_STRUCTURE.md` and `docs/CODE_ARCHITECTURE.md` and linked them
  from `README.md`.
- Updated documentation references from `test/` to `tests/` after the directory
  move.
- Added short descriptions to the `README.md` documentation links.
- Moved shared C++ helpers into `src/lib/` and updated build paths and docs.
- Ensured `src/Makefile` creates `bin/` before building legacy objects.
- Fixed Python lint issues in `python/` and `tests/` (unused imports, regex escapes,
  and local module references).
- Updated `tests/test_suite.py` to locate `test_suite.yml` from CWD, script
  directory, or `REPO_ROOT/tests/` when no config is provided.
- Added a preflight in `tests/test_suite.py` to build or fail fast when required
  binaries are missing.
- Updated `tests/test_suite.yml` summary expectations to match the post-f0becee
  rounding output from `printVol()` and `printVolCout()`.
- Standardized CLI input handling across C++ executables (except `Volume-legacy.exe`)
  to use shared XYZR filter flags and PDB-aware loading.
- Renamed in-memory XYZR buffer variables to `xyzr_buffer*` for clarity.
- Shortened `README.md` by pointing detailed sections to the docs directory.
- Renamed `src/customvolume.cpp` to `src/rna_protein_volume.cpp` for clarity.
- Updated `tests/test_suite.yml` to run executables directly on PDB input with
  filter flags and refreshed MD5 expectations accordingly.
- Adjusted `tests/test_volume.sh` expected volume to the new rounding output.
- Clarified `tests/test_volume_classic.sh` messaging when `Volume-1.0.exe` is
  unavailable.
- Updated `src/Makefile` to place shared object files under `src/lib/`.
- Renamed C++ entry point sources to snake_case and updated build references.
- Renamed `Custom.exe` to `ProteinRNAVolume.exe`.
- Standardized `pdb_to_xyzr.exe` CLI handling via `ArgumentParser`, retaining
  positional input support while documenting `-i` usage.
- Improved `tests/test_suite.py` reporting to summarize passed checks on both
  successes and failures.
- Removed the input-provenance remark line from PDB outputs to keep headers
  deterministic for test comparisons.
- Expanded `tests/test_suite.py` failure output to list per-check pass/fail lines
  in color with a final timing summary.
- Linked `pdb_to_xyzr.exe` against `argument_helper` after CLI standardization.
- Updated test fixtures and expectations after removing the PDB provenance remark
  (line counts and MD5s in `tests/test_suite.yml`, `tests/test_volume.sh`).
- Switched PDB output checks in `tests/test_suite.yml` to compare HETATM-only
  counts and MD5s alongside total line counts.
- Updated `tests/test_suite.py` sanitized MD5 helper to strip all `REMARK` lines.
- Added deterministic PDB header remarks for creation time and command line, and
  refreshed line-count expectations accordingly.
- Reformatted PDB CLI remarks to emit one sorted argument per REMARK line.
- Moved command-line capture helpers into `src/lib/argument_helper.*`.
- Removed the `volume_reference` build target; `Volume-1.0.exe` must be supplied
  externally.
- Added `tests/clean_up_test_files.sh` to remove cached test artifacts.
- Set `VERSION` to `26.01b2`.
- Renamed modern `finalGridDims()` to `initGridState()` with a compatibility wrapper.
- Added shared XYZR CLI helpers in `src/lib/xyzr_cli_helpers.*`.
- Renamed `src/volume_original.cpp` to `src/volume-legacy.cpp` and marked legacy
  sources as no-edit in `AGENTS.md`.
- Updated `tests/test_volume.sh` to validate HETATM-only counts and MD5s instead
  of full-file hashes.
- Renamed YAML test entries in `tests/test_suite.yml` to snake_case identifiers
  aligned with the C++ source filenames.
- Refactored pilot executables (`volume.cpp`, `volume-fill_cavities.cpp`,
  `two_volumes-fill_cavities.cpp`) to use shared XYZR/grid helpers and validated
  outputs via the test suite.
- Rolled the shared XYZR/grid helper refactor out to the remaining modern
  executables.
- Added common CLI settings helpers for filter/output options and centralized
  conversion option mapping.
- Closed Phase 5 cleanup (kept `finalGridDims()` wrapper).
- Added a unified `--debug` flag to report filters, inputs/outputs, grid state,
  and timing.
- Centralized common output-file writes into `write_output_files()` for modern
  tools using `OutputSettings`.
- Added an `OutputSettings.use_small_mrc` toggle so tools can select
  `writeSmallMRCFile` without per-call flags.
- Added `report_grid_metrics()` helper for consistent grid summary output.
- Added `make_zeroed_grid()` helper to combine allocation and zeroing.
- Updated C++ includes to use renamed `.hpp` headers and refreshed build refs.
- Trimmed unused global extern declarations across modern executables.
- Modernized `rna_protein_volume.cpp` grid allocations to use `make_zeroed_grid()`.
- Dropped the planned `--debug-limits` hook from the refactor plan.

## 2025-11-29
Neil Voss <vossman77@yahoo.com>
- Added pdb_io library + XYZRBuffer helpers so all tools can share in-memory XYZR data.
- Updated `Volume.exe` to auto-detect PDB/mmCIF/PDBML/XYZR inputs and expose
  `pdb_to_xyzr` filtering flags.
- Extended `README.md`, `INSTALL`, and `QUICKSTART.txt` to describe direct PDB usage
  and new CLI options.
- Expanded `VERSION` and `NEWS` for the 2.0.0-beta5 release and added YAML coverage
  for PDB input.

## 2025-11-28
Neil Voss <vossman77@yahoo.com>
- Added safety padding to `assignLimits()` (modern + legacy) to avoid OOB fills.
- Introduced `test_volumes.py` harness, YAML-driven `test_suite.py`, and
  `volume_reference` make target.
- Removed redundant `xyzr/pdb_to_xyzr_legacy.py` + `xyzr/sort_xyzr.py` in favor of
  the native converter.
- Expanded `README.md`, `NEWS`, and `VERSION` to describe the new workflow.

## 2025-11-28
Neil Voss <vossman77@yahoo.com>
- Introduced shared `argument_helper` (C++17) and migrated `Volume.exe` to it.
- Set the toolchain default to C++17 for upcoming CLI modernization.

## 2025-11-28
Neil Voss <vossman77@yahoo.com>
- Added native Python and C++ `pdb_to_xyzr` converters with fine-grained filters.
- Updated documentation/tests to rely on `./bin/pdb_to_xyzr` executables.

## 2024-11-14
Neil Voss <vossman77@yahoo.com>
- Comprehensive modernization/refactor in preparation for the 2.0 beta line.

## 2024-11-14
Neil Voss <vossman77@yahoo.com>
- Refactored some code for modern times.

## 2009-06-02
Neil Voss <vossman77@yahoo.com>
- Added ability to write to MRC files, thanks Craig.
- Rebuilt for distribution, v1.2.

## 2006-06-20
Neil Voss <vossman77@yahoo.com>
- Rebuilt for distribution, v1.1.

## 2005-01-27
Neil Voss <vossman77@yahoo.com>
- adjusted XMIN, YMIN, ZMIN to interget multiple of grid needed to better EZD file
  support.
- added testLimits() function for tracking down out-of-range calls.
- added zeroGrid(grid) function to zero out all points (cause of out-of-range calls).

## 2005-01-26
Neil Voss <vossman77@yahoo.com>
- added surface area calculation.
