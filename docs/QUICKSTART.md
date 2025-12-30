# Quickstart

Run an end-to-end example from PDB input to volume outputs. Commands assume you are
at the repository root.

## 1. Download the example PDB (1A01)

```sh
wget -c "http://www.rcsb.org/pdb/cgi/export.cgi/1A01.pdb.gz?format=PDB&pdbId=1A01&compression=gz" -O 1A01.pdb.gz
gunzip 1A01.pdb.gz
```

## 2. Convert to XYZR (native converter)

```sh
./bin/pdb_to_xyzr.exe --exclude-ions --exclude-water 1A01.pdb > 1a01-filtered.xyzr
```

Alternative: run the Python converter.

```sh
python3 xyzr/pdb_to_xyzr.py --exclude-ions --exclude-water 1A01.pdb > 1a01-filtered.xyzr
```

Alternative: skip XYZR and let Volume.exe read the PDB directly.

```sh
./bin/Volume.exe -i 1A01.pdb --exclude-ions --exclude-water -p 1.5 -g 0.5 -o 1a01-excluded.pdb
```

## 3. Build the volume tool

```sh
cd src
make vol
cd ..
```

## 4. Compute solvent-excluded volume

```sh
bin/Volume.exe -i 1a01-filtered.xyzr -p 1.5 -g 0.5
```

## 5. Output to PDB and MRC

```sh
bin/Volume.exe -i 1a01-filtered.xyzr -p 1.5 -g 0.5 -o 1a01-excluded.pdb
bin/Volume.exe -i 1a01-filtered.xyzr -p 1.5 -g 0.5 -m 1a01-excluded.mrc
```

## 6. View the MRC in Chimera

```sh
chimera 1a01-excluded.mrc
```

Download Chimera from [UCSF Chimera](http://www.cgl.ucsf.edu/chimera/).
