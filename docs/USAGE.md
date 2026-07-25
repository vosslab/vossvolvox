# Usage

Use the current C++ executables in `bin/` to convert structural input to XYZR
records and analyze molecular volumes, channels, cavities, solvent, or tunnels.
Build the suite first with the steps in [INSTALL.md](INSTALL.md).

## Quick start: PDB to volume

Convert a PDB file to XYZR records, filtering ions and water, then calculate a
solvent-excluded volume. The converter writes XYZR records to standard output.

```sh
./bin/pdb_to_xyzr.exe -i 1A01.pdb --exclude-ions --exclude-water > 1a01-filtered.xyzr
./bin/Volume.exe -i 1a01-filtered.xyzr -p 1.5 -g 0.5 -o 1a01-excluded.pdb
```

`Volume.exe` prints its numeric summary to standard output and writes accessible
surface points to the PDB path passed with `-o`.

## Current C++ executables

`make all` builds these current tools:

- `pdb_to_xyzr.exe` converts structural input to XYZR records.
- `Volume.exe`, `VolumeNoCav.exe`, `TwoVol.exe`, and `ProteinRNAVolume.exe`
  calculate volume-related measurements.
- `Cavities.exe`, `Channel.exe`, `AllChannel.exe`, `AllChannelExc.exe`, and
  `Tunnel.exe` analyze cavities, channels, or a ribosome exit tunnel.
- `Solvent.exe`, `VDW.exe`, `FsvCalc.exe`, and `FracDim.exe` provide solvent,
  van der Waals, free-solvent-volume, and fractal-dimension calculations.

Every executable has tool-specific help. For example:

```sh
./bin/Cavities.exe -h
./bin/Volume.exe -h
```

## Shared CLI options

The modern tools use `-i`/`--input` for a structure input. Where the tool
supports grid outputs, `-o` writes accessible surface points as PDB, `-e`
writes EZD density, `-m` writes MRC density, and `-c` writes CCP4 density.

- `-p`/`--probe` sets a probe radius for tools that calculate a probe surface.
- `-g`/`--grid` sets grid spacing for tools that use a grid.
- `-H`/`--hydrogens` selects explicit hydrogen radii instead of united-atom
  radii.
- `--exclude-ions`, `--exclude-ligands`, `--exclude-hetatm`,
  `--exclude-water`, `--exclude-nucleic-acids`, and `--exclude-amino-acids`
  filter structural residues before analysis.
- `-q`/`--quiet` suppresses program banner and citation output; `--debug`
  reports filter, grid-state, and timing diagnostics where supported.

Use `-h` before relying on an option: not every executable exposes every
shared option.

## More examples

Write volume density in MRC and CCP4 forms:

```sh
./bin/Volume.exe -i 1a01-filtered.xyzr -p 1.5 -g 0.5 \
  -m 1a01-excluded.mrc -c 1a01-excluded.ccp4
```

Run a cavity calculation on prepared XYZR input:

```sh
./bin/Cavities.exe -i 1a01-filtered.xyzr -b 10 -s 3 -t 3 -g 0.5 -o cavities.pdb
```

Compare the three converter implementations on a cached or downloaded PDB:

```sh
./tests/test_pdb_to_xyzr.sh 1A01
```

The comparison stages artifacts under `tests/pdb_to_xyzr_results/1A01/` and
reports timings, line counts, checksums, and differences.

## Inputs and outputs

- Current C++ tools accept XYZR and PDB input. A build with Gemmi headers also
  accepts mmCIF and PDBML input.
- `pdb_to_xyzr.exe` writes four-column XYZR records to standard output; redirect
  them to a `.xyzr` file for a later analysis.
- Grid-capable modern tools can write PDB, EZD, MRC, or CCP4 artifacts through
  their corresponding output options. Their numerical summaries remain on
  standard output.

## Alternative and legacy converters

The native `bin/pdb_to_xyzr.exe` is the current converter. The repository also
keeps two alternatives for comparison and compatibility:

```sh
python3 xyzr/pdb_to_xyzr.py --exclude-ions --exclude-water 1A01.pdb > 1a01-filtered.xyzr
xyzr/pdb_to_xyzr.sh 1A01.pdb > 1a01.xyzr
```

The Python converter accepts a positional input (or standard input), can
autodetect PDB, mmCIF, and PDBML files, and supports the filtering options
shown above. The shell converter accepts PDB input only, uses `gawk`, and
supports only `-h` and `-t`; it finds `xyzr/atmtypenumbers` automatically or
uses `PDB_TO_XYZR_TABLE`/`-t` as an override.

`Volume-legacy.exe` is a separate historical compatibility binary, not a
member of the current `make all` suite. Build it only for the classic comparison
harness with `cd src && make volume_original`.

## Known gaps

- TODO: Verify which non-volume tools support every density-output option.
- TODO: Add a documented, offline fixture for the quick-start workflow.
