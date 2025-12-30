# Install

Set up the Voss Volvox toolchain and build the C++17 executables.

## Prerequisites
- C++17 compiler (g++ 7+ or clang++)
- GNU Make

## Build steps
1. Clone the repository:

```sh
git clone https://github.com/vosslab/vossvolvox.git
```

2. Build from the source directory:

```sh
cd vossvolvox/src
make
cd ..
```

3. Confirm the executables were created:

```sh
ls ../bin/
```

4. Run tools to verify the CLI:

```sh
./bin/pdb_to_xyzr.exe --help
./bin/Volume.exe -h
./bin/Volume.exe -i 1A01.pdb --exclude-ions --exclude-water -p 1.5 -g 0.5
```

See [QUICKSTART.md](QUICKSTART.md) for a walkthrough.
