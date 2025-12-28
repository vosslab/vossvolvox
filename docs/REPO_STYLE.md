# REPO_STYLE.md

Repo-wide conventions for this project and related repos.

## Repository structure
- Prefer small, single-purpose scripts at the repo root.
- Create topic folders only when a collection needs grouping.
- Avoid deep nesting; keep paths short.
- Keep `README.md` and `AGENTS.md` at the repo root.

## Naming
- Use lowercase ascii filenames with underscores for most files; avoid spaces; 
- Match script names to their primary purpose.
- Use `.md` for docs, `.sh` for shell, `.py` for Python.

## Scripts and executables
- Keep scripts self-contained and single-purpose.
- Add a shebang for executable scripts and keep them runnable directly.
- Document shared helpers and modules in `docs/USAGE.md` when used across scripts.

## Dependency manifests
- Store Python dependencies in `pip_requirements.txt` at the repo root. 
- Use `pip_requirements.txt` not `requirements.txt` for clarity reasons
- Store Homebrew packages in `Brewfile` at the repo root.
- Use per-subproject manifests only when a subfolder is a standalone project.
- Document non-default system dependencies in `docs/INSTALL.md`.

## Data and outputs
- Keep generated outputs out of git unless they are small and intentional.
- Put large inputs or outputs under a clear folder (for example `data/` or `output/`).
- Note input and output locations in `docs/USAGE.md`.
- Keep sample inputs small and safe.

## Documentation
- Keep repo docs in `docs/` unless a file is explicitly root-level.
- Keep docs concise and current; remove stale docs when replacing them.

Use ALL CAPS for all Markdown documentation filenames (*.md) so docs visually stand out from code and scripts at a glance. Use underscores between words, avoid spaces, and choose clear, descriptive names. Keep well-known root-level docs in their conventional forms (for example README.md, AGENTS.md), and apply the same ALL CAPS rule to files under docs/ (for example docs/INSTALL.md, docs/USAGE.md).

### Recommended common docs
- `AGENTS.md`: agent instructions, tool constraints, and repo-specific workflow guardrails.
- `README.md`: project purpose, quick start, and links to deeper documentation.
- `LICENSE`: legal terms for using and redistributing the project; keep exact license text.
- `docs/CHANGELOG.md`: chronological, user facing record of changes, grouped by date. Timeline of what changed and when.
- `docs/CODE_ARCHITECTURE.md`: high-level system design, major components, and data flow.
- `docs/FILE_STRUCTURE.md`: directory map with what belongs where, including generated assets.
- `docs/INSTALL.md`: setup steps, dependencies, and environment requirements.
- `docs/NEWS.md`: curated release highlights and announcements, not a full changelog.
- `docs/RELATED_PROJECTS.md`: sibling repos, shared libraries, and integration touchpoints.
- `docs/RELEASE_HISTORY.md`: organized log of released versions and their release dates. Summarizes notable shipped qualities, including notes, major fixes, and compatibility notes.
- `docs/ROADMAP.md`: planned work, priorities, and what is intentionally not started.
- `docs/TODO.md`: backlog scratchpad for small tasks without timelines.
- `docs/TROUBLESHOOTING.md`: known issues, fixes, and debugging steps with symptoms.
- `docs/USAGE.md`: how to run the tool, CLI flags, and practical examples.

### Centrally maintained docs, do not edit locally
- `docs/AUTHORS.md`: primary maintainers and notable contributors
- `docs/MARKDOWN_STYLE.md`: Markdown writing rules and formatting conventions for this repo.
- `docs/PYTHON_STYLE.md`: Python formatting, linting, and project-specific conventions.
- `docs/REPO_STYLE.md`: repo-level organization, conventions, and file placement rules.

### Less common but acceptable
- `docs/COOKBOOK.md`: extended, real-world scenarios that build on usage docs.
- `docs/DEVELOPMENT.md`: local dev workflows, build steps, and release process.
- `docs/FAQ.md`: short answers to common questions and misconceptions.

### File I/O
Possible examples:
- `docs/INPUT_FORMATS.md`: supported input formats, required fields, and validation rules.
- `docs/OUTPUT_FORMATS.md`: generated outputs, schemas, naming rules, and destinations.
- `docs/FILE_FORMATS.md`: combined reference for input and output formats when one doc is clearer.
- `docs/YAML_FILE_FORMAT.md`: YAML schema, examples, and validation requirements.

### Docs not to use
- `CONTRIBUTING.md`: probably better under the DEVELOPMENT.md page
- `CODE_OF_CONDUCT.md`: avoid adding unless project scope changes and it will be maintained.
- `COMMUNITY.md`: avoid adding; this repo does not run a community program.
- `ISSUE_TEMPLATE.md`: avoid adding; this repo does not use GitHub issue templates here.
- `PULL_REQUEST_TEMPLATE.md`: avoid adding; we are not using GitHub PR templates here.
- `SECURITY.md`: avoid adding unless security reporting is formally supported.

### Repo-specific docs are always encouraged
- `docs/CONTAINER.md`: container image details, build steps, and run commands.
- `docs/ENGINES.md`: supported external engines/services and how to select them.
- `docs/EMWY_YAML_v2_SPEC.md`: specification for the EMWY YAML v2 format with examples.
- `docs/MACOS_PODMAN.md`: macOS-specific Podman setup steps and known issues.
- `docs/QUESTION_TYPES.md`: catalog of question types with expected fields and behavior.

## Licensing
Check the license file to match these criteria.

- Source code is licensed under **GPLv3**, unless stated otherwise.
- Libraries intended for use by proprietary or mixed-source software are licensed under **LGPLv3**.
- Non-code creative works, including text and figures, are licensed under **CC BY-SA 4.0**. Commercial use is permitted.

- Code and non-code materials are licensed separately to reflect different legal and practical requirements.
