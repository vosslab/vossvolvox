#!/bin/bash

# Exit immediately if any command fails
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

# Allow overriding the converter binary (useful for comparing implementations)
PDB_TO_XYZR_BIN="${PDB_TO_XYZR_BIN:-../bin/pdb_to_xyzr.exe}"

# Define constants
PDB_ID="2LYZ"
PDB_FILE="${PDB_ID}.pdb"
XYZR_FILE="${PDB_ID}-filtered.xyzr"
OUTPUT_PDB="${PDB_ID}-volume.pdb"
EXPECTED_OUTPUT_LINES=4882
EXPECTED_HETATM_LINES=4870
EXPECTED_HETATM_MD5="d8611307bbcc68315bd10d1a7f1f3741"
EXPECTED_VOLUME=18550.861
EXPECTED_SURFACE=4982.05
VOLUME_TOLERANCE=0.01

# Step A/B: Download or reuse the PDB file
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

# Count lines in the downloaded PDB file
PDB_LINES=$(wc -l < "${PDB_FILE}")
echo "Downloaded PDB file has ${PDB_LINES} lines."

# Step C: Convert to XYZR format with precise filters
FILTER_ARGS=(--exclude-ions --exclude-water)
if [ -n "${PDB_TO_XYZR_FILTERS:-}" ]; then
  # shellcheck disable=SC2206
  FILTER_ARGS=(${PDB_TO_XYZR_FILTERS})
fi
echo "Converting ${PDB_FILE} to XYZR format using ${PDB_TO_XYZR_BIN} ${FILTER_ARGS[*]}..."
"${PDB_TO_XYZR_BIN}" "${FILTER_ARGS[@]}" -i "${PDB_FILE}" > "${XYZR_FILE}"

# Step E: Compile the Volume program (if needed)
if [ ! -x ../bin/Volume.exe ]; then
  echo "Compiling Volume program..."
  pushd ../src > /dev/null  # Save current directory and switch to ../src
  make vol
  popd > /dev/null  # Return to the test directory
else
  echo "Volume program is already compiled."
fi

# Step F: Run the Volume program
echo "Running Volume.exe with probe radius 2.1 and grid spacing 0.9..."
VOLUME_LOG=$(mktemp)
cleanup_files+=("$VOLUME_LOG")
if ! ../bin/Volume.exe -i "${XYZR_FILE}" -p 2.1 -g 0.9 -o "${OUTPUT_PDB}" >"${VOLUME_LOG}" 2>&1; then
  echo "Volume.exe failed; log output:" >&2
  cat "${VOLUME_LOG}" >&2
  exit 1
fi

SUMMARY_LINE=$(grep -F "probe,grid,volume" "${VOLUME_LOG}" | tail -n 1 || true)
if [ -z "${SUMMARY_LINE}" ]; then
  echo "Unable to locate volume summary in Volume.exe output." >&2
  cat "${VOLUME_LOG}" >&2
  exit 1
fi
SUMMARY_NORMALIZED=$(echo "${SUMMARY_LINE}" | awk '{$1=$1}1')
read -r SUMMARY_PROBE SUMMARY_GRID SUMMARY_VOLUME SUMMARY_SURFACE SUMMARY_ATOMS SUMMARY_INPUT SUMMARY_LABEL <<<"${SUMMARY_NORMALIZED}"
echo "Volume summary: volume=${SUMMARY_VOLUME} A^3, surface=${SUMMARY_SURFACE} A^2, atoms=${SUMMARY_ATOMS} (input ${SUMMARY_INPUT})."
if ! compare_float "Volume" "${EXPECTED_VOLUME}" "${SUMMARY_VOLUME}" "${VOLUME_TOLERANCE}"; then
  overall_status=1
fi
if ! compare_float "Surface area" "${EXPECTED_SURFACE}" "${SUMMARY_SURFACE}" "${VOLUME_TOLERANCE}"; then
  overall_status=1
fi

# Count lines in the output PDB file
OUTPUT_LINES=$(wc -l < "${OUTPUT_PDB}")
echo "Output PDB file has ${OUTPUT_LINES} lines."
if [ "${OUTPUT_LINES}" -ne "${EXPECTED_OUTPUT_LINES}" ]; then
  echo "Line count mismatch: expected ${EXPECTED_OUTPUT_LINES}, got ${OUTPUT_LINES}" >&2
  overall_status=1
fi

# Step G: Calculate the HETATM-only MD5 checksum of the output
HETATM_FILE=$(mktemp)
cleanup_files+=("$HETATM_FILE")
grep '^HETATM' "${OUTPUT_PDB}" > "${HETATM_FILE}"
HETATM_LINES=$(wc -l < "${HETATM_FILE}")
echo "Output PDB file has ${HETATM_LINES} HETATM lines."
if [ "${HETATM_LINES}" -ne "${EXPECTED_HETATM_LINES}" ]; then
  echo "HETATM line count mismatch: expected ${EXPECTED_HETATM_LINES}, got ${HETATM_LINES}" >&2
  overall_status=1
fi
echo "Calculating HETATM-only MD5 checksum of ${OUTPUT_PDB}..."
HETATM_MD5=$(md5sum "${HETATM_FILE}" | awk '{print $1}')
echo "HETATM md5sum value of '${HETATM_MD5}'"

# Step H: Compare the HETATM-only MD5 checksum to the expected value
if [ "$HETATM_MD5" == "$EXPECTED_HETATM_MD5" ]; then
  echo "Test passed: HETATM MD5 checksum matches expected value."
else
  echo "Test failed: HETATM MD5 checksum does not match expected value."
  echo "Expected: $EXPECTED_HETATM_MD5"
  echo "Actual:   $HETATM_MD5"
  overall_status=1
fi

if [ "${overall_status}" -ne 0 ]; then
  echo "Test completed with failures."
  exit 1
fi

echo "All tests completed successfully!"
