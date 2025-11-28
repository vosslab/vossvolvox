#!/usr/bin/env python3
"""Convert PDB coordinates into xyzr format compatible with ms/ams."""

from __future__ import annotations

import argparse
import gzip
import io
import os
import re
import shlex
import sys
import xml.etree.ElementTree as ET
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, Iterator, List, Optional, Pattern, Set, TextIO, Tuple


@dataclass
class AtomPattern:
    residue: Pattern[str]
    atom: Pattern[str]
    atmnum: str


@dataclass
class PDBAtom:
    x: str
    y: str
    z: str
    residue: str
    atom: str
    resnum: str
    line_no: int
    record: str
    chain: str
    element: str


SUPPORTED_FORMATS = ("pdb", "mmcif", "pdbxml")

WATER_NAMES: Set[str] = {
    "HOH",
    "H2O",
    "DOD",
    "WAT",
    "SOL",
    "TIP",
    "TIP3",
    "TIP3P",
    "TIP4",
    "TIP4P",
    "TIP5P",
    "SPC",
    "OH2",
}

AMINO_ACID_RESIDUES: Set[str] = {
    "ALA",
    "ARG",
    "ASN",
    "ASP",
    "ASX",
    "CYS",
    "GLN",
    "GLU",
    "GLX",
    "GLY",
    "HIS",
    "HID",
    "HIE",
    "HIP",
    "HISN",
    "HISL",
    "ILE",
    "LEU",
    "LYS",
    "MET",
    "MSE",
    "PHE",
    "PRO",
    "SER",
    "THR",
    "TRP",
    "TYR",
    "VAL",
    "SEC",
    "PYL",
    "ASH",
    "GLH",
}

NUCLEIC_ACID_RESIDUES: Set[str] = {
    "A",
    "C",
    "G",
    "U",
    "I",
    "DA",
    "DG",
    "DC",
    "DT",
    "DI",
    "ADE",
    "GUA",
    "CYT",
    "URI",
    "THY",
    "PSU",
    "OMC",
    "OMU",
    "OMG",
    "5IU",
    "H2U",
    "M2G",
    "7MG",
    "1MA",
    "1MG",
    "2MG",
}

ION_RESIDUES: Set[str] = {
    "NA",
    "K",
    "MG",
    "MN",
    "FE",
    "ZN",
    "CU",
    "CA",
    "CL",
    "BR",
    "I",
    "LI",
    "CO",
    "NI",
    "HG",
    "CD",
    "SR",
    "CS",
    "BA",
    "YB",
    "MO",
    "RU",
    "OS",
    "IR",
    "AU",
    "AG",
    "PT",
    "TI",
    "AL",
    "GA",
    "V",
    "W",
    "ZN2",
    "FE2",
}

ION_ELEMENTS: Set[str] = {
    "NA",
    "K",
    "MG",
    "MN",
    "FE",
    "ZN",
    "CU",
    "CA",
    "CL",
    "BR",
    "I",
    "LI",
    "CO",
    "NI",
    "HG",
    "CD",
    "SR",
    "CS",
    "BA",
    "YB",
    "MO",
    "RU",
    "OS",
    "IR",
    "AU",
    "AG",
    "PT",
    "TI",
    "AL",
    "GA",
    "V",
    "W",
    "ZN",
    "FE",
    "MN",
    "CU",
}


@dataclass
class ResidueInfo:
    name: str
    chain: str
    resnum: str
    atom_count: int = 0
    hetatm_only: bool = True
    polymer_flag: bool = False
    elements: Set[str] = None
    is_water: bool = False
    is_nucleic: bool = False
    is_amino: bool = False
    is_ion: bool = False
    is_ligand: bool = False

    def __post_init__(self) -> None:
        if self.elements is None:
            self.elements = set()


@dataclass
class Filters:
    exclude_ions: bool = False
    exclude_ligands: bool = False
    exclude_hetatm: bool = False
    exclude_water: bool = False
    exclude_nucleic_acids: bool = False
    exclude_amino_acids: bool = False


def _normalize_name(name: str) -> str:
    return name.strip().upper()


def _looks_like_nucleic(name: str) -> bool:
    if len(name) == 1 and name in {"A", "C", "G", "U", "I", "T"}:
        return True
    if len(name) == 2 and name.startswith("D") and name[1] in {"A", "C", "G", "U", "T"}:
        return True
    return False


