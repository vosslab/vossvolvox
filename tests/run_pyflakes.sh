#!/usr/bin/env bash
set -e

REPO_ROOT="$(git rev-parse --show-toplevel)"
cd "${REPO_ROOT}"

# Run pyflakes on all Python files and capture output
PYFLAKES_OUT="${REPO_ROOT}/pyflakes.txt"
find "${REPO_ROOT}" \
	-type d \( -name .git -o -name .venv -o -name old_shell_folder \) -prune -o \
	\( -type f -name "*.py" ! -path "*TEMPLATE*" -print0 \) \
	| sort -z \
	| xargs -0 pyflakes > "${PYFLAKES_OUT}" 2>&1 || true

RESULT=$(wc -l < "${PYFLAKES_OUT}")

N=5

# Success if no errors were found
if [ "${RESULT}" -eq 0 ]; then
    echo "No errors found!!!"
    exit 0
fi

shorten_paths() {
	sed -E 's|.*/([^/:]+:)|\1|'
}

summarize_errors() {
	awk '
		BEGIN {
			import=0; syntax=0; name=0; variable=0; warning=0; other=0
		}
		{
			line=$0
			if (line !~ /:[0-9]+:[0-9]+:/) {
				next
			}
			if (line ~ /imported but unused/ || line ~ /import \* used/ || line ~ /could not import/) {
				import++
			} else if (line ~ /syntax error/ || line ~ /SyntaxError/ || line ~ /invalid syntax/ \
				|| line ~ /Missing parentheses in call to/ || line ~ /Did you mean print/ \
				|| line ~ /from __future__ imports must occur at the beginning/ \
				|| line ~ /unexpected indent/ || line ~ /EOL while scanning string literal/ \
				|| line ~ /EOF while scanning triple-quoted string literal/ \
				|| line ~ /unterminated string literal/ || line ~ /cannot assign to/ \
				|| line ~ /cannot use .* as / || line ~ /invalid decimal literal/) {
				syntax++
			} else if (line ~ /undefined name/ || line ~ /undefined local/ || line ~ /undefined variable/) {
				name++
			} else if (line ~ /assigned to but never used/ || line ~ /referenced before assignment/ \
				|| line ~ /redefinition of unused/) {
				variable++
			} else if (line ~ /Warning:/ || line ~ /DeprecationWarning/ || line ~ /SyntaxWarning/) {
				warning++
			} else {
				other++
			}
		}
		END {
			print "Error categories"
			print "Import errors: " import
			print "Syntax errors: " syntax
			print "Name errors: " name
			print "Variable errors: " variable
			print "Warning errors: " warning
			print "Other errors: " other
		}
	' "${PYFLAKES_OUT}"
}

print_unclassified() {
	awk '
		BEGIN {
			printed=0
		}
		{
			line=$0
			if (line !~ /:[0-9]+:[0-9]+:/) {
				next
			}
			if (line ~ /imported but unused/ || line ~ /import \* used/ || line ~ /could not import/) {
				next
			} else if (line ~ /syntax error/ || line ~ /SyntaxError/ || line ~ /invalid syntax/ \
				|| line ~ /Missing parentheses in call to/ || line ~ /Did you mean print/ \
				|| line ~ /from __future__ imports must occur at the beginning/ \
				|| line ~ /unexpected indent/ || line ~ /EOL while scanning string literal/ \
				|| line ~ /EOF while scanning triple-quoted string literal/ \
				|| line ~ /unterminated string literal/ || line ~ /cannot assign to/ \
				|| line ~ /cannot use .* as / || line ~ /invalid decimal literal/) {
				next
			} else if (line ~ /undefined name/ || line ~ /undefined local/ || line ~ /undefined variable/) {
				next
			} else if (line ~ /assigned to but never used/ || line ~ /referenced before assignment/ \
				|| line ~ /redefinition of unused/) {
				next
			} else if (line ~ /Warning:/ || line ~ /DeprecationWarning/ || line ~ /SyntaxWarning/) {
				next
			}
			print line
			printed++
			if (printed >= 5) {
				exit
			}
		}
	' "${PYFLAKES_OUT}" | shorten_paths
}

echo ""
echo "First ${N} errors"
grep -E ':[0-9]+:[0-9]+:' "${PYFLAKES_OUT}" | head -n "${N}" | shorten_paths
echo "-------------------------"
echo ""

echo "Random ${N} errors"
grep -E ':[0-9]+:[0-9]+:' "${PYFLAKES_OUT}" | sort -R | head -n "${N}" | shorten_paths || true
echo "-------------------------"
echo ""

echo "Last ${N} errors"
grep -E ':[0-9]+:[0-9]+:' "${PYFLAKES_OUT}" | tail -n "${N}" | shorten_paths
echo "-------------------------"
echo ""

summarize_errors
echo "-------------------------"
echo ""

echo "Unclassified errors (up to ${N})"
print_unclassified
echo "-------------------------"
echo ""

echo "Found ${RESULT} pyflakes errors written to REPO_ROOT/pyflakes.txt"

# Fail if any errors were found
exit 1
