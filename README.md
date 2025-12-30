# Molecular Volume and Solvent Analysis Tools

This repository provides a set of command-line tools for analyzing molecular structures, focusing on extracting channels, cavities, calculating solvent-excluded volumes, and analyzing ribosomal tunnels. These tools are invaluable for researchers in structural biology and molecular biophysics.

## Documentation

- [docs/AUTHORS.md](docs/AUTHORS.md)
- [docs/CHANGELOG.md](docs/CHANGELOG.md)
- [docs/CODE_ARCHITECTURE.md](docs/CODE_ARCHITECTURE.md)
- [docs/INSTALL.md](docs/INSTALL.md)
- [docs/FILE_STRUCTURE.md](docs/FILE_STRUCTURE.md)
- [docs/MARKDOWN_STYLE.md](docs/MARKDOWN_STYLE.md)
- [docs/QUICKSTART.md](docs/QUICKSTART.md)
- [docs/RELEASE_HISTORY.md](docs/RELEASE_HISTORY.md)
- [docs/REPO_STYLE.md](docs/REPO_STYLE.md)
- [docs/TODO.md](docs/TODO.md)
- [docs/UTILS.md](docs/UTILS.md)

## Citation

If you use these tools in your research, please cite:

- **Neil R. Voss, Mark Gerstein.**  
  *3V: cavity, channel, and cleft volume calculator and extractor.*  
  Nucleic Acids Res. 2010 Jul;38(Web Server issue):W555-62.  
  - DOI: [10.1093/nar/gkq395](https://doi.org/10.1093/nar/gkq395)  
  - PubMed: [20478824](https://pubmed.ncbi.nlm.nih.gov/20478824)  
  - PubMed Central: [PMC2896178](https://www.ncbi.nlm.nih.gov/pmc/articles/PMC2896178/)  
  - Download Pre-print PDF: [3V_cavity_channel_and_cleft_volume_calculator_and_extractor.pdf](https://github.com/vosslab/vossvolvox/raw/master/publications/3V_cavity_channel_and_cleft_volume_calculator_and_extractor.pdf)

### Secondary References

- **Voss NR.**  
  *Geometric Studies of RNA and Ribosomes, and Ribosome Crystallization.*  
  Doctoral Dissertation. Yale University; November 2006. Dissertation Director: Prof. Peter B. Moore.  
  - Download PDF: [NR_Voss-thesis-Geometric_Studies_of_RNA_and_Ribosomes.pdf](https://github.com/vosslab/vossvolvox/raw/master/publications/NR_Voss-thesis-Geometric_Studies_of_RNA_and_Ribosomes.pdf)

- **Voss NR, et al.**  
  *The Geometry of the Ribosomal Polypeptide Exit Tunnel.*  
  J Mol Biol. 2006 Jul 21;360(4):893-906.  
  - DOI: [10.1016/j.jmb.2006.05.023](http://dx.doi.org/10.1016/j.jmb.2006.05.023)  
  - PubMed: [16784753](https://pubmed.ncbi.nlm.nih.gov/16784753)  
  - Download Pre-print PDF: [Geometry_of_the_Ribosomal_Polypeptide_Exit_Tunnel.pdf](https://github.com/vosslab/vossvolvox/raw/master/publications/Geometry_of_the_Ribosomal_Polypeptide_Exit_Tunnel.pdf)

- Contact: Mark Gerstein &lt;mark.gerstein@yale.edu&gt; or Neil Voss &lt;vossman77@yahoo.com&gt;

## Installation

To install the Voss Volume Voxelator package, follow these steps:

1. **Clone or Download the Repository**
   - If you have not already, clone or download the repository to your local machine:
     ```bash
     git clone https://github.com/vosslab/vossvolvox.git
     ```

2. **Navigate to the Source Code Directory**
   - Open a terminal and navigate to the directory containing the source code. For example:
     ```bash
     cd /path/to/vossvolvox/src
     ```

3. **Compile the Package**
   - In the terminal, type the following command to compile the package:
     ```bash
     make
     ```
   - This will build the necessary executables from the source code.

4. **Check the Output**
   - After compilation, you should see the output executables in the `/path/to/vossvolvox/bin/` directory. You can list the files in the bin folder:
     ```bash
     ls /path/to/vossvolvox/bin/
     ```

5. **Run the Programs**
   - To test the programs, you can run any of the compiled executables. For example:
     ```bash
     ./bin/Volume.exe -h
     ```
   - This will display the help information for the Volume.exe program.

### Optional Gemmi Support

- The native `pdb_to_xyzr.exe` uses the [Gemmi](https://gemmi.readthedocs.io/) reader when the headers are available, which enables direct parsing of PDB, mmCIF, PDBML, and their `.gz` variants without intermediate scripts.
- All converters now accept `--exclude-*` flags (`ions`, `ligands`, `hetatm`, `water`, `nucleic-acids`, `amino-acids`) so you can trim structures without ad-hoc `grep` pipelines.
- Every grid executable (Volume.exe, Solvent.exe, etc.) now auto-detects `.xyzr`, `.pdb`, `.mmcif`, and `.pdbml` inputs and exposes the same `-H/--hydrogens` and `--exclude-*` switches as `pdb_to_xyzr.exe`, so you can point tools directly at raw structural files.
- Every executable shares a unified C++17 CLI helper, so `./bin/<tool> --help` now prints consistent usage information.
- Install Gemmi via your preferred package manager (e.g., `pip install gemmi` or `brew install gemmi`) before running `make`. The build automatically detects the headers via `__has_include`.
- **Security updates:** because Gemmi is header-only, periodically update the installed package (`pip install --upgrade gemmi` or `brew upgrade gemmi`) and rebuild `pdb_to_xyzr.exe` to pick up upstream fixes.

For more details, see [docs/QUICKSTART.md](docs/QUICKSTART.md).

### Testing and Regression Harness

- Run `python3 test/test_volumes.py` to regenerate filtered XYZR files, execute `Volume.exe`, `Volume-legacy.exe`, and the archival `Volume-1.0.exe`, and store the resulting PDB/MRC pairs under `test/volume_results/<PDBID>/`.
- The script accepts `--pdb-id`, `--probe-radius`, and `--grid-spacing` so you can sweep different structures and parameters. Pass `--skip-build` once the binaries are up to date.
- We now ship two convenience targets: `make volume_original` produces `bin/Volume-legacy.exe` (stand-alone legacy build) and `make volume_reference` rebuilds the historical `bin/Volume-1.0.exe` so it picks up the latest safety fixes.
- Use the output tables from `test_volumes.py` to confirm volumes, surfaces, line counts, and sanitized MD5 sums remain stable after changes.
- Run `python3 test/test_suite.py` to execute the YAML-driven regression suite defined in `test/test_suite.yml`. The runner auto-resolves binaries from `bin/`, reuses cached inputs, prints color-coded pass/fail lines with per-test runtimes, and surfaces MD5/volume mismatches immediately. Install [PyYAML](https://pyyaml.org/) (`pip install pyyaml`) before running the suite.
- The suite exercises both traditional `.xyzr` pipelines and the new direct-PDB path (`volume_pdb_input_2LYZ`), ensuring the embedded converter stays in lockstep with the standalone script.
- Use `python3 test/test_suite.py --list` to see all scenarios; add a YAML block whenever you fix a bug or add a feature so coverage grows with the codebase.

### What's new in 2.0.0-beta5

- **In-memory XYZR buffers:** `pdb_io.*` now exposes a converter library that every executable can call. Programs no longer need temporary `.xyzr` files just to populate grids.
- **Direct structure input:** Volume.exe, Solvent.exe, and the other grid-based tools now accept `.pdb`, `.mmcif`, `.pdbml`, or `.xyzr` transparently and expose the same `-H/--hydrogens` and `--exclude-*` filtering flags as `pdb_to_xyzr`.
- **Cleaner CLIs:** `argument_helper` gained reusable option bundles (`add_output_file_options`, `add_xyzr_filter_flags`), giving each executable the same help layout and spacing.
- **Regression coverage:** `test/test_suite.yml` now includes `volume_pdb_input_2LYZ` so the embedded converter stays in sync with the standalone script even when reading raw PDB/mmCIF files.

#### YAML test-suite reference

Each test block in `test/test_suite.yml` contains the following keys:

- `name`, `description`: human-friendly identifiers that show up in the CLI log.
- `workdir`: directory (relative to `test/`) where artifacts and downloads will be staged. Results for different programs can share the same workspace to avoid redundant downloads, and cached files are reused across runs.
- `prerequisites`: ordered actions the harness performs before invoking the binary. Available actions today are:
  - `download_pdb`: fetches `<pdb_id>.pdb` (gz cache supported) into an arbitrary `dest`, preferring existing copies under `test/volume_results/<PDBID>/` before contacting the RCSB servers.
  - `convert_xyzr`: calls `bin/pdb_to_xyzr.exe` with the provided `filters` (e.g., `["--exclude-ions", "--exclude-water"]`). Use `overwrite: true` to force regeneration.
  - `ensure_dir`: creates a directory tree, handy for deposit locations.
  - `remove`: deletes a path if it exists (useful when you want programs to recreate outputs).
- `program`: just the executable name (for example `Volume.exe` or `Channel.exe`). The runner finds it under `<repo>/bin/` automatically, so there is no need to hard-code relative paths like `../../bin/...`.
- `args`: list of CLI flags exactly as you would pass them at the shell. Keep each flag/value pair on the same item (e.g., `-i 2LYZ-modern-suite.xyzr`) so order is preserved.
- `expect`: assertions the harness enforces after the command succeeds. Common fields are:
  - `summary`: expected numeric values parsed from the program’s summary line (`volume`, `surface`, `atoms`, etc.).
  - `pdb`: path plus optional `lines` and `md5` (sanitized by stripping `REMARK Date`). Leave off `lines`/`md5` if the file is optional.
  - `stdout_contains`: substring that must appear in stdout.

You can provide an alternate YAML via `python3 test/test_suite.py --config custom.yml`, and the script prints timing totals and highlights failures in red to speed up debugging.

## Quickstart

To get started with the Voss Volume Voxelator tools, follow these steps:

1. **Get the PDB of Hemoglobin (1A01)**
   ```
   wget -c "http://www.rcsb.org/pdb/cgi/export.cgi/1A01.pdb.gz?format=PDB&pdbId=1A01&compression=gz" -O 1A01.pdb.gz
   gunzip 1A01.pdb.gz
   ```

2. **Filter Out Solvent/Ions While Converting to XYZR**
   Use the native converter with fine-grained exclude flags (ions, water, ligands, etc.):
   ```
   bin/pdb_to_xyzr.exe --exclude-ions --exclude-water 1A01.pdb > 1a01-filtered.xyzr
   ```
   - Additional switches include `--exclude-ligands`, `--exclude-hetatm`, `--exclude-amino-acids`, and `--exclude-nucleic-acids`, letting you tailor the structure to your workflow without external `grep` passes.
   - The legacy shell (`xyzr/pdb_to_xyzr.sh`) and Python (`xyzr/pdb_to_xyzr.py`) scripts are still available if you need to compare outputs.
   - Prefer to skip the intermediate file? `Volume.exe` (and friends) now accept raw PDB/mmCIF/PDBML directly with the same filters:  
     `bin/Volume.exe -i 1A01.pdb --exclude-ions --exclude-water -p 1.5 -g 0.5`.

3. **Compile the Program**
   Navigate to the source directory and build the `vol` program:
   ```
   cd src
   make vol
   cd ..
   ```

4. **Calculate Solvent Excluded Volume**
   Run the Volume.exe tool with the desired input and parameters:
   ```
   bin/Volume.exe -i 1a01-filtered.xyzr -p 1.5 -g 0.5
   ```

5. **Output to PDB (Visualize with RasMol or another molecular viewer)**
   ```
   bin/Volume.exe -i 1a01-filtered.xyzr -p 1.5 -g 0.5 -o 1a01-excluded.pdb
   ```

6. **Output to MRC format for visualization (e.g., in UCSF Chimera)**
   ```
   bin/Volume.exe -i 1a01-filtered.xyzr -p 1.5 -g 0.5 -m 1a01-excluded.mrc
   ```

8. **View MRC file in UCSF Chimera**
   If you have UCSF Chimera installed, use the following command:
   ```
   chimera 1a01-excluded.mrc
   ```
   Alternatively, download Chimera from: http://www.cgl.ucsf.edu/chimera/

## Utility Function Reference

- The helper routines behind the grids, volumes, and exclusions live together in `src/utils-main.cpp`.
- A grouped catalog of every helper, including inputs/outputs and responsibilities, now lives in
  [docs/UTILS.md](docs/UTILS.md) for quick lookup before refactoring or porting.

## Program Descriptions

### Primary Programs

- **Volume.exe**
  - **Description**: Calculates the solvent-excluded volume and surface area for any specified probe radius.
  - **Input**: Structure file, grid spacing, and probe radius.
  - **Output**: PDB, EZD, and MRC files containing volume and surface data.

- **Channel.exe**
  - **Description**: Identifies and extracts a specific solvent channel from a structure.
  - **Input**: Structure file, probe radius, grid spacing, and coordinates for channel location.
  - **Output**: PDB, EZD, and MRC files containing the specified channel.

### Simple Programs

- **Cavities.exe**
  - **Description**: Extracts cavities within a molecular structure based on a specified probe radius.
  - **Input**: Molecular structure file, grid spacing, probe radius, and output file paths.
  - **Output**: PDB, EZD, and MRC files containing identified cavities.

- **FsvCalc.exe**
  - **Description**: Calculates the fractional solvent volume for a structure based on given probe radii.
  - **Input**: Structure file, probe radius, and grid spacing.
  - **Output**: Solvent volume data.

- **Solvent.exe**
  - **Description**: Extracts all solvent areas in a molecular structure above a specified cutoff radius.
  - **Input**: Structure file, probe radius, grid spacing, and output file paths.
  - **Output**: PDB, EZD, and MRC files with solvent regions.

- **Tunnel.exe**
  - **Description**: Identifies and extracts the ribosomal exit tunnel from the *Haloarcula marismortui* structure.
  - **Input**: Structure file, grid spacing, probe radius, and output file paths.
  - **Output**: PDB, EZD, and MRC files representing the exit tunnel.

- **VDW.exe**
  - **Description**: Calculates the van der Waals (VDW) volume and surface area of a structure.
  - **Input**: Structure file, grid spacing, and probe radius.
  - **Output**: PDB, EZD, and MRC files with VDW volume data.

### Looping Programs

- **AllChannel.exe**
  - **Description**: Extracts all solvent channels from a structure that exceed a specified cutoff.
  - **Input**: Structure file, probe radius, grid spacing, minimum volume, and channel count.
  - **Output**: Data for channels meeting the cutoff criteria.

- **AllChannelExc.exe**
  - **Description**: Similar to AllChannel.exe but includes additional cutoff and percentage criteria.
  - **Input**: Structure file, probe radius, grid spacing, minimum volume, and percentage.
  - **Output**: Data for channels meeting the cutoff and percentage criteria.
  
### Experimental Programs
  
- **FracDim.exe**
  - **Description**: Calculates the fractal dimension of the molecular structure based on different grid resolutions.
  - **Input**: Structure file, probe radius, and grid spacing parameters.
  - **Output**: Fractal dimension values.

- **TwoVol.exe**
  - **Description**: Produces two solvent-excluded volumes on the same grid for comparison or 3D printing.
  - **Input**: Two structure files, grid spacing, two probe radii, and merge/fill options.
  - **Output**: MRC files representing the calculated volumes.

- **VolumeNoCav.exe**
  - **Description**: Similar to Volume.exe but excludes cavities from the solvent-excluded volume calculation.
  - **Input**: Structure file, grid spacing, and probe radius.
  - **Output**: PDB, EZD, and MRC files excluding cavity volumes.

### Additional Notes

Each program is optimized for specific structural analysis tasks, ranging from general solvent volume calculations to detailed ribosomal tunnel analysis. Detailed usage instructions for each program can be accessed via the `-h` flag.

These tools offer flexibility for in-depth analysis and visualization, suitable for molecular modeling and bioinformatics research.
