# Agent instructions

Read before working:
- docs/REPO_STYLE.md
- docs/PYTHON_STYLE.md
- docs/MARKDOWN_STYLE.md
- docs/DEVELOPMENT.md
- docs/CODE_ARCHITECTURE.md
- docs/FILE_STRUCTURE.md
- docs/CHANGELOG.md

## Non-default constraints
- Do not edit src/lib/utils-main-legacy.cpp or src/volume-legacy.cpp.
- Run Python only as /opt/homebrew/opt/python@3.12/bin/python3.12; modules are in /opt/homebrew/lib/python3.12/site-packages/.
- For code changes, run the affected make target and closest shell test; documentation-only changes need no test.
- Only humans run git commit; update docs/CHANGELOG.md for user-facing behavior changes.