def _residue_key(atom: PDBAtom) -> Tuple[str, str, str]:
    return (_normalize_name(atom.chain), atom.resnum.strip(), _normalize_name(atom.residue))


def _is_water(name: str) -> bool:
    upper = _normalize_name(name)
    return upper in WATER_NAMES or upper.startswith("HOH") or upper.startswith("TIP")


def _is_amino(name: str) -> bool:
    upper = _normalize_name(name)
    return upper in AMINO_ACID_RESIDUES


def _is_nucleic(name: str) -> bool:
    upper = _normalize_name(name)
    return upper in NUCLEIC_ACID_RESIDUES or _looks_like_nucleic(upper)


def _is_ion(info: ResidueInfo) -> bool:
    upper = _normalize_name(info.name)
    if upper in ION_RESIDUES:
        return True
    if info.atom_count <= 1:
        for element in info.elements:
            elem = _normalize_name(element)
            if elem in ION_ELEMENTS:
                return True
        if upper in ION_ELEMENTS:
            return True
    return False


def classify_residues(atoms: List[PDBAtom]) -> Dict[Tuple[str, str, str], ResidueInfo]:
    residues: Dict[Tuple[str, str, str], ResidueInfo] = {}
    for atom in atoms:
        key = _residue_key(atom)
        info = residues.get(key)
        if info is None:
            info = ResidueInfo(name=atom.residue, chain=atom.chain, resnum=atom.resnum)
            residues[key] = info
        info.atom_count += 1
        if atom.element:
            info.elements.add(_normalize_name(atom.element))
        if atom.record.upper() == "ATOM":
            info.polymer_flag = True
        if atom.record.upper() != "HETATM":
            info.hetatm_only = False
    for info in residues.values():
        name = _normalize_name(info.name)
        if name in AMINO_ACID_RESIDUES or name in NUCLEIC_ACID_RESIDUES:
            info.polymer_flag = True
        info.is_water = _is_water(name)
        info.is_amino = _is_amino(name)
        info.is_nucleic = _is_nucleic(name)
        info.is_ion = _is_ion(info)
        info.is_ligand = not info.polymer_flag and not info.is_water and not info.is_ion
    return residues


def should_filter(atom: PDBAtom, info: Optional[ResidueInfo], filters: Filters) -> bool:
    if info is None:
        return False
    if filters.exclude_water and info.is_water:
        return True
    if filters.exclude_ions and info.is_ion:
        return True
    if filters.exclude_ligands and info.is_ligand:
        return True
    if filters.exclude_hetatm and info.hetatm_only:
        return True
    if filters.exclude_nucleic_acids and info.is_nucleic:
        return True
    if filters.exclude_amino_acids and info.is_amino:
        return True
    return False

def format_coordinate(value: str | float) -> str:
    try:
        number = float(value)
    except ValueError as exc:
        raise ValueError(f"Invalid coordinate value {value!r}") from exc
    return f"{number:8.3f}"


def normalize_atom_name(raw: str) -> str:
    """Return canonical atom label with heuristics for hydrogens."""

    candidate = raw.strip()
    padded = (raw + "  ")[:2]
    if re.match(r"[ 0-9][HhDd]", padded):
        return "H"
    if re.match(r"[Hh][^Gg]", padded):
        return "H"
    return candidate.replace(" ", "")


