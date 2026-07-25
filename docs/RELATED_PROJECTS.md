# Related projects

This guide maps the external projects, standards, and prior work that have a direct
relationship to this repository's structure conversion and volumetric-map workflows.

## Confirmed related projects

### 3V: cavity, channel and cleft volume calculator and extractor

- Relationship: prior art and inspiration
- Link: https://doi.org/10.1093/nar/gkq395
- Evidence: The repository README asks users to cite this 2010 3V publication, and
  the implementation provides the volume, cavity, channel, and tunnel analyses it describes.

### Gemmi

- Relationship: optional dependency
- Link: https://gemmi.readthedocs.io/en/stable/
- Evidence: `src/lib/pdb_io.hpp` conditionally detects Gemmi headers, and
  `src/lib/pdb_io.cpp` uses its mmCIF, PDB, and structure headers for supported inputs.
- Notes: Gemmi documents C++ support for macromolecular PDB and PDBx/mmCIF models.

### Open-source PyMOL

- Relationship: optional integration target
- Link: https://github.com/schrodinger/pymol-open-source
- Evidence: `docs/pymol-deep-research-report.md` evaluates how PyMOL loads this
  repository's MRC and CCP4 outputs, including their distinct origin conventions.
- Notes: Use MRC or CCP4 output semantics that match the viewer-loading workflow.

### CCP4 map library

- Relationship: optional integration target
- Link: https://www.ccp4.ac.uk/html/maplib.html
- Evidence: `src/lib/utils-ccp4.cpp` writes CCP4 output, and the CCP4 map-library
  documentation defines the header and placement conventions that output follows.

### MRC2014 file format

- Relationship: optional integration target
- Link: https://www.ccpem.ac.uk/downloads/other/EPU_MRC2014_File_Image_Format_Specification_-_306687.pdf
- Evidence: `src/lib/utils-mrc.cpp` labels its output as MRC2014 and writes placement
  through `ORIGIN` with zeroed `NSTART` values.

## Evidence notes

The relationships above are grounded in the repository's citation, conditional Gemmi
headers, MRC and CCP4 writers, and the PyMOL compatibility investigation. External
links point to the cited paper or the primary project and format documentation.
