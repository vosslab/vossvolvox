# Code architecture

High-level system design, major components, and data flow for the toolchain.

## Overview
The codebase builds a suite of C++17 command-line tools that load structural
inputs (PDB/mmCIF/PDBML/XYZR), build voxel grids, and compute volumes, channels,
and related geometric outputs. Tools share a common CLI layer and converter
helpers so behavior and flags stay consistent across executables.

## Major components
- CLI layer: `argument_helper.*` defines shared flags, output options, and banner
  behavior for all executables.
- Converters: `pdb_to_xyzr` (C++ and Python) plus shared XYZR helpers transform
  structural inputs into XYZR streams or in-memory buffers.
- Grid and geometry core: voxel grid generation, exclusion, and surface
  calculations live in `src/` utilities (for example `utils-main.cpp`).
- Executables: tool-specific entry points (for example `Volume.exe`,
  `Channel.exe`, `Cavities.exe`, `Solvent.exe`, `Tunnel.exe`, `VDW.exe`) that
  orchestrate input parsing, grid construction, and output emission.
- Test harnesses: shell scripts and Python runners in `tests/` and `python/`
  validate converter parity and volume stability.

## Data flow
1. Input selection: the tool accepts `.xyzr` or raw structure formats
   (`.pdb`, `.mmcif`, `.pdbml`, optionally `.gz`).
2. Conversion: if the input is not XYZR, the converter reads the structure and
   applies exclude filters (ions, water, ligands, etc.).
3. Grid setup: grid dimensions and spacing are derived from the bounding box,
   with padding for probe radius and atom radii.
4. Grid fill: atom spheres are rasterized into voxel grids, creating accessible
   or excluded maps depending on the tool.
5. Analysis: voxel grids are processed to compute volumes, surfaces, channels,
   cavities, or tunnel geometry.
6. Output: results are written as PDB, EZD, MRC, or summary metrics, depending
   on the executable.

## Shared conventions
- C++17 standard, four-space indentation, brace-on-newline for functions.
- Shared `ArgumentParser` usage to keep flags consistent.
- XYZR helpers are used to avoid ad-hoc parsing differences across tools.

## Testing and verification
- `tests/test_volume.sh` validates volume outputs and cached artifacts.
- `tests/test_pdb_to_xyzr.sh` compares C++/Python/shell converter outputs and MD5s.
- Python harnesses exercise YAML-defined suites and reference builds.
