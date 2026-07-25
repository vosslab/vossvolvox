# Release history

Chronological list of released versions and notable changes.

## v26.07 - 2026-07-25

### Highlights

- Modern C++17 tools accept PDB and XYZR input through a shared command-line
  interface; builds with Gemmi also accept mmCIF and PDBML.
- The native `pdb_to_xyzr.exe` converter supports reusable XYZR output and
  filters for ions, ligands, water, HETATM records, nucleic acids, amino acids,
  and explicit hydrogens.
- Modern executables share input, filtering, output, quiet, help, and debug
  behavior while retaining tool-specific analysis options.
- Grid-capable tools can write MRC2014 maps using real-space `ORIGIN` placement
  or CCP4-compatible maps using grid-index `NSTART` placement.
- The repository includes YAML-driven regression scenarios, converter and
  volume shell tests, release tooling, and expanded user and maintainer
  documentation.

### Notable fixes

- Grid bounds include safety padding that prevents out-of-bounds access at
  structure boundaries.
- Volume and grid-resolution reporting use consistent rounding and shared
  metric formatting.
- MRC output follows MRC2014 conventions with zeroed `NSTART`, real-space
  `ORIGIN`, single-volume metadata, and a valid machine stamp.
- Individual Makefile targets create `bin/` before linking.
- The YAML-driven whole-system runner now lives under `tests/e2e/`, keeping it
  out of fast pytest collection and eliminating false test-class warnings.

### Compatibility notes

- `Custom.exe` is now `ProteinRNAVolume.exe`.
- Plain `make` builds `Volume.exe`; use `make all` for the complete modern
  executable suite.
- `Volume-legacy.exe` remains a separate compatibility build and is not part of
  `make all`.
- MRC and CCP4 exports deliberately use different placement conventions.
  Workflows that require `NSTART` placement should select CCP4 output instead
  of renaming an MRC file.
- The repository license changes from GPLv3 to LGPLv3.

### Validation

- Current build targets and representative CLI help commands were verified.
- The relocated YAML runner listed all 11 scenarios, and the fast pytest suite
  passed 567 tests without collection warnings.
- README, Markdown-link, ASCII, and whitespace checks passed, together with a
  complete-docset scan that included newly added documentation.

## 2026-01-01 - 26.01b7
- Ensured single-target builds (for example `make vol`) create `bin/` before
  linking executables.
- Rounded grid resolution metrics consistently (for example `1000` instead of
  `999.999`) and reused a shared formatter across tools.

## 2025-12-30 - 26.01b6
- Switch to CalVer-style version strings (for example `26.01b6`).
- Standardized C++ tool CLIs to share XYZR filter flags and PDB-aware input handling.

## 2025-11-29 - 2.0.0-beta5
- In-memory XYZR buffers, direct PDB/mmCIF/PDBML input, and shared filtering flags
  across `Volume.exe`.

## 2025-11-28 - 2.0.0-beta4
- Grid padding fixes, Python regression harness (YAML suite), and a new reference
  build target.

## 2025-11-28 - 2.0.0-beta3
- Shared CLI helper (C++17) with `Volume.exe` migration.

## 2025-11-28 - 2.0.0-beta2
- Native converters with filtering and new test tooling.

## 2024-11-14 - 2.0.0-beta1
- Codebase modernization and groundwork for new converters.

## 2009-06-02 - 1.2
- Added MRC export support.

## 2006-10-24 - 1.1
- Minimal updates and maintenance release.

## 2006-06-20 - 1.0
- Initial public release.

## Notes
- Legacy source was hosted in Subversion:

```sh
svn co http://vossvolvox.svn.sourceforge.net/svnroot/vossvolvox vossvolvox
```
