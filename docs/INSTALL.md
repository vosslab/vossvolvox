# Install

Install means building the C++17 command-line executables into [bin/](bin/) and
optionally using the Python utilities under [python/](python/) and [xyzr/](xyzr/).

## Requirements
- C++17 compiler (g++ or clang++).
- GNU Make.
- Python 3.12 for Python utilities and tests.
- Gemmi headers for mmCIF or PDBML inputs (optional but required for those
  formats).

## Install steps
- Build the executables:

```sh
cd src
make
cd ..
```

- Confirm the executables exist in [bin/](bin/).

## Verify install

```sh
./bin/Volume.exe -h
```

## Known gaps
- TODO: Confirm supported operating systems and shells.
- TODO: Document how to install Gemmi headers for C++ builds on macOS and Linux.
- TODO: Document any required Python dependencies for [python/](python/) scripts.
