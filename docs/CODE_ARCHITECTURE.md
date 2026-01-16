# Code architecture

High-level system design, major components, and data flow for the toolchain.

## Overview
The codebase builds a suite of C++17 command-line tools that load structural
inputs (PDB/mmCIF/PDBML/XYZR), build voxel grids, and compute volumes, channels,
and related geometric outputs. Tools share a common CLI layer and
converter helpers so behavior and flags stay consistent across executables.

## Major components
- [src/](src/): C++ entry points for each executable, built via
  [src/Makefile](src/Makefile) into [bin/](bin/).
- [src/lib/](src/lib/): shared CLI, parsing, and grid helpers including
  [src/lib/argument_helper.cpp](src/lib/argument_helper.cpp),
  [src/lib/pdb_io.cpp](src/lib/pdb_io.cpp),
  [src/lib/xyzr_cli_helpers.cpp](src/lib/xyzr_cli_helpers.cpp),
  [src/lib/vossvolvox_cli_common.cpp](src/lib/vossvolvox_cli_common.cpp), and
  [src/lib/utils-main.cpp](src/lib/utils-main.cpp).
- Legacy C++ entry points in [src/volume-legacy.cpp](src/volume-legacy.cpp) and
  [src/lib/utils-main-legacy.cpp](src/lib/utils-main-legacy.cpp).
- [xyzr/](xyzr/): legacy and scripting converters such as
  [xyzr/pdb_to_xyzr.py](xyzr/pdb_to_xyzr.py) and
  [xyzr/pdb_to_xyzr.sh](xyzr/pdb_to_xyzr.sh), plus
  [xyzr/atmtypenumbers](xyzr/atmtypenumbers) data.
- [python/](python/): Python utilities for PDB and MRC processing, including
  [python/pyami/](python/pyami/).
- [tests/](tests/): shell and Python regression harnesses and YAML scenarios.

## Data flow
1. Input selection: tools accept XYZR or raw structure formats and parse shared
   CLI flags through [src/lib/argument_helper.cpp](src/lib/argument_helper.cpp).
2. Conversion: [src/lib/pdb_io.cpp](src/lib/pdb_io.cpp) loads structure inputs
   into in-memory XYZR buffers using embedded atom type data.
3. Grid setup: shared utilities compute bounds, padding, and grid spacing for
   voxel allocation.
4. Grid fill and analysis: entry points in [src/](src/) rasterize atoms and run
   tool-specific analyses (volumes, channels, cavities, or tunnels).
5. Output: tools emit summary metrics and optional PDB, EZD, or MRC outputs via
   shared output helpers.

## Testing and verification
- [tests/test_volume.sh](tests/test_volume.sh) validates Volume output metrics.
- [tests/test_pdb_to_xyzr.sh](tests/test_pdb_to_xyzr.sh) compares converter
  parity and MD5s.
- [tests/test_suite.py](tests/test_suite.py) runs YAML scenarios defined in
  [tests/test_suite.yml](tests/test_suite.yml).

## Extension points
- Add new C++ tools in [src/](src/), register targets in
  [src/Makefile](src/Makefile), and reuse
  [src/lib/argument_helper.cpp](src/lib/argument_helper.cpp) plus
  [src/lib/xyzr_cli_helpers.cpp](src/lib/xyzr_cli_helpers.cpp).
- Add shared C++ utilities in [src/lib/](src/lib/) and update callers.
- Add converter scripts in [xyzr/](xyzr/) or supporting Python utilities in
  [python/](python/).
- Add or update regression checks in [tests/](tests/).

## Known gaps
- Confirm whether [test/](test/) is still used in active workflows alongside
  [tests/](tests/).
