# Frequently asked questions

This FAQ clarifies input handling, structure filtering, and volume-map output
choices for users of the command-line tools.

## Can I skip XYZR?

Yes. `Volume.exe` can load a structure file directly, apply the selected
filter options, and construct its in-memory XYZR representation before volume
calculation. Use a saved XYZR file when you want to inspect, reuse, or compare
the exact sphere records used by a calculation.

PDB input works without the optional Gemmi dependency. mmCIF, PDBML, and
PDBXML inputs require a build with Gemmi headers. A build without Gemmi reports
an error for those formats and must be rebuilt after Gemmi is available. See
[INSTALL.md](INSTALL.md) and [USAGE.md](USAGE.md) for the supported input list
and build requirements.

## What do structure filters change?

The filters decide which residues contribute atomic spheres when a structure
file is converted or loaded directly. They do not modify an existing XYZR
file.

- `--exclude-ions` drops residues classified as ions.
- `--exclude-water` drops water molecules.
- `--exclude-ligands` drops non-polymer ligands.
- `--exclude-hetatm` drops residues made only from `HETATM` records.
- `--exclude-nucleic-acids` and `--exclude-amino-acids` restrict the selected
  polymer class.

Use the same filter flags whenever results must describe the same molecular
selection. The minimal PDB-to-XYZR workflow is shown in [USAGE.md](USAGE.md).

## Should I write MRC or CCP4?

Choose the map format based on the placement convention expected by the viewer.
MRC output stores real-space placement in the header `ORIGIN` fields and is the
recommended format when that exact placement is required. CCP4 output stores
placement through `NSTART` with `ORIGIN` zeroed; use a `.ccp4` or `.map`
extension for viewers that rely on `NSTART`.

The formats intentionally use different placement conventions. If a map appears
shifted, first verify that the viewer is reading it as the format you wrote. The
[CHANGELOG.md](CHANGELOG.md) records the compatibility change, and
[pymol-deep-research-report.md](pymol-deep-research-report.md) provides its
technical background.

## Why is an output option unavailable?

Output support is tool-specific. The common interface uses `-o` for PDB-style
surface output, `-m` for MRC, and `-c` for CCP4 when a program exposes those
outputs, but an individual executable can omit or ignore an output type. Check
that executable's `--help` before scripting an output path. [USAGE.md](USAGE.md)
lists the common flags and identifies output support as conditional.
