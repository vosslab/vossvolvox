#!/bin/bash

set -euo pipefail

REPO_DIR="$(git rev-parse --show-toplevel)"
echo "Repository root: $REPO_DIR"

echo "Cleaning test-generated files"
"$REPO_DIR/tests/clean_up_test_files.sh"

echo "Removing .DS_Store files"
find "$REPO_DIR" -name ".DS_Store" -exec rm -vf {} +

echo "Removing stale git index lock if present"
rm -vf "$REPO_DIR/.git/index.lock"

echo "Running make clean in src"
make -C "$REPO_DIR/src" distclean

echo "Cleanup complete"
