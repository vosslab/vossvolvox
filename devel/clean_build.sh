#!/usr/bin/env bash
# clean_build.sh - light clean: wipe build output, tool caches, and test
# artifacts while KEEPING dependency installs (node_modules, Rust target/,
# SwiftPM checkouts) so no re-fetch is needed afterward. For Swift, a
# recompile IS expected on the next build; only the dependency re-fetch is
# avoided, mirroring `swift package clean` (compiled output only) rather than
# `swift package reset` (full wipe, including fetched checkouts).
#
# Front door: this is the everyday build cleaner, wired to `npm run clean` in
# TypeScript repos. Run directly as ./devel/clean_build.sh. For a deep reset that
# also removes node_modules, Rust target/, and SwiftPM's fetched checkouts (a
# distribution-clean checkout that re-fetches everything), use
# devel/dist_clean.sh instead. Both keep the committed package-lock.json.
#
# Universal across repo types (python, typescript, rust). Patterns that do not
# exist in a given repo are silently skipped via `nullglob` + an existence
# check, so no false-positive output.
set -euo pipefail
cd "$(git rev-parse --show-toplevel)"

DELETED=()

delete_path() {
	local p="$1"
	if [ -e "$p" ] || [ -L "$p" ]; then
		rm -rf "$p"
		DELETED+=("$p")
	fi
}

delete_find_matches() {
	local label="$1"
	shift
	local match
	while IFS= read -r -d '' match; do
		rm -rf "$match"
		DELETED+=("${match#./}")
	done < <(find . "$@" -print0)
}

# Generic build outputs (any language).
delete_path dist
delete_path dist-single
delete_path _site
delete_path build
delete_path out

# TypeScript / JS build artifacts and bundler metadata.
delete_path _bundle.js
delete_path meta.json
delete_path stats.html
delete_find_matches tsbuildinfo -type f -name '*.tsbuildinfo'

# JS/TS tool caches.
delete_path .cache
delete_path .eslintcache
delete_path .prettiercache
delete_path .nyc_output

# Xcode / Swift build outputs and metadata. Compiled output only: keeps
# .build/checkouts, .build/repositories, and .build/registry (SwiftPM's
# fetched dependency sources and cache), plus .build/workspace-state.json
# (the dependency resolution manifest, the only top-level .build/*.json file
# in current SwiftPM layouts -- a build-plan glob would wrongly sweep it up),
# so the next build recompiles without re-fetching. .swiftpm is per-user/IDE
# state, not build output, so it is also kept here (dist_clean.sh still
# removes both for a full reset). Mirrors `swift package clean` (compiled
# output only) rather than `swift package reset` (full wipe).
delete_path .build/debug
delete_path .build/release
delete_path .build/artifacts
delete_path .build/build.db
delete_find_matches swiftpmTriple -type d -path './.build/*-apple-macosx'
delete_find_matches swiftpmBuildPlan -type f -path './.build/*.yaml'
delete_path DerivedData
delete_find_matches xcresult -type d -name '*.xcresult'
delete_find_matches xcuserdata -type d -name 'xcuserdata'

# Test outputs (Playwright, coverage).
delete_path test-results
delete_path playwright-report
delete_path blob-report
delete_path coverage

# Python bytecode and tool caches (any depth).
delete_find_matches pycache -type d -name '__pycache__'
delete_find_matches pytest_cache -type d -name '.pytest_cache'
delete_find_matches mypy_cache -type d -name '.mypy_cache'
delete_find_matches ruff_cache -type d -name '.ruff_cache'

# Dependency installs (node_modules, Rust target/) and the committed
# package-lock.json are intentionally KEPT here. Use devel/dist_clean.sh for a
# full reset that also removes node_modules and target/.

if [ "${#DELETED[@]}" -eq 0 ]; then
	echo "Nothing to clean."
else
	echo "Cleaned ${#DELETED[@]} path(s):"
	for p in "${DELETED[@]}"; do
		echo "  $p"
	done
fi