class AtomTypeLibrary:
    """Load radius data and atom/residue matching patterns."""

    def __init__(self, path: Path) -> None:
        self.path = path
        self.explicit_radii: dict[str, str] = {}
        self.united_radii: dict[str, str] = {}
        self.patterns: List[AtomPattern] = []
        self._load()

    def _load(self) -> None:
        if not self.path.is_file():
            raise FileNotFoundError(self.path)

        with self.path.open("r", encoding="utf-8", errors="replace") as handle:
            for idx, raw_line in enumerate(handle, start=1):
                line = raw_line.strip()
                if not line or line.startswith("#"):
                    continue

                # Strip inline comments while keeping token spacing.
                if "#" in line:
                    line = line.split("#", 1)[0].strip()
                if not line:
                    continue

                fields = line.split()
                if not fields:
                    continue

                if fields[0] == "radius":
                    self._add_radius(fields, idx)
                else:
                    self._add_pattern(fields, idx)

    def _add_radius(self, fields: List[str], line_no: int) -> None:
        if len(fields) < 4:
            raise ValueError(
                f"Invalid radius entry in {self.path} line {line_no}: {' '.join(fields)}"
            )

        key = fields[1]
        explicit_text = fields[3]
        try:
            float(explicit_text)
        except ValueError as exc:
            raise ValueError(
                f"Invalid explicit radius in {self.path} line {line_no}: {explicit_text!r}"
            ) from exc

        if len(fields) >= 5:
            united_text = fields[4]
            try:
                float(united_text)
            except ValueError:
                united_text = explicit_text
        else:
            united_text = explicit_text

        self.explicit_radii[key] = explicit_text
        self.united_radii[key] = united_text

    def _add_pattern(self, fields: List[str], line_no: int) -> None:
        if len(fields) < 3:
            raise ValueError(
                f"Invalid atom entry in {self.path} line {line_no}: {' '.join(fields)}"
            )

        residue_pattern = ".*" if fields[0] == "*" else fields[0]
        atom_pattern = fields[1].replace("_", " ")

        try:
            residue_regex = re.compile(f"^{residue_pattern}$")
            atom_regex = re.compile(f"^{atom_pattern}$")
        except re.error as exc:
            raise ValueError(
                f"Invalid regex in {self.path} line {line_no}: {exc}"
            ) from exc

        atmnum = fields[2]
        if atmnum not in self.explicit_radii:
            print(
                "pdb_to_xyzr: error in library file",
                self.path,
                "entry",
                fields[0],
                fields[1],
                fields[2],
                "has no corresponding radius value",
                file=sys.stderr,
            )
            self.explicit_radii.setdefault(atmnum, "0.01")
            self.united_radii.setdefault(atmnum, "0.01")

        self.patterns.append(
            AtomPattern(residue=residue_regex, atom=atom_regex, atmnum=atmnum)
        )

    def radius_for(self, residue: str, atom: str, use_united: bool) -> tuple[Optional[str], str]:
        for pattern in self.patterns:
            if pattern.atom.search(atom) and pattern.residue.search(residue):
                radii = self.united_radii if use_united else self.explicit_radii
                return pattern.atmnum, radii.get(pattern.atmnum, "0.01")
        return None, "0.01"


def parse_args(argv: Optional[Iterable[str]] = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        add_help=False,
        description=(
            "Extract x, y, z coordinates and atom radii from a PDB formatted file "
            "and write xyzr records to stdout."
        ),
    )
    parser.add_argument(
        "-h",
        "--hydrogens",
        action="store_true",
        help="use explicit hydrogen radii instead of the default united-atom radii",
    )
    parser.add_argument(
        "--help",
        action="help",
        help="show this help message and exit",
    )
    parser.add_argument(
        "-t",
        "--atmtypenumbers",
        metavar="FILE",
        help=(
            "path to the atmtypenumbers file (defaults to the first readable file among "
            "$PDB_TO_XYZR_TABLE, a file next to this script, or ./atmtypenumbers)"
        ),
    )
    parser.add_argument(
        "-f",
        "--format",
        choices=SUPPORTED_FORMATS,
        help="force the input format (autodetected from extension by default)",
    )
    parser.add_argument(
        "--exclude-ions",
        action="store_true",
        help="exclude residues classified as ions",
    )
    parser.add_argument(
        "--exclude-ligands",
        action="store_true",
        help="exclude non-polymer ligands (excludes ions and water automatically)",
    )
    parser.add_argument(
        "--exclude-hetatm",
        action="store_true",
        help="exclude residues composed solely of HETATM records",
    )
    parser.add_argument(
        "--exclude-water",
        action="store_true",
        help="exclude water molecules / solvent",
    )
    parser.add_argument(
        "--exclude-nucleic-acids",
        action="store_true",
        help="exclude nucleic-acid residues",
    )
    parser.add_argument(
        "--exclude-amino-acids",
        action="store_true",
        help="exclude amino-acid residues",
    )
    parser.add_argument(
        "pdb",
        nargs="?",
        default="-",
        help=(
            "input structure file (defaults to stdin). Autodetects .pdb, .cif, and .xml "
            "along with their .gz equivalents when a path is provided"
        ),
    )
    return parser.parse_args(argv)


