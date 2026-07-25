# File structure

This map describes the current source, test, documentation, and generated-file
locations for the molecular-volume toolchain.

## Top-level layout

```text
AGENTS.md                repository guidance
README.md                project overview and documentation entry point
VERSION                  release version string
bin/                     generated C++ executables
devel/                   release, changelog, cleanup, and link-maintenance scripts
docs/                    project documentation and archived documentation
publications/            publication files
python/                  standalone PDB/MRC utilities and pyami helpers
src/                     C++ entry points, build rules, and shared C++ code
tests/                   shell/Python verification harnesses and test scenarios
xyzr/                    converter implementations, inputs, and atom-radius table
```

- [AGENTS.md](../AGENTS.md) supplies repository-specific implementation and test
  guidance.
- [README.md](../README.md) introduces the tools, citations, and primary
  documentation routes.
- [LICENSE.LGPL_v3](../LICENSE.LGPL_v3) contains the repository license text.
- [REPO_TYPE](../REPO_TYPE) identifies the repository as `other` for shared
  repository tooling.
- [dist_clean.sh](../dist_clean.sh) is the root distribution-cleanup wrapper.
- [pip_requirements-dev.txt](../pip_requirements-dev.txt) lists Python development
  dependencies used by the repository's checks and tooling.

## Key subtrees

- [src/](../src/) contains C++ source files and [src/Makefile](../src/Makefile).
  Its entry points compile into historical CamelCase executable names in `bin/`.
- [src/lib/](../src/lib/) contains the shared parser, input conversion, grid,
  output, MRC/CCP4, and CLI-common implementations and headers.
- [xyzr/](../xyzr/) contains the native-converter comparison inputs,
  [xyzr/pdb_to_xyzr.py](../xyzr/pdb_to_xyzr.py),
  [xyzr/pdb_to_xyzr.sh](../xyzr/pdb_to_xyzr.sh), and
  [xyzr/atmtypenumbers](../xyzr/atmtypenumbers).
- [python/](../python/) contains command-line PDB/MRC utilities. Its
  [python/pyami/](../python/pyami/) subtree provides MRC and array helper modules.
- [tests/](../tests/) contains shell regression tests and Python repository
  checks. Its [tests/e2e/](../tests/e2e/) subtree contains the YAML-driven
  [tests/e2e/e2e_test_suite.py](../tests/e2e/e2e_test_suite.py) runner and
  [tests/e2e/test_suite.yml](../tests/e2e/test_suite.yml) scenarios.
- [devel/](../devel/) contains version, changelog, release, cleanup, and Markdown
  link-maintenance helpers.
- Archive files, when created, are stored in `docs/archive/`. The current
  documentation set remains directly under [docs/](./).

## Generated artifacts

- `bin/` receives executable files such as `Volume.exe` and `pdb_to_xyzr.exe` from
  [src/Makefile](../src/Makefile). The ignore rules exclude
  `bin/*.exe`.
- [src/](../src/) and [src/lib/](../src/lib/) receive `*.o` object files during C++
  builds. The ignore rules exclude `*.o`.
- `tests/pdb_to_xyzr_results/` is created by
  [tests/test_pdb_to_xyzr.sh](../tests/test_pdb_to_xyzr.sh) for downloaded inputs,
  converted files, and comparison outputs.
- `tests/volume_results/` is the scenario work directory
  used by [tests/e2e/test_suite.yml](../tests/e2e/test_suite.yml).
- Shell tests can also create PDB and XYZR files in [tests/](../tests/). The ignore
  rules exclude `*.pdb` and `*.xyzr` globally.
- [devel/clean_build.sh](../devel/clean_build.sh) and
  [devel/dist_clean.sh](../devel/dist_clean.sh) provide developer cleanup paths;
  [src/Makefile](../src/Makefile) also defines `clean` and `distclean` targets.

## Documentation map

- [AUTHORS.md](AUTHORS.md): contributor and maintainer information.
- [CHANGELOG.md](CHANGELOG.md): chronological project changes.
- [CODE_ARCHITECTURE.md](CODE_ARCHITECTURE.md): code components and data flow.
- [E2E_TESTS.md](E2E_TESTS.md): end-to-end testing guidance.
- [FILE_STRUCTURE.md](FILE_STRUCTURE.md): this directory and artifact map.
- [INSTALL.md](INSTALL.md): build prerequisites and setup.
- [MARKDOWN_STYLE.md](MARKDOWN_STYLE.md): Markdown requirements.
- [PYTEST_STYLE.md](PYTEST_STYLE.md): pytest design and hygiene guidance.
- [PYTHON_STYLE.md](PYTHON_STYLE.md): Python implementation conventions.
- [QUICKSTART.md](QUICKSTART.md): short workflow walkthrough.
- [RELEASE_HISTORY.md](RELEASE_HISTORY.md): released-version history.
- [REPO_STYLE.md](REPO_STYLE.md): repository-wide conventions.
- [TODO.md](TODO.md): remaining work notes.
- [USAGE.md](USAGE.md): command-line usage information.
- [UTILS.md](UTILS.md): shared C++ utility reference.

## Where to add new work

- Add C++ executables in [src/](../src/) and their build targets in
  [src/Makefile](../src/Makefile).
- Add shared C++ code in [src/lib/](../src/lib/) and keep tool-specific logic in its
  entry point.
- Add converter implementations or converter data in [xyzr/](../xyzr/).
- Add standalone Python utilities in [python/](../python/) and supporting modules
  in [python/pyami/](../python/pyami/) when they are reusable there.
- Add executable-focused shell tests or Python checks in [tests/](../tests/).
- Add durable Markdown documentation directly under [docs/](./) using an uppercase,
  underscore-separated filename; place archived documents in `docs/archive/`.

## Known gaps

- Align the generated-test-artifact ignore policy with active `tests/` paths if
  cached results are not intended for version control.
