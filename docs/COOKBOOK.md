# Cookbook

This cookbook collects practical analysis workflows that extend the command overview in
[USAGE.md](USAGE.md). Run the commands from the repository root after building the required
executable in `src`.

## Export maps for viewers

Use this workflow when a structure needs both a surface-point PDB file and a density map for a
viewer with a known placement convention.

```sh
cd src
make vol
cd ..
./bin/Volume.exe -i structure.pdb --exclude-ions --exclude-water \
  -p 1.5 -g 0.5 -o structure-surface.pdb \
  -m structure-volume.mrc -c structure-volume.ccp4
```

- Use `structure-volume.mrc` when the viewer honors the MRC `ORIGIN` field.
- Use `structure-volume.ccp4` (or a `.map` filename) when the viewer expects CCP4 `NSTART`
  placement.
- Retain the PDB output when surface points are useful for checking the grid result.
- See [RELEASE_HISTORY.md](RELEASE_HISTORY.md) for the MRC and CCP4 placement conventions.

## Extract an internal cavity

Use `Cavities.exe` to extract cavities from a prepared structure. The shell, probe, and trim
radii below are the tool's documented example values; adjust them as required for the structure.

```sh
cd src
make cav
cd ..
./bin/Cavities.exe -i structure.pdb --exclude-ions --exclude-water \
  -b 10 -s 3 -t 3 -g 0.5 -o structure-cavities.pdb \
  -m structure-cavities.mrc
```

- `-b` defines the enclosing shell radius.
- `-s` defines the cavity probe radius.
- `-t` trims the shell before the cavity calculation.
- Use the PDB output for surface-point inspection and the MRC output for volumetric viewing.

## Select a solvent channel

Use a seed coordinate to select one solvent channel rather than exporting every connected region.
Coordinates are supplied in the structure coordinate system.

```sh
cd src
make chan
cd ..
./bin/Channel.exe -i structure.pdb --exclude-ions --exclude-water \
  -b 9.0 -s 1.5 -t 4.0 -g 0.5 -x -10 -y 5 -z 0 \
  -o selected-channel.pdb -m selected-channel.mrc
```

- Choose `-x`, `-y`, and `-z` from a coordinate known to lie inside the channel of interest.
- Confirm the selected channel visually before treating its volume as the intended result.
- Run `./bin/Channel.exe --help` to review the defaults before changing the three probe values.

## Produce paired print volumes

Use `TwoVol.exe` when two structures need solvent-excluded volumes on the same grid for a
3D-printing workflow.

```sh
cd src
make twovol
cd ..
./bin/TwoVol.exe -i1 protein.pdb -i2 ligand.pdb \
  --exclude-ions --exclude-water -p1 1.5 -p2 3.0 -g 0.6 \
  -m1 protein-volume.mrc -m2 ligand-volume.mrc
```

- `-m1` and `-m2` name the two MRC outputs.
- `--merge 1` merges the second volume into the first; `--merge 2` performs the reverse merge.
- `--fill 1` fills from the second volume into the first; `--fill 2` performs the reverse fill.
- Leave merge and fill at their default `0` when separate volumes are required.

## Compare RNA and protein maps

Use `ProteinRNAVolume.exe` for RNA and amino-acid inputs that must share one grid. It writes
`rna.mrc` and `amino.mrc` in the current directory, so run it in an empty results directory or
move those files after inspection.

```sh
cd src
make cust
cd ..
mkdir -p results/rna-protein
cd results/rna-protein
../../bin/ProteinRNAVolume.exe -r ../../rna.pdb -a ../../protein.pdb \
  --exclude-ions --exclude-water -p 1.5 -g 0.6
```

- The RNA and amino-acid inputs use the same probe radius and grid spacing.
- Inspect `rna.mrc` and `amino.mrc` together to compare their aligned results.
- Use `./bin/ProteinRNAVolume.exe --help` from the repository root to confirm available filters.