def determine_format(source: str, override: Optional[str]) -> str:
    if override:
        return override

    if source == "-":
        return "pdb"

    path = Path(source)
    suffixes = [suffix.lower() for suffix in path.suffixes]
    if suffixes and suffixes[-1] == ".gz":
        suffixes = suffixes[:-1]
    if not suffixes:
        return "pdb"

    mapping = {
        ".pdb": "pdb",
        ".ent": "pdb",
        ".brk": "pdb",
        ".pdb1": "pdb",
        ".cif": "mmcif",
        ".mmcif": "mmcif",
        ".xml": "pdbxml",
        ".pdbxml": "pdbxml",
        ".pdbml": "pdbxml",
    }
    return mapping.get(suffixes[-1], "pdb")


def resolve_atmtypenumbers_path(cli_value: Optional[str]) -> Path:
    candidates: List[Path] = []
    if cli_value:
        candidates.append(Path(cli_value))

    env_value = os.getenv("PDB_TO_XYZR_TABLE")
    if env_value:
        candidates.append(Path(env_value))

    script_dir = Path(__file__).resolve().parent
    candidates.append(script_dir / "atmtypenumbers")
    candidates.append(Path.cwd() / "atmtypenumbers")

    seen: set[Path] = set()
    for candidate in candidates:
        if not candidate:
            continue
        candidate = candidate.expanduser()
        if candidate in seen:
            continue
        seen.add(candidate)
        if candidate.is_file():
            return candidate

    raise FileNotFoundError(
        "Unable to locate atmtypenumbers file; specify it with --atmtypenumbers"
    )


def open_structure_source(source: str) -> TextIO:
    if source == "-":
        return sys.stdin

    path = Path(source)
    if not path.is_file():
        raise FileNotFoundError(path)

    if any(suffix.lower() == ".gz" for suffix in path.suffixes):
        return io.TextIOWrapper(gzip.open(path, "rb"), encoding="utf-8", errors="replace")

    return path.open("r", encoding="utf-8", errors="replace")


def iter_pdb_atoms(handle: TextIO) -> Iterator[PDBAtom]:
    for line_no, line in enumerate(handle, start=1):
        record = line[:6].strip().upper()
        if record not in {"ATOM", "HETATM"}:
            continue

        x_raw = line[30:38]
        y_raw = line[38:46]
        z_raw = line[46:54]
        if not (x_raw.strip() and y_raw.strip() and z_raw.strip()):
            continue
        residue = line[17:20].strip()
        atom_name = line[12:16]
        resnum = line[22:26].strip()
        chain = line[21:22].strip()
        element = line[76:78].strip() if len(line) >= 78 else ""
        if not element:
            element = re.sub(r"[^A-Za-z]", "", atom_name).upper()[:2]

        yield PDBAtom(
            x=x_raw,
            y=y_raw,
            z=z_raw,
            residue=residue,
            atom=normalize_atom_name(atom_name),
            resnum=resnum,
            line_no=line_no,
            record=record,
            chain=chain,
            element=element.upper(),
        )


def _collect_mmcif_row_values(lines: List[str], start_index: int, expected: int) -> tuple[List[str], int]:
    values: List[str] = []
    consumed = 0
    idx = start_index
    while idx < len(lines):
        raw_line = lines[idx]
        stripped = raw_line.strip()
        lower = stripped.lower()
        if not stripped or stripped.startswith("#"):
            if consumed == 0:
                idx += 1
                consumed += 1
                continue
            break
        if consumed > 0 and (lower.startswith("loop_") or stripped.startswith("_") or lower.startswith("data_")):
            break
        values.extend(shlex.split(raw_line, comments=False, posix=True))
        consumed += 1
        idx += 1
        if len(values) >= expected:
            break
    if consumed == 0:
        consumed = 1
    return values, consumed


def _mmcif_value(values: List[str], tag_index: Dict[str, int], *candidates: str) -> str:
    for tag in candidates:
        idx = tag_index.get(tag)
        if idx is None:
            continue
        value = values[idx]
        if value not in {".", "?"}:
            return value
    return ""


