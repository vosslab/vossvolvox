# Molecular Volume and Solvent Analysis Tools

This repository provides a set of command-line tools for analyzing molecular structures, focusing on extracting channels, cavities, calculating solvent-excluded volumes, and analyzing ribosomal tunnels. These tools are invaluable for researchers in structural biology and molecular biophysics.

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
- Every executable shares a unified C++17 CLI helper, so `./bin/<tool> --help` now prints consistent usage information.
- Install Gemmi via your preferred package manager (e.g., `pip install gemmi` or `brew install gemmi`) before running `make`. The build automatically detects the headers via `__has_include`.
- **Security updates:** because Gemmi is header-only, periodically update the installed package (`pip install --upgrade gemmi` or `brew upgrade gemmi`) and rebuild `pdb_to_xyzr.exe` to pick up upstream fixes.

For more details, see the `QUICKSTART` section below.

### Testing and Regression Harness

- Run `python3 test/test_volumes.py` to regenerate filtered XYZR files, execute `Volume.exe`, `Volume-legacy.exe`, and the archival `Volume-1.0.exe`, and store the resulting PDB/MRC pairs under `test/volume_results/<PDBID>/`.
- The script accepts `--pdb-id`, `--probe-radius`, and `--grid-spacing` so you can sweep different structures and parameters. Pass `--skip-build` once the binaries are up to date.
- We now ship two convenience targets: `make volume_original` produces `bin/Volume-legacy.exe` (stand-alone legacy build) and `make volume_reference` rebuilds the historical `bin/Volume-1.0.exe` so it picks up the latest safety fixes.
- Use the output tables from `test_volumes.py` to confirm volumes, surfaces, line counts, and sanitized MD5 sums remain stable after changes.

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
