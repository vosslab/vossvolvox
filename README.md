# Molecular volume and solvent analysis tools

This repository provides C++17 command-line tools for analyzing molecular
structures by computing volumes, channels, cavities, and tunnel geometry from
PDB, mmCIF, PDBML, or XYZR inputs. It is intended for researchers in structural
biology and molecular biophysics who need reproducible grid-based analyses.

## Documentation

### Getting started
- [docs/INSTALL.md](docs/INSTALL.md): setup steps and build requirements.
- [docs/USAGE.md](docs/USAGE.md): how to run the executables and common flags.
- [docs/QUICKSTART.md](docs/QUICKSTART.md): end-to-end workflow walkthrough.

### Reference
- [docs/CODE_ARCHITECTURE.md](docs/CODE_ARCHITECTURE.md): system design and data flow.
- [docs/FILE_STRUCTURE.md](docs/FILE_STRUCTURE.md): directory map and generated assets.
- [docs/UTILS.md](docs/UTILS.md): shared C++ helper reference.

### Project records
- [docs/CHANGELOG.md](docs/CHANGELOG.md): chronological record of changes.
- [docs/RELEASE_HISTORY.md](docs/RELEASE_HISTORY.md): released versions and highlights.
- [docs/REPO_STYLE.md](docs/REPO_STYLE.md): repo organization and naming conventions.
- [docs/PYTHON_STYLE.md](docs/PYTHON_STYLE.md): Python conventions for the repo.
- [docs/MARKDOWN_STYLE.md](docs/MARKDOWN_STYLE.md): documentation formatting rules.

## Quick start

Build the executables:

```sh
cd src
make
cd ..
```

Confirm the CLI is available:

```sh
./bin/Volume.exe -h
```

See [docs/USAGE.md](docs/USAGE.md) for workflows and examples.

## Testing

- `./tests/test_volume.sh` runs the volume regression check.
- `./tests/test_pdb_to_xyzr.sh 1A01` compares converter outputs.

## Citation

If you use these tools in your research, cite:

- Neil R. Voss, Mark Gerstein. 3V: cavity, channel, and cleft volume calculator
  and extractor. Nucleic Acids Res. 2010.
  [DOI](https://doi.org/10.1093/nar/gkq395). See [publications/](publications/)
  for PDFs.
- Additional references are listed in [publications/](publications/).

## Contact

- Neil Voss on
  [neilvosslab on Bluesky](https://bsky.app/profile/neilvosslab.bsky.social).
