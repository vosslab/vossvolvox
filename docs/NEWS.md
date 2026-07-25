# News

Curated highlights from current vossvolvox-cpp releases.

## v26.07 - 2026-07-25

### Highlights

- Modern tools read PDB and XYZR directly, with mmCIF and PDBML support when
  built with Gemmi.
- A native converter and shared filtering flags streamline structure-to-XYZR
  workflows.
- MRC2014 and CCP4 map exports provide explicit `ORIGIN` and `NSTART` placement
  choices for structure viewers.
- The C++17 toolchain now shares consistent command-line behavior and expanded
  regression and user documentation.

### Upgrade notes

- Replace `Custom.exe` with `ProteinRNAVolume.exe` in scripts.
- Use `make all` to build the complete modern suite; plain `make` builds
  `Volume.exe`.
- Use CCP4 output for workflows that require `NSTART` placement; MRC output now
  follows MRC2014 real-space `ORIGIN` conventions.