def iter_mmcif_atoms(text: str, source_label: str) -> Iterator[PDBAtom]:
    lines = text.splitlines()
    i = 0
    while i < len(lines):
        stripped = lines[i].strip()
        lower = stripped.lower()
        if not stripped or stripped.startswith("#") or lower.startswith("data_"):
            i += 1
            continue
        if lower != "loop_":
            i += 1
            continue
        i += 1
        tags: List[str] = []
        while i < len(lines):
            current = lines[i].strip()
            if not current or current.startswith("#"):
                i += 1
                continue
            if current.startswith("_"):
                tags.append(current)
                i += 1
                continue
            break
        if not tags:
            continue
        lower_tags = [tag.lower() for tag in tags]
        if not any(tag.startswith("_atom_site.") for tag in lower_tags):
            while i < len(lines):
                checkpoint = lines[i].strip()
                lower_checkpoint = checkpoint.lower()
                if not checkpoint or checkpoint.startswith("#"):
                    i += 1
                    continue
                if lower_checkpoint.startswith("loop_") or checkpoint.startswith("_") or lower_checkpoint.startswith("data_"):
                    break
                i += 1
            continue

        tag_index = {tag: idx for idx, tag in enumerate(lower_tags)}
        required = ["_atom_site.cartn_x", "_atom_site.cartn_y", "_atom_site.cartn_z"]
        missing = [tag for tag in required if tag not in tag_index]
        if missing:
            raise ValueError(
                f"Missing required columns ({', '.join(missing)}) in _atom_site loop of {source_label}"
            )

        while i < len(lines):
            raw_line = lines[i]
            stripped_line = raw_line.strip()
            lower_line = stripped_line.lower()
            if not stripped_line or stripped_line.startswith("#"):
                i += 1
                continue
            if lower_line.startswith("loop_") or stripped_line.startswith("_") or lower_line.startswith("data_"):
                break
            row_start = i
            values, consumed = _collect_mmcif_row_values(lines, i, len(tags))
            i = row_start + consumed
            if len(values) != len(tags):
                continue

            x_val = _mmcif_value(values, tag_index, "_atom_site.cartn_x")
            y_val = _mmcif_value(values, tag_index, "_atom_site.cartn_y")
            z_val = _mmcif_value(values, tag_index, "_atom_site.cartn_z")
            if not x_val or not y_val or not z_val:
                continue
            try:
                x = format_coordinate(x_val)
                y = format_coordinate(y_val)
                z = format_coordinate(z_val)
            except ValueError:
                continue

            residue = _mmcif_value(
                values,
                tag_index,
                "_atom_site.label_comp_id",
                "_atom_site.auth_comp_id",
            )
            atom_label = _mmcif_value(
                values,
                tag_index,
                "_atom_site.label_atom_id",
                "_atom_site.auth_atom_id",
            )
            if not atom_label:
                atom_label = _mmcif_value(values, tag_index, "_atom_site.type_symbol")
            resnum = _mmcif_value(
                values,
                tag_index,
                "_atom_site.auth_seq_id",
                "_atom_site.label_seq_id",
            )
            insertion = _mmcif_value(values, tag_index, "_atom_site.pdbx_pdb_ins_code")
            if insertion and insertion not in {".", "?"}:
                resnum = f"{resnum}{insertion}"
            chain = _mmcif_value(
                values,
                tag_index,
                "_atom_site.auth_asym_id",
                "_atom_site.label_asym_id",
            )
            record_type = _mmcif_value(values, tag_index, "_atom_site.group_pdb") or "ATOM"
            element = _mmcif_value(values, tag_index, "_atom_site.type_symbol")

            yield PDBAtom(
                x=x,
                y=y,
                z=z,
                residue=residue,
                atom=normalize_atom_name(atom_label),
                resnum=resnum,
                line_no=row_start + 1,
                record=record_type.upper(),
                chain=chain,
                element=(element or "").upper(),
            )
        # continue scanning rest of file for additional loops


def _strip_namespace(tag: str) -> str:
    return tag.split("}", 1)[-1] if "}" in tag else tag


