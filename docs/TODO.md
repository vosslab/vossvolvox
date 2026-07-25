# TODO

Small, evidence-backed follow-up tasks without a delivery timeline. For larger
work, use a dedicated plan or release record instead.

## Installation documentation

- Verify a supported compiler configuration for hosts where `g++`,
  `-march=native`, or `-mtune=native` is unavailable, as called out in
  [INSTALL.md](INSTALL.md).
- Add tested macOS and Linux instructions for making Gemmi headers available
  to the C++ build, as called out in [INSTALL.md](INSTALL.md).
- Inventory Python dependencies used by the utilities under `python` and
  document their installation requirements in [INSTALL.md](INSTALL.md).

## Command-line documentation

- Inventory the executables that write EZD or MRC output, verify their flags
  from each tool's `--help`, and document the results in [USAGE.md](USAGE.md).
- Add a small offline structure fixture and use it in the quick-start workflow,
  as called out in [USAGE.md](USAGE.md).
