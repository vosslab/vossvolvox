# Molecular Volume and Solvent Analysis Tools

This repository provides a set of command-line tools for analyzing molecular structures, focusing on extracting channels, cavities, calculating solvent-excluded volumes, and analyzing ribosomal tunnels. These tools are invaluable for researchers in structural biology and molecular biophysics.

## Documentation

- [docs/AUTHORS.md](docs/AUTHORS.md): maintainers and contributor credits.
- [docs/CHANGELOG.md](docs/CHANGELOG.md): chronological record of changes.
- [docs/CODE_ARCHITECTURE.md](docs/CODE_ARCHITECTURE.md): high-level system design and data flow.
- [docs/INSTALL.md](docs/INSTALL.md): setup steps and build requirements.
- [docs/FILE_STRUCTURE.md](docs/FILE_STRUCTURE.md): directory map and generated assets.
- [docs/MARKDOWN_STYLE.md](docs/MARKDOWN_STYLE.md): documentation formatting rules.
- [docs/QUICKSTART.md](docs/QUICKSTART.md): end-to-end usage walkthrough.
- [docs/RELEASE_HISTORY.md](docs/RELEASE_HISTORY.md): released versions and highlights.
- [docs/REPO_STYLE.md](docs/REPO_STYLE.md): repo organization and naming conventions.
- [docs/TODO.md](docs/TODO.md): backlog items and legacy work notes.
- [docs/UTILS.md](docs/UTILS.md): reference for `src/lib/utils-main.cpp` helpers.

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

## Getting started

- Install and build: [docs/INSTALL.md](docs/INSTALL.md)
- Quickstart walkthrough: [docs/QUICKSTART.md](docs/QUICKSTART.md)
- Command help: run `./bin/<tool>.exe --help`

## Testing

- Regression harnesses live in `tests/` (see `tests/test_suite.py` and `tests/test_volumes.py`).
- YAML scenarios are defined in `tests/test_suite.yml`.

## Release notes

- Release highlights: [docs/RELEASE_HISTORY.md](docs/RELEASE_HISTORY.md)
- Detailed change log: [docs/CHANGELOG.md](docs/CHANGELOG.md)

## Utilities

- Helper catalog and grid utilities: [docs/UTILS.md](docs/UTILS.md)

## Tools

Common executables: `Volume.exe`, `Channel.exe`, `Cavities.exe`, `Solvent.exe`,
`Tunnel.exe`, `VDW.exe`, `VolumeNoCav.exe`, `AllChannel.exe`, `AllChannelExc.exe`,
`FsvCalc.exe`, `FracDim.exe`, `TwoVol.exe`, `Custom.exe`.
