#!/bin/bash

set -euo pipefail

cleanup_files=()
cleanup() {
  for f in "${cleanup_files[@]:-}"; do
    if [ -n "$f" ] && [ -f "$f" ]; then
      rm -f "$f"
    fi
  done
}
trap cleanup EXIT

compare_float() {
  local label="$1"
  local expected="$2"
  local actual="$3"
  local tolerance="${4:-1e-3}"
  python3 - "$label" "$expected" "$actual" "$tolerance" <<'PY'
import sys, math
label, expected, actual, tol = sys.argv[1:]
expected = float(expected)
actual = float(actual)
tol = float(tol)
if math.fabs(expected - actual) > tol:
    print(f"{label} mismatch: expected {expected}, got {actual}")
    sys.exit(1)
PY
}
overall_status=0

cd "$(dirname "$0")"

PDB_ID="2LYZ"
PDB_FILE="${PDB_ID}.pdb"
PDB_ATOM_ONLY="${PDB_ID}-atom-only.pdb"
XYZR_FILE="${PDB_ID}-classic.xyzr"
OUTPUT_PDB="${PDB_ID}-volume-classic.pdb"
VOLUME_TOLERANCE=0.01

PDB_TO_XYZR_BIN="${PDB_TO_XYZR_BIN:-../xyzr/pdb_to_xyzr.sh}"
LEGACY_BIN="${LEGACY_BIN:-../bin/Volume-legacy.exe}"
REFERENCE_BIN="${REFERENCE_BIN:-../bin/Volume-1.0.exe}"

if [ -s "${PDB_FILE}" ]; then
  echo "Found existing ${PDB_FILE}; skipping download."
else
  if [ -s "${PDB_FILE}.gz" ]; then
    echo "Found existing ${PDB_FILE}.gz; reusing local copy."
  else
    echo "Downloading PDB file for ${PDB_ID}..."
    curl -s -L -o "${PDB_FILE}.gz" "https://files.rcsb.org/download/${PDB_ID}.pdb.gz"
  fi
  echo "Extracting PDB file..."
  gunzip -f "${PDB_FILE}.gz"
fi

PDB_LINES=$(wc -l < "${PDB_FILE}")
echo "Downloaded PDB file has ${PDB_LINES} lines."

echo "Filtering ATOM records from ${PDB_FILE}..."
grep -E "^ATOM  " "${PDB_FILE}" > "${PDB_ATOM_ONLY}"
ATOM_LINES=$(wc -l < "${PDB_ATOM_ONLY}")
echo "Filtered PDB file has ${ATOM_LINES} lines."

"${PDB_TO_XYZR_BIN}" "${PDB_ATOM_ONLY}" > "${XYZR_FILE}"

if [ ! -x "${LEGACY_BIN}" ]; then
  echo "Compiling legacy Volume program..."
  pushd ../src > /dev/null
  make volume_original
  popd > /dev/null
else
  echo "Legacy Volume program is already compiled."
fi

if [ ! -x "${REFERENCE_BIN}" ]; then
  echo "Reference binary ${REFERENCE_BIN} not found; classic regression requires the Volume-1.0.exe binary." >&2
  echo "Download/build the 1.0 codebase and place the binary at ${REFERENCE_BIN}, or set REFERENCE_BIN to its location." >&2
  exit 1
fi

run_volume() {
  local bin="$1"
  local label="$2"
  local pdb="$3"
  local log
  log=$(mktemp)
  cleanup_files+=("$log")
  echo "Running ${label} (${bin}) -> ${pdb}"
  if ! "${bin}" -i "${XYZR_FILE}" -p 2.1 -g 0.9 -o "${pdb}" >"${log}" 2>&1; then
    echo "${label} run failed; log output:" >&2
    cat "${log}" >&2
    exit 1
  fi

  local summary_line
  summary_line=$(grep -F "probe,grid,volume" "${log}" | tail -n 1 || true)
  if [ -z "${summary_line}" ]; then
    echo "Unable to locate volume summary in ${label} output." >&2
    cat "${log}" >&2
    exit 1
  fi
  summary_line=$(echo "${summary_line}" | awk '{$1=$1}1')
  read -r summary_probe summary_grid summary_volume summary_surface summary_atoms summary_input summary_label <<<"${summary_line}"

  local lines
  lines=$(wc -l < "${pdb}")
  local sanitized
  sanitized=$(mktemp)
  cleanup_files+=("$sanitized")
  grep -v '^REMARK Date' "${pdb}" > "${sanitized}"
  local md5
  md5=$(md5sum "${sanitized}" | awk '{print $1}')

  eval "${label}_LOG=\"${log}\""
  eval "${label}_PDB=\"${pdb}\""
  eval "${label}_SANITIZED=\"${sanitized}\""
  eval "${label}_VOLUME=\"${summary_volume}\""
  eval "${label}_SURFACE=\"${summary_surface}\""
  eval "${label}_ATOMS=\"${summary_atoms}\""
  eval "${label}_LINES=${lines}"
  eval "${label}_MD5=\"${md5}\""
}

REF_PDB="${PDB_ID}-volume-reference.pdb"
LEG_PDB="${OUTPUT_PDB}"

run_volume "${REFERENCE_BIN}" "REFERENCE" "${REF_PDB}"
run_volume "${LEGACY_BIN}" "LEGACY" "${LEG_PDB}"

echo
echo "Reference summary: volume=${REFERENCE_VOLUME} A^3, surface=${REFERENCE_SURFACE} A^2, atoms=${REFERENCE_ATOMS}, lines=${REFERENCE_LINES}, md5=${REFERENCE_MD5}"
echo "Legacy summary:    volume=${LEGACY_VOLUME} A^3, surface=${LEGACY_SURFACE} A^2, atoms=${LEGACY_ATOMS}, lines=${LEGACY_LINES}, md5=${LEGACY_MD5}"

if ! compare_float "Volume difference" "${REFERENCE_VOLUME}" "${LEGACY_VOLUME}" "${VOLUME_TOLERANCE}"; then
  overall_status=1
fi
if ! compare_float "Legacy surface" "${REFERENCE_SURFACE}" "${LEGACY_SURFACE}" "${VOLUME_TOLERANCE}"; then
  overall_status=1
fi
if [ "${REFERENCE_ATOMS}" -ne "${LEGACY_ATOMS}" ]; then
  echo "Atom count mismatch: reference ${REFERENCE_ATOMS}, legacy ${LEGACY_ATOMS}" >&2
  overall_status=1
fi
if [ "${REFERENCE_LINES}" -ne "${LEGACY_LINES}" ]; then
  echo "Line count mismatch: reference ${REFERENCE_LINES}, legacy ${LEGACY_LINES}" >&2
  overall_status=1
fi
if [ "${REFERENCE_MD5}" != "${LEGACY_MD5}" ]; then
  echo "Sanitized MD5 mismatch: reference ${REFERENCE_MD5}, legacy ${LEGACY_MD5}" >&2
  overall_status=1
fi

if [ "${overall_status}" -ne 0 ]; then
  echo "Classic regression FAILED."
  exit 1
fi

echo "Classic regression passed: Volume-legacy.exe matches historical Volume-1.0.exe."
