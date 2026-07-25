# Code architecture

This repository provides grid-based command-line programs for molecular structure
analysis. The modern C++ tools accept XYZR coordinates or supported structure files,
then calculate volumes, cavities, channels, tunnel-related geometry, or density maps.

## Overview

- [src/Makefile](../src/Makefile) builds C++17 executables in `bin/`.
  Its default target is `vol`; `make all` builds the complete modern tool set.
- Each C++ entry point in [src/](../src/) links the shared modern object set unless
  it is the native `pdb_to_xyzr` converter.
- The two frozen legacy sources,
  [src/volume-legacy.cpp](../src/volume-legacy.cpp) and
  [src/lib/utils-main-legacy.cpp](../src/lib/utils-main-legacy.cpp), build the
  separate `Volume-legacy.exe` target.
- [xyzr/](../xyzr/) retains Python and shell converter implementations and their
  atom-radius table; [python/](../python/) contains separate PDB and MRC utilities.

## Major components

- [src/lib/argument_helper.hpp](../src/lib/argument_helper.hpp) implements the
  common option parser, `--quiet`, input/output option helpers, and filter flags.
- [src/lib/vossvolvox_cli_common.cpp](../src/lib/vossvolvox_cli_common.cpp) maps
  shared filter and output settings to the structure-conversion and debug layers.
- [src/lib/pdb_io.cpp](../src/lib/pdb_io.cpp) recognizes XYZR, PDB, mmCIF, and
  PDBML inputs. It converts structure data to XYZR atoms and uses the embedded
  [src/lib/atmtypenumbers_data.hpp](../src/lib/atmtypenumbers_data.hpp) radius data.
  When Gemmi headers are available, its interface exposes Gemmi-backed conversion.
- [src/lib/xyzr_cli_helpers.cpp](../src/lib/xyzr_cli_helpers.cpp) loads an input
  into `XYZRBuffer` data and prepares shared grid bounds from one or more buffers.
- [src/lib/utils-main.cpp](../src/lib/utils-main.cpp) owns grid dimensions, grid
  coordinate helpers, rasterization, and grid transformations. Individual programs
  perform their analysis through these shared grid operations.
- [src/lib/utils-output.cpp](../src/lib/utils-output.cpp),
  [src/lib/utils-mrc.cpp](../src/lib/utils-mrc.cpp), and
  [src/lib/utils-ccp4.cpp](../src/lib/utils-ccp4.cpp) write PDB, EZD, MRC, and
  CCP4 outputs. The shared output options include `--pdb-output`, `--ezd-output`,
  `--mrc-output`, and `--ccp4-output`.
- [src/](../src/) contains the analysis entry points: volumes, cavities, channels,
  solvent, van der Waals calculations, tunnel analysis, RNA/protein volume, and
  fractal dimension. The target-to-executable mapping is the source of truth in
  [src/Makefile](../src/Makefile).
- [src/pdb_to_xyzr.cpp](../src/pdb_to_xyzr.cpp) is the native converter. It writes
  XYZR text to standard output and accepts either `-i`/`--input` or one positional
  input path.

## Data flow

1. A modern analysis executable parses its tool-specific options and the common
   input, filter, output, quiet, and debug options.
2. The input loader either reads XYZR records or converts PDB, mmCIF, or PDBML into
   an in-memory `XYZRBuffer`. Structure filters are applied during conversion.
3. Shared grid preparation derives coordinate limits and allocates boolean voxel
   grids. Entry points use the configured spacing and probe-related options for
   their analysis.
4. The selected executable fills, transforms, compares, or traverses grid data to
   calculate its tool-specific result.
5. Shared reporting prints metrics, and requested output settings write PDB, EZD,
   MRC, or CCP4 files. The `pdb_to_xyzr.exe` path ends after conversion and writes
   XYZR to standard output instead.

## Testing and verification

- [tests/test_volume.sh](../tests/test_volume.sh) retrieves or reuses `2LYZ`,
  converts it with the native converter, runs `Volume.exe`, and checks metrics plus
  a HETATM-only MD5.
- [tests/test_pdb_to_xyzr.sh](../tests/test_pdb_to_xyzr.sh) stages a PDB input and
  compares native C++, Python, and shell converter outputs, timings, line counts,
  MD5s, and differing lines.
- [tests/e2e/e2e_test_suite.py](../tests/e2e/e2e_test_suite.py) executes the
  YAML scenarios in [tests/e2e/test_suite.yml](../tests/e2e/test_suite.yml).
  The scenarios exercise multiple generated binaries and expected output
  summaries or files.
- The repository also carries Python checks for formatting, imports, shebangs,
  Markdown links, and test hygiene in [tests/](../tests/).

## Extension points

- Add a new modern executable by adding an entry point in [src/](../src/) and a
  target plus executable name in [src/Makefile](../src/Makefile). Link the shared
  `OBJS` set when the tool uses the common grid and CLI layers.
- Put reusable C++ behavior in [src/lib/](../src/lib/) and add its object build rule
  to [src/Makefile](../src/Makefile) when needed by executable targets.
- Extend structure conversion in [src/lib/pdb_io.cpp](../src/lib/pdb_io.cpp) and
  surface common flags through
  [src/lib/vossvolvox_cli_common.cpp](../src/lib/vossvolvox_cli_common.cpp).
- Add converter parity work under [xyzr/](../xyzr/) and regression coverage under
  [tests/](../tests/). Keep the legacy C++ sources unchanged.

## Known gaps

- Verify a documented policy for retaining or cleaning the cached test-result
  directories after test runs; the current ignore rules still name `test/` while
  active test scripts use `tests/`.
