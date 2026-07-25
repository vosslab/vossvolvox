# v26.07 - 2026-07-25

## Added

- Accepts PDB and XYZR input across modern tools, plus mmCIF and PDBML when
  built with Gemmi.
- Adds a native C++ `pdb_to_xyzr.exe` converter with residue and atom filtering
  options.
- Adds CCP4 density-map output using `NSTART` placement alongside MRC and EZD
  exports.
- Adds YAML-driven regression tests, converter comparisons, release tooling,
  and expanded user and maintainer documentation.

## Changed

- Modernizes the toolchain to C++17 with shared CLI, structure-loading, grid,
  and output libraries.
- Standardizes common input, filtering, output, quiet, help, and debug options
  across modern executables.
- Renames `Custom.exe` to `ProteinRNAVolume.exe` and retains
  `Volume-legacy.exe` as a separate compatibility build.
- Makes `Volume.exe` the default build target, while `make all` builds the
  complete modern suite.
- Changes the repository license from GPLv3 to LGPLv3.

## Fixed

- Pads grid bounds to prevent out-of-bounds access at structure boundaries.
- Corrects volume and grid-resolution rounding and standardizes metric
  reporting.
- Updates MRC output to MRC2014 conventions, using real-space `ORIGIN`, zeroed
  `NSTART`, single-volume metadata, and a valid machine stamp.
- Ensures individual build targets create `bin/` before linking and improves
  C++ buffer sizing and formatted-output safety.
- Moves the YAML-driven whole-system runner under `tests/e2e/` so the fast
  pytest suite no longer emits false test-class collection warnings.
