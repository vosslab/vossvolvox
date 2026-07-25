# Development workflow

This guide covers local development of the C++17 command-line tools, their
Python helpers, and their deterministic regression checks.

## Prepare a checkout

- Use a C++17-capable `g++` or `clang++` and GNU Make.
- Use Python 3.12 for the Python utilities and test tooling.
- Install the checked-in developer dependencies before running the Python
  checks:

```sh
python3.12 -m pip install -r pip_requirements-dev.txt
```

- The YAML suite in
  [tests/e2e/e2e_test_suite.py](../tests/e2e/e2e_test_suite.py) imports `yaml`.
  Ensure PyYAML is available in the development environment; it is not yet
  listed in [pip_requirements-dev.txt](../pip_requirements-dev.txt).
- Gemmi headers are optional for a basic build, but are required when testing
  mmCIF or PDBML input support. See [INSTALL.md](INSTALL.md) for the current
  dependency notes.

## Build a tool

Build from [src](../src), which places executables in generated `bin/`:

```sh
cd src
make
cd ..
```

The Makefile default is `vol`, so `make` builds `Volume.exe`. Build all modern
tools with `make all`, or build the changed tool and its shared objects with a
named target:

```sh
cd src
make vol       # bin/Volume.exe
make pdbxyzr   # bin/pdb_to_xyzr.exe
make cav       # bin/Cavities.exe
```

Confirm a CLI change through the executable help before wider testing:

```sh
./bin/Volume.exe --help
./bin/pdb_to_xyzr.exe --help
```

The build uses C++17 and optimizes for the local CPU. The Makefile detects
OpenMP support and adds its compiler flag only when the compiler advertises it.

## Run tests

Start with the smallest relevant check after a code change:

```sh
cd src && make vol
cd ..
./tests/test_volume.sh
```

[test_volume.sh](../tests/test_volume.sh) downloads or reuses 2LYZ, converts it
to XYZR, runs `Volume.exe`, and checks volume, surface, output counts, and a
HETATM-only MD5. It caches the input in the test directory and cleans its
temporary files on exit.

For converter changes, build the native converter and compare the C++, Python,
and shell implementations:

```sh
cd src && make pdbxyzr
cd ..
./tests/test_pdb_to_xyzr.sh 1A01
```

This test stages reusable artifacts under `tests/pdb_to_xyzr_results/<PDBID>/`
and reports timings, line counts, checksums, and line differences.

The YAML-driven regression harness builds missing binaries by default. List its
scenarios without running them, then run the suite when the relevant coverage
applies:

```sh
/opt/homebrew/opt/python@3.12/bin/python3.12 tests/e2e/e2e_test_suite.py --list
/opt/homebrew/opt/python@3.12/bin/python3.12 tests/e2e/e2e_test_suite.py
```

Run the fast repository checks with pytest:

```sh
pytest tests/
```

Follow [PYTEST_STYLE.md](PYTEST_STYLE.md) when adding Python tests. A fragile
pytest asserts dates, collection sizes, required-key lists, hardcoded defaults,
or other tunable implementation details instead of observable behavior.

## Change code safely

- Use four-space indentation and brace-on-newline functions in C++.
- Keep C++ sources in snake_case; retain the established CamelCase executable
  names and `.exe` suffixes.
- Extend the shared `ArgumentParser` and XYZR helpers for a new modern CLI
  rather than duplicating input and option parsing.
- Keep [utils-main-legacy.cpp](../src/lib/utils-main-legacy.cpp) and
  [volume-legacy.cpp](../src/volume-legacy.cpp) unchanged; they are frozen
  legacy implementations.
- Make shell tests idempotent and cache downloaded fixtures under
  `tests/pdb_to_xyzr_results/<PDBID>/` when converter coverage needs them.
- When converter output changes, report the relevant MD5 result in the test
  output and update expected values only after investigating the difference.

The shared repository policies in [REPO_STYLE.md](REPO_STYLE.md),
[PYTHON_STYLE.md](PYTHON_STYLE.md), and [MARKDOWN_STYLE.md](MARKDOWN_STYLE.md)
apply to all contributions.

## Clean build outputs

Use the Makefile targets from [src](../src) for C++ artifacts:

```sh
cd src
make clean
make distclean
```

`make clean` removes object files and editor backups. `make distclean` also
removes generated `bin/`. The repository-level cleaners cover broader generated
artifacts and caches; review their removal scope before running them:

```sh
./devel/clean_build.sh
./devel/dist_clean.sh
```

## Prepare a release

- Update [VERSION](../VERSION), [CHANGELOG.md](CHANGELOG.md), and
  [RELEASE_HISTORY.md](RELEASE_HISTORY.md) when user-facing behavior changes.
- Use a focused Conventional Commit-style message such as `feat:`, `chore:`, or
  `meta:`. A human performs the commit.
- Run the closest build and regression checks, then record commands and results
  in the review or release notes.
- Preview source-release validation without writing archives:

```sh
/opt/homebrew/opt/python@3.12/bin/python3.12 devel/make_release.py --dry-run
```

[make_release.py](../devel/make_release.py) validates the version, a committed
license, and the source snapshot, then prints the manual tag and GitHub-release
commands. Its `--write` mode builds archives under `output_release/`; use it
only when preparing the release artifacts.
