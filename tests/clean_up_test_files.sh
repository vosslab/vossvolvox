#!/bin/bash

set -euo pipefail

REPO_DIR="$(git rev-parse --show-toplevel)"

rm -fvr \
  "$REPO_DIR"/2LYZ*.pdb \
  "$REPO_DIR"/tests/2LYZ*.xyzr \
  "$REPO_DIR"/tests/2LYZ*.pdb \
  "$REPO_DIR"/tests/volume_results \
  "$REPO_DIR"/tests/pdb_to_xyzr_results
