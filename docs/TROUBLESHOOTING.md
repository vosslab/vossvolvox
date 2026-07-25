# Troubleshooting

Use this guide to diagnose build, input, converter, and regression-test messages from the
command-line tools. Confirm the exact command and message first; the tools print help with
`-h` or `--help`.

## Missing executable

**Symptom:** the shell cannot run a program in `bin`, or the converter comparison reports
`Skipping C++ implementation; executable not found.`

- Build all current programs from `src` with `make all`, or build a single program with its
  Make target.
- Use `make vol` for `../bin/Volume.exe` and `make pdbxyzr` for `../bin/pdb_to_xyzr.exe`.
- The Makefile creates `bin` as a prerequisite of these targets. Confirm the requested executable
  exists before rerunning the command or comparison.

```sh
cd src
make vol
make pdbxyzr
cd ..
./bin/Volume.exe -h
```

See [INSTALL.md](INSTALL.md) for the compiler and build requirements.

## Input and option errors

**Symptom:** a tool reports an unknown option, a missing value, `input file not specified`, or
`unable to load XYZR data`.

- Run the same executable with `-h` to verify its supported option spelling and whether it requires
  a value.
- Supply an input path with `-i <path>` for the single-input tools.
- Confirm that the input path is readable and has the intended data format. XYZR input uses four
  whitespace-separated numeric fields per atom; PDB-family inputs are converted before analysis.
- For `TwoVol.exe`, provide both `--input1` and `--input2`. For `ProteinRNAVolume.exe`, provide
  both `--rna-input` and `--amino-input`.

The shared command-line parser prints the relevant error before returning a nonzero status. See
[USAGE.md](USAGE.md) for common flags and examples.

## mmCIF or PDBML input

**Symptom:** the program reports that the build lacks Gemmi and cannot read a `.cif`, `.mmcif`,
`.xml`, `.pdbml`, or `.pdbxml` file.

- These formats require Gemmi headers at C++ compile time; PDB and XYZR inputs use the built-in
  reader path.
- Install the Gemmi headers, then rebuild the requested target so the header check is evaluated
  again.
- If Gemmi is present but parsing still fails, the converter prints the Gemmi read error and the
  input path. Use that message to check the source file before retrying.

[INSTALL.md](INSTALL.md) records Gemmi as optional generally but required for mmCIF and PDBML.

## Converter diagnostics

**Symptom:** `pdb_to_xyzr` reports that an atom pattern was not found in the embedded
`atmtypenumbers` data.

- The message identifies the input file, residue number, residue, and atom pattern that lacks a
  radius entry.
- Save that complete message with the input structure when reporting or investigating the result;
  it is the direct identifier for the unmatched atom type.
- The converter continues writing an XYZR line using its returned radius value, so review the
  output before treating it as a fully typed conversion.

## Downloaded test input

**Symptom:** `test_pdb_to_xyzr.sh` reports a download, decompression, or input-preparation failure.

- The comparison script downloads `<PDB_ID>.pdb.gz` with `curl` and unpacks it with `gunzip`.
- It reuses a cached PDB at `tests/pdb_to_xyzr_results/<PDB_ID>/<PDB_ID>.pdb`; retain a valid cached
  file to run the comparison without another download.
- Run the script from the repository root, passing the PDB identifier when needed:

```sh
./tests/test_pdb_to_xyzr.sh 1A01
```

The script reports whether a missing Python, shell, or C++ implementation was skipped. A skipped
implementation makes the comparison incomplete rather than supplying its output.

## Regression differences

**Symptom:** `test_volume.sh` reports a volume, surface, line-count, or HETATM MD5 mismatch.

- The test runs `pdb_to_xyzr` with `--exclude-ions --exclude-water`, then runs `Volume.exe` with a
  probe radius of `2.1` and grid spacing of `0.9`.
- Keep the test's printed volume log and the generated `2LYZ-volume.pdb`; the script uses them to
  report the summary, output line count, HETATM line count, and HETATM-only MD5.
- Rebuild `Volume.exe` if the script reports it is absent. The test itself performs `make vol` when
  that executable is not present.

Run the regression from `tests` through the documented root command:

```sh
./tests/test_volume.sh
```

Its expected values are a regression baseline, so a mismatch is a test failure that needs the
reported values and input/output artifacts for comparison.
