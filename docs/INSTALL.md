# Install

Installation builds the current C++ command-line suite in `bin/`. The repository
is not a packaged library: run the tools from a source checkout.

## Requirements

- A C++17 compiler available as `g++`; the Makefile invokes `g++` and enables
  `-march=native` and `-mtune=native`.
- GNU Make.
- Gemmi C++ headers only when native mmCIF (`.cif`/`.mmcif`) or PDBML
  (`.pdbml`/`.pdbxml`) input is needed. The build detects the headers with
  `__has_include`.
- Python 3.12 for the optional Python converter and Python-based checks.
- `gawk` only for the optional shell converter in `xyzr/`.

## Build the current executable suite

From the repository root, build every current C++ executable:

```sh
cd src
make all
cd ..
```

`make` by itself builds only the default `vol` target (`bin/Volume.exe`). For a
focused build, use `make vol` for `Volume.exe` or `make pdbxyzr` for
`pdb_to_xyzr.exe`.

## Verify install

```sh
./bin/Volume.exe -h
./bin/pdb_to_xyzr.exe -h
```

Both commands print their option lists and exit successfully. See
[USAGE.md](USAGE.md) for a PDB-to-XYZR-to-volume workflow.

## Compatibility builds and utilities

- `make volume_original` separately builds `bin/Volume-legacy.exe` for the
  historical comparison harness. It is not part of `make all`, and its source
  files are frozen.
- `xyzr/pdb_to_xyzr.py` and `xyzr/pdb_to_xyzr.sh` are alternative converter
  utilities. They are not C++ binaries installed in `bin/`; see their distinct
  invocation and input support in [USAGE.md](USAGE.md).

## Known gaps

- TODO: Verify a supported cross-compiler configuration for hosts where `g++`
  or the native CPU flags are unavailable.
- TODO: Document a tested Gemmi-header installation method for each supported
  platform.
