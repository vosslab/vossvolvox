# Usage

Use the executables in [bin/](bin/) to convert structure files into XYZR data
and run volume, channel, cavity, or tunnel analyses.

## Quick start
- Convert a PDB file to XYZR:

```sh
./bin/pdb_to_xyzr.exe --exclude-ions --exclude-water -i 2LYZ.pdb > 2LYZ-filtered.xyzr
```

- Compute volume from the XYZR file:

```sh
./bin/Volume.exe -i 2LYZ-filtered.xyzr -p 2.1 -g 0.9 -o 2LYZ-volume.pdb
```

## CLI
- Executables live in [bin/](bin/) and expose help via `-h` (for example,
  `./bin/Volume.exe -h`).
- Common flags used across tools:
  - `-i <path>` for input files.
  - `-o <path>` for output PDB files when supported.
  - `-p <probe>` for probe radius.
  - `-g <spacing>` for grid spacing.
  - `--exclude-ions` and `--exclude-water` for converter filtering.
  - `-q` to reduce banner output.

## Examples
- Run cavities on a prepared XYZR file:

```sh
./bin/Cavities.exe -i 1a01.xyzr -b 10 -s 3 -t 3 -g 0.5 -o cavities.pdb
```

- Compute a fractal dimension sweep:

```sh
./bin/FracDim.exe -i sample.xyzr -p 1.5 -g1 0.4 -g2 0.8 -gn 8
```

## Inputs and outputs
- Inputs: `*.xyzr` or structure files (`*.pdb`, `*.cif`, `*.mmcif`, `*.pdbml`,
  `*.pdbxml`) when Gemmi support is compiled in.
- Outputs: PDB-style outputs via `-o` plus tool-specific summaries printed to
  stdout.

## Known gaps
- TODO: Confirm which tools emit EZD or MRC outputs and document flags.
- TODO: Confirm whether a dry-run or no-write mode exists across executables.
