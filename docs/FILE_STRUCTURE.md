# File structure

Directory map of the repository and where files belong, including generated assets.

## Top-level layout
- `src/`: C++17 sources for all command-line tools.
- `src/lib/`: shared C++ helpers, grid utilities, and converter libraries.
- `bin/`: generated executables (built by `cd src && make`).
- `xyzr/`: legacy and Python converters for XYZR.
- `python/`: Python utilities and scripts used by tests and workflows.
- `tests/`: shell tests, regression harnesses, and support scripts.
- `docs/`: project documentation (see list in `README.md`).
- `publications/`: published papers and PDFs.
- `devel/`: developer utilities and one-off helpers.
- `mapman/`: mapman data and related assets.
- `VERSION`: current release version string.
- `README.md`, `LICENSE`, `AGENTS.md`: root-level project docs and policies.

## Generated assets
- `bin/`: compiled executables (for example `Volume.exe`, `pdb_to_xyzr.exe`).
- `tests/pdb_to_xyzr_results/<PDBID>/`: cached downloads, XYZR outputs, MD5s.
- `tests/volume_results/<PDBID>/`: volume outputs and reference artifacts.
- `tests/pdb_to_xyzr_results/<PDBID>/*.pdb.gz`: cached PDB downloads.
- `tests/pdb_to_xyzr_results/<PDBID>/*.xyzr`: generated converter outputs.
- `tests/volume_results/<PDBID>/*.pdb`, `*.mrc`, `*.md5`: volume outputs.

## Documentation placement
- Keep most docs in `docs/`.
- Root-level docs are limited to `README.md`, `LICENSE`, `AGENTS.md`, and `VERSION`.

## Where to add new work
- New C++ binaries: `src/` with `argument_helper.*` integration via `src/lib/`.
- New shared C++ helpers: `src/lib/`.
- New converters or filters: `src/` (native) or `xyzr/` (legacy/Python).
- New tests: `tests/` (shell and support scripts).
- New docs: `docs/` with SCREAMING_SNAKE_CASE filenames.

## Source naming
- Use snake_case for C++ source files in `src/` (for example `all_channel_accessible.cpp`).
- Keep executables in CamelCase with the `.exe` suffix (for example `AllChannel.exe`).
