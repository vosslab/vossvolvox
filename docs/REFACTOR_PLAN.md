# Refactor Plan

## Purpose
Reduce repeated CLI, XYZR loading, and grid initialization code across executables, while keeping behavior stable and avoiding changes to legacy code.

## Scope
Modern executables that use `src/lib/utils-main.cpp` and the in-memory XYZR buffer path.

Not in scope:
- Any legacy-only executables or legacy grid codepaths.
- Numerical or algorithm changes.

## Hard constraints
- Legacy freeze: per AGENTS, **do not edit** `src/lib/utils-main-legacy.cpp` or `src/volume-legacy.cpp`.
- Keep existing output stable for the pilot tools and for the rollout set.

## Invariants (must not change)
### Grid initialization order
All refactors must preserve this order for grid bounds and sizing:

1. `GRID = grid_spacing`
2. `initGridState(max_probe)` (modern codepath only)
3. set tracking label (if used for logging)
4. `read_NumAtoms_from_array()` once per XYZRBuffer that shares the grid
5. `assignLimits()` exactly once

Important: allocate large grids only after `assignLimits()`.

### assignLimits() padding
`assignLimits()` already applies a padding/fudge factor to handle rounding
errors when converting bounds to grid dimensions. Preserve the existing padding
behavior; do not remove or tighten it during refactor.

### Probe semantics
`max_probe` passed into grid setup must match the tool's current meaning. Do not infer a new value during refactor.

### Multi-input behavior
Some tools need per-input atom counts, others only need totals. The helper must support both without changing semantics.

## Phases

### Phase 0: Baseline outputs
- Run existing tests for the pilot tools and retain outputs as reference.

### Phase 1: Rename in modern utilities only
- Implement `initGridState()` in `src/lib/utils-main.cpp`.
- Keep `finalGridDims()` as a thin wrapper for compatibility.
- Leave legacy code unchanged.

### Phase 2: New helper module
- Add `src/lib/xyzr_cli_helpers.{h,cpp}` with shared XYZR loading and grid prep.

### Phase 3: Pilot refactors
- Convert `volume.cpp`, `two_volumes-fill_cavities.cpp`, and `volume-fill_cavities.cpp`.
- Optionally add `--debug-limits` to pilots only, gated behind a flag.

### Phase 4: Rollout
- Convert remaining modern single-input tools, then multi-input tools.

### Phase 4b: Common CLI settings refactor
Add a shared helper module for filter flags, output options, and conversion option mapping.

Goal
Remove repeated boilerplate for XYZR filters and output file arguments across executables, while keeping tool-specific inputs and numeric parameters local.

Scope
Centralize only the pieces that are common and stable across tools:
- filter flags (hydrogens, exclude flags)
- output file options (pdb, ezd, mrc, and other recurring output groups)
- mapping of filter flags into pdbio::ConversionOptions

Tool-specific settings remain local.

4b.1 Introduce two common settings objects
Create a small module:
- vossvolvox_cli_common.hpp
- vossvolvox_cli_common.cpp

Add structs:
- FilterSettings
  - bool use_hydrogens
  - bool exclude_ions
  - bool exclude_ligands
  - bool exclude_hetatm
  - bool exclude_water
  - bool exclude_nucleic
  - bool exclude_amino
- OutputSettings
  - std::string pdbFile
  - std::string ezdFile
  - std::string mrcFile

Rationale: these are stable across tools, and do not assume probe counts, grids, or number of inputs.

4b.2 Centralize parser wiring for common pieces
Add helper functions:
- void add_filter_options(ArgumentParser& parser, FilterSettings& f);
  - wraps add_xyzr_filter_flags(parser, ...)
  - keeps naming and help text consistent
- void add_output_options(ArgumentParser& parser, OutputSettings& o);
  - wraps add_output_file_options(parser, o.pdbFile, o.ezdFile, o.mrcFile);

Notes:
- No parsing happens here.
- Tools can call one or both helpers.

4b.3 Centralize filter mapping into ConversionOptions
Add:
- pdbio::ConversionOptions make_conversion_options(const FilterSettings& f);

Implementation preserves current behavior:
- use_united = !f.use_hydrogens
- copy exclude flags into convert_options.filters

