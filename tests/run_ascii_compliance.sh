#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(git rev-parse --show-toplevel)"
cd "${REPO_ROOT}"

PYTHON_BIN="/opt/homebrew/opt/python@3.12/bin/python3.12"
SCRIPT_PATH="${REPO_ROOT}/tests/check_ascii_compliance.py"
ASCII_OUT="${REPO_ROOT}/ascii_compliance.txt"
PYFLAKES_OUT="${REPO_ROOT}/pyflakes.txt"

: > "${ASCII_OUT}"

find "${REPO_ROOT}" \
	-type d \( -name .git -o -name .venv -o -name old_shell_folder \) -prune -o \
	-type f \( \
		-name "*.md" -o \
		-name "*.txt" -o \
		-name "*.py" -o \
		-name "*.js" -o \
		-name "*.jsx" -o \
		-name "*.ts" -o \
		-name "*.tsx" -o \
		-name "*.html" -o \
		-name "*.htm" -o \
		-name "*.css" -o \
		-name "*.json" -o \
		-name "*.yml" -o \
		-name "*.yaml" -o \
		-name "*.toml" -o \
		-name "*.ini" -o \
		-name "*.cfg" -o \
		-name "*.conf" -o \
		-name "*.sh" -o \
		-name "*.bash" -o \
		-name "*.zsh" -o \
		-name "*.fish" -o \
		-name "*.csv" -o \
		-name "*.tsv" -o \
		-name "*.xml" -o \
		-name "*.svg" -o \
		-name "*.sql" -o \
		-name "*.rb" -o \
		-name "*.php" -o \
		-name "*.java" -o \
		-name "*.c" -o \
		-name "*.h" -o \
		-name "*.cpp" -o \
		-name "*.hpp" -o \
		-name "*.go" -o \
		-name "*.rs" -o \
		-name "*.swift" \
	\) \
	! -path "${ASCII_OUT}" \
	! -path "${PYFLAKES_OUT}" \
	-print0 \
	| sort -z \
	| while IFS= read -r -d '' file; do
		"${PYTHON_BIN}" "${SCRIPT_PATH}" -i "${file}" 2>> "${ASCII_OUT}" || true
	done

RESULT=$(wc -l < "${ASCII_OUT}")
N=5

if [ "${RESULT}" -eq 0 ]; then
	echo "No errors found!!!"
	exit 0
fi

shorten_paths() {
	sed -E 's|.*/([^/:]+:)|\1|'
}

echo ""
echo "First ${N} errors"
grep -E ':[0-9]+:[0-9]+:' "${ASCII_OUT}" | head -n "${N}" | shorten_paths
echo "-------------------------"
echo ""

echo "Random ${N} errors"
grep -E ':[0-9]+:[0-9]+:' "${ASCII_OUT}" | sort -R | head -n "${N}" | shorten_paths || true
echo "-------------------------"
echo ""

echo "Last ${N} errors"
grep -E ':[0-9]+:[0-9]+:' "${ASCII_OUT}" | tail -n "${N}" | shorten_paths
echo "-------------------------"
echo ""

echo "Found ${RESULT} ASCII compliance errors written to REPO_ROOT/ascii_compliance.txt"

exit 1