def iter_pdbxml_atoms(text: str, source_label: str) -> Iterator[PDBAtom]:
    try:
        root = ET.fromstring(text)
    except ET.ParseError as exc:
        raise ValueError(f"Unable to parse XML from {source_label}: {exc}") from exc

    line_no = 0
    for atom_el in root.iter():
        if _strip_namespace(atom_el.tag).lower() != "atom_site":
            continue
        data: Dict[str, str] = {}
        for child in atom_el:
            key = _strip_namespace(child.tag).lower()
            data[key] = (child.text or "").strip()

        x_val = data.get("cartn_x")
        y_val = data.get("cartn_y")
        z_val = data.get("cartn_z")
        if not x_val or not y_val or not z_val:
            continue

        try:
            x = format_coordinate(x_val)
            y = format_coordinate(y_val)
            z = format_coordinate(z_val)
        except ValueError:
            continue

        residue = data.get("label_comp_id") or data.get("auth_comp_id") or ""
        atom_label = data.get("label_atom_id") or data.get("auth_atom_id") or data.get("type_symbol") or ""
        resnum = data.get("auth_seq_id") or data.get("label_seq_id") or ""
        insertion = data.get("pdbx_pdb_ins_code")
        if insertion and insertion not in {".", "?"}:
            resnum = f"{resnum}{insertion}"
        chain = data.get("auth_asym_id") or data.get("label_asym_id") or ""
        record_type = data.get("group_pdb") or "ATOM"
        element = data.get("type_symbol") or ""

        line_no += 1
        yield PDBAtom(
            x=x,
            y=y,
            z=z,
            residue=residue,
            atom=normalize_atom_name(atom_label),
            resnum=resnum,
            line_no=line_no,
            record=record_type.upper(),
            chain=chain,
            element=element.upper(),
        )


def process_structure(
    library: AtomTypeLibrary,
    handle: TextIO,
    source_label: str,
    use_united: bool,
    structure_format: str,
    filters: Filters,
) -> None:
    if structure_format == "pdb":
        atoms = list(iter_pdb_atoms(handle))
    else:
        text = handle.read()
        if structure_format == "mmcif":
            atoms = list(iter_mmcif_atoms(text, source_label))
        elif structure_format == "pdbxml":
            atoms = list(iter_pdbxml_atoms(text, source_label))
        else:
            raise ValueError(f"Unsupported format {structure_format!r}")

    if not atoms:
        return

    residue_info = classify_residues(atoms)

    for atom in atoms:
        info = residue_info.get(_residue_key(atom))
        if should_filter(atom, info, filters):
            continue
        atmnum, radius = library.radius_for(atom.residue, atom.atom, use_united)
        if atmnum is None:
            print(
                "pdb_to_xyzr: error, file",
                source_label,
                "line",
                atom.line_no,
                "residue",
                atom.resnum,
                "atom pattern",
                atom.residue,
                atom.atom,
                "was not found in",
                library.path,
                file=sys.stderr,
            )
        print(atom.x, atom.y, atom.z, radius)


def main(argv: Optional[Iterable[str]] = None) -> int:
    args = parse_args(argv)

    try:
        table_path = resolve_atmtypenumbers_path(args.atmtypenumbers)
    except FileNotFoundError as exc:
        print(f"pdb_to_xyzr: {exc}", file=sys.stderr)
        return 2

    try:
        library = AtomTypeLibrary(table_path)
    except (FileNotFoundError, ValueError) as exc:
        print(f"pdb_to_xyzr: {exc}", file=sys.stderr)
        return 2

    structure_format = determine_format(args.pdb, args.format)

    try:
        handle = open_structure_source(args.pdb)
    except FileNotFoundError as exc:
        print(f"pdb_to_xyzr: {exc}", file=sys.stderr)
        return 2

    use_united = not args.hydrogens
    label = "<stdin>" if args.pdb == "-" else args.pdb
    filters = Filters(
        exclude_ions=args.exclude_ions,
        exclude_ligands=args.exclude_ligands,
        exclude_hetatm=args.exclude_hetatm,
        exclude_water=args.exclude_water,
        exclude_nucleic_acids=args.exclude_nucleic_acids,
        exclude_amino_acids=args.exclude_amino_acids,
    )
    try:
        process_structure(library, handle, label, use_united, structure_format, filters)
    except ValueError as exc:
        print(f"pdb_to_xyzr: {exc}", file=sys.stderr)
        return 2
    finally:
        if handle is not sys.stdin:
            handle.close()

    return 0


if __name__ == "__main__":
    sys.exit(main())
