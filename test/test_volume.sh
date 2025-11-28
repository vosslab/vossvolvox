#!/bin/bash

# Exit immediately if any command fails
set -e

# Allow overriding the converter binary (useful for comparing implementations)
PDB_TO_XYZR_BIN="${PDB_TO_XYZR_BIN:-../bin/pdb_to_xyzr.exe}"

# Define constants
PDB_ID="2LYZ"
PDB_FILE="${PDB_ID}.pdb"
PDB_NOIONS="${PDB_ID}-noions.pdb"
XYZR_FILE="${PDB_ID}-noions.xyzr"
OUTPUT_PDB="${PDB_ID}-volume.pdb"
EXPECTED_MD5="06d3c78774706cb6fe4b2ded04bc2882"

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

# Step C: Filter the ATOM lines to remove ions and save to a new file
echo "Filtering ATOM lines from ${PDB_FILE}..."
grep -E "^ATOM  " "${PDB_FILE}" > "${PDB_NOIONS}"

# Count lines in the filtered PDB file
NOIONS_LINES=$(wc -l < "${PDB_NOIONS}")
echo "Filtered PDB file (no ions) has ${NOIONS_LINES} lines."

# Step D: Convert the filtered PDB to XYZR format
echo "Converting ${PDB_NOIONS} to XYZR format using ${PDB_TO_XYZR_BIN}..."
"${PDB_TO_XYZR_BIN}" "${PDB_NOIONS}" > "${XYZR_FILE}"

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
../bin/Volume.exe -i "${XYZR_FILE}" -p 2.1 -g 0.9 -o "${OUTPUT_PDB}" 1> /dev/null 2> /dev/null

# Count lines in the output PDB file
OUTPUT_LINES=$(wc -l < "${OUTPUT_PDB}")
echo "Output PDB file has ${OUTPUT_LINES} lines."

# Step G: Calculate the MD5 checksum of the output
echo "Calculating MD5 checksum of ${OUTPUT_PDB}..."
OUTPUT_MD5=$(md5sum "${OUTPUT_PDB}" | awk '{print $1}')
echo "Output PDB file md5sum value of '${OUTPUT_MD5}'"

# Step H: Compare the MD5 checksum to the expected value
if [ "$OUTPUT_MD5" == "$EXPECTED_MD5" ]; then
  echo "Test passed: MD5 checksum matches expected value."
else
  echo "Test failed: MD5 checksum does not match expected value."
  echo "Expected: $EXPECTED_MD5"
  echo "Actual:   $OUTPUT_MD5"
  exit 1
fi

echo "All tests completed successfully!"
