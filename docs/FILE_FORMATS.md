# File formats

This reference describes the structure inputs and grid exports accepted or written by the
current C++ tools. Use [USAGE.md](USAGE.md) for commands and [CODE_ARCHITECTURE.md](CODE_ARCHITECTURE.md)
for the processing pipeline.

## Structure inputs

Modern tools accept `-i` / `--input`. The shared reader converts every accepted structure into
an in-memory XYZR atom list before grid calculations. The exact set of inputs depends on how the
binary was built.

| Input | Recognition and requirements | Interpretation |
| --- | --- | --- |
| XYZR (`.xyzr` or `.xyz`) | Read directly by the shared loader. | One whitespace-separated atom record: `x y z radius`. The reader accepts the first four numeric fields and skips blank or unparseable lines. |
| PDB | The built-in reader handles PDB-style `ATOM` and `HETATM` records. | Coordinates and atom identity are converted to a radius using the embedded atom-type table. Filtering flags control which records reach the grid. |
| mmCIF (`.cif`, `.mmcif`) | Requires Gemmi headers when the binary is built. | Read through Gemmi, then converted to the same XYZR atom list. |
| PDBML (`.xml`, `.pdbml`, `.pdbxml`) | Requires Gemmi headers when the binary is built. | Read through Gemmi, then converted to the same XYZR atom list. |

The loader removes a trailing `.gz` only while classifying an extension. This does not by itself
document decompression support; provide a readable input for the parser selected by the build.
When Gemmi is unavailable, mmCIF and PDBML inputs fail with a rebuild instruction rather than
falling back to the PDB reader. See [pdb_io.cpp](../src/lib/pdb_io.cpp) and
[argument_helper.hpp](../src/lib/argument_helper.hpp).

### XYZR output

`pdb_to_xyzr.exe` writes XYZR records to standard output. Each retained atom is formatted as four
fields in this order: x, y, z, and radius. Coordinates use three decimal places; radius uses two.
Redirect standard output to create a reusable XYZR file:

```sh
./bin/pdb_to_xyzr.exe -i input.pdb --exclude-ions --exclude-water > filtered.xyzr
```

The converter can also read PDB text from standard input. File-based mmCIF and PDBML support is
the Gemmi path described above. The implementation is in [pdb_io.cpp](../src/lib/pdb_io.cpp) and
[pdb_to_xyzr.cpp](../src/pdb_to_xyzr.cpp).

## Grid exports

Tools that register the shared output options accept the following paths. A tool's `--help` is the
authoritative list for that executable; legacy and specialized tools need not expose every output.

| Flag | File | Contents |
| --- | --- | --- |
| `-o`, `--pdb-output` | PDB text | Surface grid points only, emitted as `HETATM` records. The header includes grid settings, a creation timestamp, and sorted command-line arguments. |
| `-e`, `--ezd-output` | EZD text | A factor-two binned grid. It has `EZD_MAP`, `CELL`, `ORIGIN`, `EXTENT`, `GRID`, `SCALE`, `MAP`, and `END` records; each map value is `0.0` or `1.0`. |
| `-m`, `--mrc-output` | MRC2014 binary | A one-byte grid with MRC mode 0. `NSTART` is zeroed and the real-space voxel origin is stored in `ORIGIN` in Angstroms. |
| `-c`, `--ccp4-output` | CCP4-compatible binary | A one-byte grid with the same shared header layout. `ORIGIN` is zeroed and placement is stored in `NSTART` grid indices. Prefer a `.ccp4` or `.map` filename. |

MRC and CCP4 are deliberately different placement conventions, even though both use the shared
header structure. Use MRC when the viewer honors `ORIGIN` for exact real-space placement. Use the
CCP4 export for software that expects placement through `NSTART`. Both writers mark their chosen
convention in the first header label, use axis order X/Y/Z, and write a single-volume header.

The MRC and CCP4 writers do not create a file for an empty grid. Some tools select a trimmed grid
writer; it retains the same placement convention while reducing the output bounds. See
[utils-output.cpp](../src/lib/utils-output.cpp), [utils-mrc.cpp](../src/lib/utils-mrc.cpp),
[utils-ccp4.cpp](../src/lib/utils-ccp4.cpp), and [utils-mrc-header.hpp](../src/lib/utils-mrc-header.hpp).

## Practical checks

- Give XYZR files the `.xyzr` extension so the shared loader selects direct XYZR parsing.
- Build with Gemmi before supplying mmCIF or PDBML to a C++ executable.
- Keep the MRC/CCP4 placement convention with the output file; changing only its extension does
  not change header semantics.
- Inspect `./bin/<tool>.exe --help` before scripting an export because output wiring is
  executable-specific.
