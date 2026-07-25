# Code architecture

High-level system design, major components, and data flow for the toolchain.

## Overview
The codebase builds a suite of C++17 command-line tools that load structural
inputs (PDB/mmCIF/PDBML/XYZR), build voxel grids, and compute volumes, channels,
and related geometric outputs. Tools share a common CLI layer and
converter helpers so behavior and flags stay consistent across executables.

## Major components
- `src`: C++ entry points for each executable, built via
  [Makefile](../src/Makefile) into `bin`.
- `lib`: shared CLI, parsing, and grid helpers including
  [argument_helper.cpp](../src/lib/argument_helper.cpp),
  [pdb_io.cpp](../src/lib/pdb_io.cpp),
  [xyzr_cli_helpers.cpp](../src/lib/xyzr_cli_helpers.cpp),
  [vossvolvox_cli_common.cpp](../src/lib/vossvolvox_cli_common.cpp), and
  [utils-main.cpp](../src/lib/utils-main.cpp).
- Legacy C++ entry points in [volume-legacy.cpp](../src/volume-legacy.cpp) and
  [utils-main-legacy.cpp](../src/lib/utils-main-legacy.cpp).
- `xyzr`: legacy and scripting converters such as
  [pdb_to_xyzr.py](../xyzr/pdb_to_xyzr.py) and
  [pdb_to_xyzr.sh](../xyzr/pdb_to_xyzr.sh), plus
  [atmtypenumbers](../xyzr/atmtypenumbers) data.
- `python`: Python utilities for PDB and MRC processing, including
  `pyami`.
- `tests`: shell and Python regression harnesses and YAML scenarios.

## Data flow
1. Input selection: tools accept XYZR or raw structure formats and parse shared
   CLI flags through [argument_helper.cpp](../src/lib/argument_helper.cpp).
2. Conversion: [pdb_io.cpp](../src/lib/pdb_io.cpp) loads structure inputs
   into in-memory XYZR buffers using embedded atom type data.
3. Grid setup: shared utilities compute bounds, padding, and grid spacing for
   voxel allocation.
4. Grid fill and analysis: entry points in `src` rasterize atoms and run
   tool-specific analyses (volumes, channels, cavities, or tunnels).
5. Output: tools emit summary metrics and optional PDB, EZD, or MRC outputs via
   shared output helpers.

## Testing and verification
- [test_volume.sh](../tests/test_volume.sh) validates Volume output metrics.
- [test_pdb_to_xyzr.sh](../tests/test_pdb_to_xyzr.sh) compares converter
  parity and MD5s.
- [test_suite.py](../tests/test_suite.py) runs YAML scenarios defined in
  [test_suite.yml](../tests/test_suite.yml).

## Extension points
- Add new C++ tools in `src`, register targets in
  [Makefile](../src/Makefile), and reuse
  [argument_helper.cpp](../src/lib/argument_helper.cpp) plus
  [xyzr_cli_helpers.cpp](../src/lib/xyzr_cli_helpers.cpp).
- Add shared C++ utilities in `lib` and update callers.
- Add converter scripts in `xyzr` or supporting Python utilities in
  `python`.
- Add or update regression checks in `tests`.

## Known gaps
- Confirm whether `test` is still used in active workflows alongside
  `tests`.
