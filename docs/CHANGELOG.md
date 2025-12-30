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