4b.4 Optional: small probe helpers
Because probes vary, prefer tiny helpers rather than a single monolithic settings struct. Examples:
- struct ProbeSetting { double value = 10.0; };
- struct TwoProbeSettings { double probe1 = 10.0; double probe2 = 1.4; };
- void add_probe_option(ArgumentParser& parser, double& probe, double default_value);
- void add_two_probe_options(ArgumentParser& parser, double& probe_a, double& probe_b, double def_a, double def_b);

These are optional; the core win is filters and outputs.

4b.5 Update executables to use composition
For each main():
1. Keep tool-specific variables local (input files, probes, grid, etc.).
2. Replace common variable blocks with FilterSettings filters and OutputSettings outputs.
3. Replace wiring calls with add_filter_options(parser, filters) and add_output_options(parser, outputs).
4. Replace manual mapping with make_conversion_options(filters).

4b.6 Validation
- Compile all tools.
- Confirm --help output still contains the same flags and defaults.
- Run representative commands for a tool with one probe, two probes, and no probe.
- Confirm filtering behavior matches prior outputs.

Deliverables
- vossvolvox_cli_common.{hpp,cpp} with FilterSettings, OutputSettings, and helper functions
- Updated executables using composition rather than copy-pasted option and mapping code
- No CLI behavior changes, less boilerplate, less risk of divergence

### Phase 5: Cleanup
- Keep `finalGridDims()` wrapper for at least one release window.
- Remove temporary debug hooks or keep them behind flags.
- Update docs that reference the old name.

## Planned changes

### 1. Rename `finalGridDims` in modern code
- In `src/lib/utils-main.cpp`:
  - Introduce `initGridState(float maxprobe)` with the current implementation of `finalGridDims`.
  - Keep `finalGridDims(float maxprobe)` as a wrapper that calls `initGridState` for compatibility.
- In legacy files:
  - Keep `finalGridDims` untouched.

### 2. Add shared helper module
Add `src/lib/xyzr_cli_helpers.{h,cpp}`.

Proposed API (subject to small adjustments during implementation):

- `bool load_xyzr_or_exit(const std::string& path, const ConversionOptions& opts, XYZRBuffer& out);`
- `struct GridPrepResult { int total_atoms; std::vector<int> per_input; };`
- `GridPrepResult prepare_grid_from_xyzr(const std::vector<const XYZRBuffer*>& buffers, float grid_spacing, float max_probe, const std::string& input_label, bool debug_limits);`

Helper responsibilities:
- Centralize XYZR load and XYZRBuffer population.
- Centralize grid init sequence and return atom counts.
- Optionally call `testLimits()` when `debug_limits` is true.

### 3. Optional debug hook for pilots
- Consider `--debug-limits` for pilot tools if we can call it after grids are populated.
- Default behavior must remain unchanged.
- `testLimits()` requires a populated grid, so defer unless the call site is safe.

## Pilots
Refactor these first:
- `volume.cpp` (single input)
- `two_volumes-fill_cavities.cpp` (multi-input)
- `volume-fill_cavities.cpp` (cavity-fill path)

## Verification
For each pilot:
- Compare outputs before vs after refactor using existing test scripts and reference outputs.
- When using `--debug-limits`, verify bounds and dimensions match before vs after on the same input.
- Confirm no changes to legacy files.

## Rollout
After pilots pass:
1. Convert remaining modern single-input tools.
2. Convert remaining modern multi-input tools using the vector-of-buffers helper.
3. Keep the compatibility wrapper `finalGridDims()` in modern utilities for at least one release window.

## Notes
- If memory usage becomes a problem, add an optional guard after `assignLimits()` that estimates required bytes from `NUMBINS` and fails early with a helpful message. This should be a separate change and should not alter default behavior during the initial refactor.

## Progress (living log)
- [x] Baseline outputs captured for pilot tools.
- [x] `initGridState()` added (wrapper `finalGridDims()` retained).
- [x] Document `assignLimits()` padding behavior as a preserved invariant.
- [x] `xyzr_cli_helpers` module added.
- [x] Pilots refactored and validated.
- [ ] Optional: add `--debug-limits` to pilots once there is a safe call site.
- [x] Unified --debug flag added for filters, inputs/outputs, grid state, and timing.
- [x] Rollout completed for remaining modern tools.
- [x] Phase 4b common CLI settings refactor implemented.
- [x] Phase 5 cleanup complete (wrapper retained).
