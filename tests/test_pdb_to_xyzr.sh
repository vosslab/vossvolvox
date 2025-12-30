#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
PDB_ID="${1:-2LYZ}"
RESULT_DIR="${SCRIPT_DIR}/pdb_to_xyzr_results/${PDB_ID}"
mkdir -p "${RESULT_DIR}"
echo "Staging files under ${RESULT_DIR}" >&2
OUTPUT_DIR="${RESULT_DIR}"
XYZR_DIR="${REPO_DIR}/xyzr"
BIN_DIR="${REPO_DIR}/bin"

download_pdb() {
	local pdb="$1"
	local pdb_file="${RESULT_DIR}/${pdb}.pdb"
	if [ -f "${pdb_file}" ]; then
		echo "Reusing cached ${pdb_file}" >&2
		return 0
	fi
	echo "Downloading ${pdb}..." >&2
	local gz_file="${pdb_file}.gz"
	curl -s -L -o "${gz_file}" "https://files.rcsb.org/download/${pdb}.pdb.gz"
	if [ ! -f "${gz_file}" ]; then
		echo "Download failed for ${pdb}" >&2
		return 1
	fi
	echo "Unpacking ${gz_file}" >&2
	gunzip -f "${gz_file}"
	if [ ! -f "${pdb_file}" ]; then
		echo "Decompression failed for ${pdb}" >&2
		return 1
	fi
}

prepare_input() {
	local pdb="$1"
	local pdb_file="${RESULT_DIR}/${pdb}.pdb"
	local filtered="${RESULT_DIR}/${pdb}-noions.pdb"
	if ! download_pdb "${pdb}"; then
		return 1
	fi
	echo "Filtering ATOM records into ${filtered}" >&2
	grep -E "^ATOM  " "${pdb_file}" > "${filtered}"
	echo "${filtered}"
}

INPUT_PDB=$(prepare_input "${PDB_ID}") || {
	echo "Failed to prepare input for ${PDB_ID}" >&2
	exit 1
}

IMPLEMENTATIONS=(
  "cpp::${BIN_DIR}/pdb_to_xyzr.exe -i"
  "python::python3 ${XYZR_DIR}/pdb_to_xyzr.py"
  "sh::${XYZR_DIR}/pdb_to_xyzr.sh"
)

KEYS=()
DURATIONS=()
MD5S=()
OUTPUTS=()
LINE_COUNTS=()

if [ ! -f "${INPUT_PDB}" ]; then
  echo "Input PDB '${INPUT_PDB}' not found." >&2
  exit 1
fi

hash_cmd() {
  local file="$1"
  if command -v md5sum >/dev/null 2>&1; then
    md5sum "${file}" | awk '{print $1}'
  else
    md5 -q "${file}"
  fi
}

for entry in "${IMPLEMENTATIONS[@]}"; do
  key="${entry%%::*}"
  cmd="${entry#*::}"
  if [[ "${cmd}" == *"pdb_to_xyzr.py"* && ! -f "${XYZR_DIR}/pdb_to_xyzr.py" ]]; then
    echo "Skipping python implementation; script not found." >&2
    continue
  fi
  if [[ "${cmd}" == *"pdb_to_xyzr.sh"* && ! -f "${XYZR_DIR}/pdb_to_xyzr.sh" ]]; then
    echo "Skipping shell implementation; script not found." >&2
    continue
  fi
  if [[ "${cmd}" == "${BIN_DIR}/pdb_to_xyzr.exe"* && ! -x "${BIN_DIR}/pdb_to_xyzr.exe" ]]; then
    echo "Skipping C++ implementation; executable not found." >&2
    continue
  fi
  output="${OUTPUT_DIR}/${key}.xyzr"
  echo "Running ${key} converter -> ${output}" >&2
  start_ns=$(date +%s%N)
  if ! eval "${cmd} \"${INPUT_PDB}\" > \"${output}\""; then
    echo "Failed: ${cmd}" >&2
    continue
  fi
  end_ns=$(date +%s%N)
  elapsed_ms=$(( (end_ns - start_ns)/1000000 ))
  md5=$(hash_cmd "${output}")
  line_count=$(wc -l < "${output}")
  KEYS+=("${key}")
  DURATIONS+=("${elapsed_ms}")
  MD5S+=("${md5}")
  OUTPUTS+=("${output}")
  LINE_COUNTS+=("${line_count}")
done

echo ""
printf "%-8s %-8s %-10s %s\n" "Impl" "Lines" "Duration" "MD5"
printf "%-8s %-8s %-10s %s\n" "----" "-----" "--------" "---"
for ((idx=0; idx<${#KEYS[@]}; idx++)); do
  printf "%-8s %-8s %-10s %s\n" "${KEYS[$idx]}" "${LINE_COUNTS[$idx]}" "${DURATIONS[$idx]}ms" "${MD5S[$idx]}"
done

diff_lines() {
  python3 - "$1" "$2" <<'PY'
import sys
from itertools import zip_longest
ref = open(sys.argv[1]).read().splitlines()
other = open(sys.argv[2]).read().splitlines()
diff = sum(1 for a, b in zip_longest(ref, other, fillvalue=None) if a != b)
print(diff)
PY
}

baseline_key="sh"
baseline_file=""
for ((idx=0; idx<${#KEYS[@]}; idx++)); do
  if [ "${KEYS[$idx]}" = "${baseline_key}" ]; then
    baseline_file="${OUTPUTS[$idx]}"
    break
  fi
done

if [ -n "${baseline_file}" ]; then
  echo ""
  echo "Line differences vs ${baseline_key}.xyzr:"
  printf "%-8s %s\n" "Impl" "DifferingLines"
  printf "%-8s %s\n" "----" "--------------"
  for ((idx=0; idx<${#KEYS[@]}; idx++)); do
    key="${KEYS[$idx]}"
    file="${OUTPUTS[$idx]}"
    if [ "${key}" = "${baseline_key}" ]; then
      continue
    fi
    diff_count=$(diff_lines "${baseline_file}" "${file}")
    printf "%-8s %s\n" "${key}" "${diff_count}"
  done
else
  echo ""
  echo "Baseline (${baseline_key}) output missing; skipping line comparison."
fi
