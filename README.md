# Molecular volume and solvent analysis tools

C++ command-line tools for structural-biology researchers who need reproducible molecular-volume, cavity, channel, and solvent analyses from PDB, mmCIF, PDBML, or XYZR structure data.

## From one structure to useful geometry

The tools turn atomic coordinates into grid-based measurements and output artifacts that
can be inspected or carried into a structure-visualization workflow. They are useful when
a protein-structure question needs a scriptable answer rather than a manual measurement.

- Calculate molecular volume and surface area at a chosen probe radius.
- Find cavities, channels, solvent-accessible regions, and ribosome exit tunnels.
- Accept prepared XYZR input or read PDB, mmCIF, and PDBML structures through the shared
  command-line interface.
- Filter ions, ligands, water, amino acids, or nucleic acids before analysis.
- Write PDB surface points and density maps in EZD, MRC2014, or CCP4-compatible formats
  when the selected tool supports them.

## What a volume run produces

`Volume.exe` reports the calculated volume and surface metrics to the terminal. With
`-o`, it also writes accessible surface points as a PDB-style file. With `-m` or `-c`,
it writes a density map for a compatible viewer or downstream workflow.

<!-- screenshots:begin (managed by screenshot-docs) -->
<!-- screenshots:end -->

## Quick start

You need a C++17 compiler and GNU Make. Python 3.12 is required only for the repository's
Python utilities and tests. Build the primary volume executable from the repository root:

```sh
make -C src
./bin/Volume.exe --help
```

Run a first analysis on a local structure file. Replace `my_structure.pdb` with a PDB
file you already have. This command removes waters and ions, calculates a
solvent-excluded volume with a 1.5 Angstrom probe and 0.5 Angstrom grid, and writes
surface points to `surface.pdb`.

```sh
./bin/Volume.exe -i my_structure.pdb --exclude-ions --exclude-water \
  -p 1.5 -g 0.5 -o surface.pdb
```

The terminal prints the calculation summary; `surface.pdb` contains the generated
surface points. See [docs/INSTALL.md](docs/INSTALL.md) for compiler and optional
Gemmi requirements.

## Convert and analyze XYZR

XYZR is the compact atom-and-radius input format used by the volume tools. Build the
native converter when you want to inspect or reuse this intermediate representation:

```sh
make -C src pdbxyzr
./bin/pdb_to_xyzr.exe -i my_structure.pdb --exclude-ions --exclude-water \
  > filtered.xyzr
./bin/Volume.exe -i filtered.xyzr -p 1.5 -g 0.5 -o surface.pdb
```

The converter writes XYZR records to standard output, so shell redirection makes the
result a reusable input file. This is also the path used by the converter comparison
test. More examples and common options are in [docs/USAGE.md](docs/USAGE.md).

## Choose a map format

For tools that expose map-output flags, select the format expected by the next viewer
or processing step:

| Output | Flag | Placement convention |
| --- | --- | --- |
| MRC2014 density map | `-m output.mrc` | Real-space `ORIGIN`; recommended for exact real-space placement. |
| CCP4-compatible density map | `-c output.ccp4` | Grid-index `NSTART`; useful for viewers that expect CCP4 placement. |
| EZD density map | `-e output.ezd` | Tool-supported density-map output. |

For example, add `-m excluded.mrc` or `-c excluded.ccp4` to a supported volume command.
Use a `.ccp4` or `.map` extension for CCP4 output. The two binary map formats intentionally
use different placement conventions, so choose one deliberately rather than treating them
as interchangeable.

## Tools at a glance

The shared C++17 command-line layer keeps common input, filtering, quiet, and help behavior
consistent across the modern executables in `bin/`.

- `Volume.exe` calculates molecular volume and surface area.
- `Cavities.exe`, `Channel.exe`, `AllChannel.exe`, and `AllChannelExc.exe` analyze
  cavities and channels.
- `Solvent.exe`, `VDW.exe`, and `VolumeNoCav.exe` support related solvent, van der Waals,
  and cavity-filled analyses.
- `Tunnel.exe` targets ribosome exit-tunnel workflows.
- `ProteinRNAVolume.exe`, `TwoVol.exe`, and `FracDim.exe` provide specialized comparisons
  and calculations.

Run `./bin/<tool>.exe --help` before using a specific executable; supported output flags
vary by tool.

## Verification

The repository maintains shell-based integration checks for the two primary workflows:

```sh
./tests/test_volume.sh
./tests/test_pdb_to_xyzr.sh 1A01
```

The tests cache PDB downloads under `tests/pdb_to_xyzr_results/` and report checksums for
converter comparisons. They may download the indicated PDB structure if it is not cached.

## Documentation

- [docs/INSTALL.md](docs/INSTALL.md): compiler, dependency, and build requirements.
- [docs/USAGE.md](docs/USAGE.md): command-line workflows, options, and examples.
- [docs/CODE_ARCHITECTURE.md](docs/CODE_ARCHITECTURE.md): components, shared helpers, and
  data flow.
- [docs/FILE_STRUCTURE.md](docs/FILE_STRUCTURE.md): repository layout and generated artifacts.
- [docs/RELEASE_HISTORY.md](docs/RELEASE_HISTORY.md): released versions and compatibility notes.
- [docs/CHANGELOG.md](docs/CHANGELOG.md): chronological maintenance and behavior record.

## Status and limitations

This is an established research-code toolchain with a modernized C++17 command-line layer.
The legacy sources `src/lib/utils-main-legacy.cpp` and `src/volume-legacy.cpp` remain frozen.
PDB input is the primary documented path; reading mmCIF or PDBML requires a build with Gemmi
headers available. Confirm each executable's help text because map-output and analysis options
are not identical across the suite.

## Citation

If you use these tools in research, cite:

- Neil R. Voss and Mark Gerstein, "3V: cavity, channel and cleft volume calculator and
  extractor," *Nucleic Acids Research* (2010).
  [DOI](https://doi.org/10.1093/nar/gkq395)

Additional references and PDFs are in [publications/](publications/).

## License and contact

The repository is distributed under the
[GNU Lesser General Public License v3.0](LICENSE.LGPL_v3).

For project contact, see [neilvosslab on Bluesky](https://bsky.app/profile/neilvosslab.bsky.social).
