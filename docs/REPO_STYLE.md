# REPO_STYLE.md

Repo-wide conventions for this project and related repos.

## Repository structure
- Prefer small, single-purpose scripts at the repo root.
- Create topic folders only when a collection needs grouping.
- Avoid deep nesting; keep paths short.
- Keep `README.md` and `AGENTS.md` at the repo root.
- Determine REPO_ROOT with `git rev-parse --show-toplevel`, not by deriving paths from the current working directory.

## Naming
- Use SCREAMING_SNAKE_CASE for Markdown docs filenames, with the .md extension
- For non-Markdown filenames, use only lowercase ASCII letters, numbers, and underscores.
- Prefer snake_case for most filenames. Avoid CamelCase in filenames.
- Use underscores between words and avoid spaces.
- Use `.md` for docs, `.sh` for shell, `.py` for Python.
- Keep filenames descriptive, and consistent with the primary thing the file provides.

## Git moves, renames, and index locks
- Use `git mv` for all renames and moves.
- Do not use `mv` plus add/remove as a fallback. Do not use `git rm` unless deleting a file permanently.
- Before any index-writing Git command (including `git mv`, `git add`, `git rm`, `git checkout`, `git switch`, `git restore`, `git merge`, `git rebase`, `git reset`, `git commit`), verify `.git` is writable by the current user. If not, stop and report a permissions error.
- If `.git/index.lock` exists:
  - Do not modify files and do not run Git commands. Stop and report:
    - lock owner, permissions, and age (mtime)
    - process holding the lock, if detectable (for example, `lsof .git/index.lock`)
  - If a process holds the lock, report an active concurrent Git operation.
  - If no process holds the lock and the lock age is > 5 minutes, report a likely stale lock. Do not delete it automatically.
- If any Git command fails with an index lock error (cannot create `.git/index.lock`), stop immediately. Do not retry and do not fall back to `mv`.
- Error report must include: the command run and full stderr, plus a short next step: close other Git processes, remove a stale lock only if no process holds it, or fix `.git` permissions.

## Versioning
- Prefer `pyproject.toml` as the single source of truth when the repo is a single Python package with a single `pyproject.toml`.
- If the repo contains multiple Python packages (multiple `pyproject.toml` files), keep package versions in sync across all `pyproject.toml` files. Unless otherwise stated.
- Maintain a REPO_ROOT/`VERSION` file as well that is sync'd with the `pyproject.toml` version.
- Store the version under `[project] version`.
- Prefer CalVer-style zero-padded year/zero-padded month versioning for new releases, formatted as `0Y.0M.PATCH` (for example `25.02.3rc1`). See https://calver.org/
- Use PEP 440 pre-release tags when needed: `aN` for alpha, `bN` for beta, and `rcN` for release candidates.
- When PATCH == 0, use shorthand `25.02b1` instead of `25.02.0b1`
- Prefer zero-padded 0Y.0M for readability and lexicographic sorting. Packaging tools may normalize 25.02.* to 25.2.*; this does not affect version ordering.
- Reference: [PyPA version specifiers](https://packaging.python.org/en/latest/specifications/version-specifiers/).

## Scripts and executables
- Keep scripts self-contained and single-purpose.
- Add a shebang for executable scripts and keep them runnable directly.
- Document shared helpers and modules in `docs/USAGE.md` when used across scripts.
- Use `tests/test_pyflakes.py` and `tests/test_ascii_compliance.py` for repo-wide lint checks, with `tests/check_ascii_compliance.py` for single-file ASCII/ISO-8859-1 checks and `tests/fix_ascii_compliance.py` for single-file fixes.
- For smoke tests, reuse stable output folder names (for example `output_smoke/`) instead of creating one-off output directory names; reusing/overwriting avoids repeated delete-approval prompts.

## Dependency manifests
- Store Python standard dependencies in `pip_requirements.txt` at the repo root and developer dependencies, e.g., pytest in `pip_requirements-dev.txt`.
- Use `pip_requirements.txt` not `requirements.txt` for clarity reasons
- Store Homebrew packages in `Brewfile` at the repo root.
- Use per-subproject manifests only when a subfolder is a standalone project.
- Document non-default system dependencies in `docs/INSTALL.md`.
- In general, we want to require all dependencies, rather than provide work-arounds if they are mssing, because without all the dependencies the program is too crippled to run properly

## Data and outputs
- Keep generated outputs out of git unless they are small and intentional.
- Put large inputs or outputs under a clear folder (for example `data/` or `output/`).
- Note input and output locations in `docs/USAGE.md`.
- Keep sample inputs small and safe.

## Documentation
- Keep repo docs in `docs/` unless a file is explicitly root-level.
- Keep docs current. Remove or replace stale docs.
- Use SCREAMING_SNAKE_CASE for Markdown docs filenames, with the .md extension
- Apply the ALL CAPS rule to files under docs/ (for example docs/INSTALL.md).
- Use underscores between words and avoid spaces.
- Choose clear, descriptive names.
- Keep well-known root-level docs (for example VERSION, README.md, AGENTS.md).
- I prefer to use social media links instead of hard coding my email in repos. For example, Neil Voss, https://bsky.app/profile/neilvosslab.bsky.social
- When referencing files, use Markdown links so users can click through. Markdown links are created using the syntax [link text](URL), where "link text" is the clickable text that appears in the document, and "URL" is the web address or file path the link points to. This allows users to navigate between different content easily. Use file-path link text so readers know the exact filename (good: [docs/MARKDOWN_STYLE.md](docs/MARKDOWN_STYLE.md), bad: [Style Guide for Markdown](docs/MARKDOWN_STYLE.md)). Only include a backticked path when the link text is not the path.


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

- Most source code is licensed under **GPLv3**, unless stated otherwise.
- Libraries intended for use by proprietary or mixed-source software are licensed under **LGPLv3**.
- Non-code creative works, including text and figures, are licensed under **CC BY-SA 4.0**. Commercial use is permitted.

- Code and non-code materials are licensed separately to reflect different legal and practical requirements.
