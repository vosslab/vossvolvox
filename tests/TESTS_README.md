# Tests

This folder holds three test tiers, each with its own execution model. Pytest runs the fast lane under 1 second; the other two tiers run directly and may take longer.

## Layout

```
tests/
  test_*.py              fast pytest unit/integration (collected by pytest)
  test_*.mjs             pure Node tests, no browser (rare)
  conftest.py            pytest config; declares collect_ignore
  conftest.py includes:  collect_ignore = ["e2e", "playwright"]
  playwright/            browser-driven tests (Playwright)
    test_*.mjs           smoke/layout/regression
    repo_root.mjs        shared helper: exports REPO_ROOT (centrally propagated)
    helpers.mjs          shared test utilities
    e2e/                 OPTIONAL: full-path browser walkthroughs
      test_*.mjs
  e2e/                   non-browser whole-system E2E (shell/Python)
    e2e_*.sh             shell orchestration
    e2e_*.py             Python orchestration
    run_all.sh           OPTIONAL: run all E2E tests at once
```

## How to run

- Fast pytest lane: `pytest tests/`
- Single browser test: `node tests/playwright/test_<name>.mjs` (TypeScript repos include `PLAYWRIGHT_USAGE.md` in their propagated `docs/` folder)
- Single non-browser E2E: `bash tests/e2e/e2e_<name>.sh` or `source source_me.sh && python3 tests/e2e/e2e_<name>.py` (see [../docs/E2E_TESTS.md](../docs/E2E_TESTS.md))
- Bulk non-browser E2E: `bash tests/e2e/run_all.sh` (if present; in the template repo the reset harness runner lives at `tests/meta/e2e/run_all.sh`)

## Why two folders for E2E

Playwright is a tool; E2E is a scope. Not every Playwright test is end-to-end (a layout check or single-interaction smoke test is browser-driven but not E2E). One folder per execution model:

- `tests/playwright/` -- browser-driven tests (Playwright; future tools like Cypress would get their own tool-named folder)
- `tests/e2e/` -- non-browser whole-system orchestration (CLIs, build pipelines, multi-suite runners)

The optional `tests/playwright/e2e/` subfolder groups full-path browser walkthroughs separately from smoke tests and regression checks.

## How pytest stays fast

`tests/conftest.py` declares `collect_ignore = ["e2e", "playwright"]`, so pytest never collects test functions from those subtrees, regardless of filename inside them. The filename conventions (`e2e_*` prefix in `tests/e2e/`, `test_*.mjs` for Playwright) are a readability layer on top of this active guard.

Important: `collect_ignore` only affects pytest test collection. The repo's lint tests (ASCII compliance, whitespace, pyflakes, indentation, shebangs, etc.) enumerate files via `git ls-files` and still scan files inside `tests/playwright/` and `tests/e2e/`. A non-ASCII character in `tests/playwright/foo.mjs` will still fail the ASCII check - only execution as a pytest test is suppressed.

## Hygiene file discovery

Enumerating hygiene tests (ascii, whitespace, pyflakes, shebangs, and similar) get their file
list from one shared helper, `file_utils.discover_files`. It is the canonical discovery API
and owns git scope selection, absolute-path join, dedupe, built-in scratch/directory filtering,
extension filtering, the `isfile` check, and the sort. Use `file_utils.discover_files` as the
single source of file discovery; the shared `SKIP_DIRS` and `path_has_skip_dir` live only in
`file_utils.py`.

Signature:

```python
discover_files(extensions=None, extra_filter=None, *, test_key=None, repo_root=None) -> list[str]
```

`test_key` and `repo_root` are keyword-only (note the bare `*`). The module-level discovered list
in a hygiene test is named `FILES` (not `_FILES`).

Three contracts:

- Returns ABSOLUTE paths, sorted ascending.
- `extra_filter` receives a REPO-RELATIVE POSIX path (for example `tests/foo.py`) and returns
  `True` to keep the file. `None` keeps all files.
- `extensions=None` means all files; otherwise extension match is case-insensitive (pass
  lowercase suffixes like `(".py",)`).

Normal hygiene tests call `discover_files(extensions=..., test_key="<stem>")`; `discover_files`
resolves the repo root itself via `get_repo_root()` (a negligible extra call). Pass `repo_root=`
only in `file_utils` regression tests that point discovery at a temporary directory.

Exclusions come from three layers, in order:

- Layer 1, universal exclusions (vendored, `file_utils.py`): built-in skipped directories,
  `_temp*` scratch files/directories, and `dist_*/` scratch build directories, handled by
  `path_has_skip_dir`.
- Layer 2, `REPO_HYGIENE_FILTERS` (repo-local, `conftest.py`): per-test repo-local file/glob
  exclusions, keyed by `"all"` or a vendored test key, as lists of repo-relative POSIX glob
  patterns matched with `fnmatch.fnmatchcase`. This is the only home for repo-specific exclusions,
  because `conftest.py` survives propagation while vendored files do not. A test key is the test
  filename stem without the `test_` prefix; recursive subtrees need an explicit `/**`.
- Layer 3, `extra_filter` (vendored call site): a universal per-test SELECTION mechanism only.
  Keep all repo-specific exclusions in `conftest.py REPO_HYGIENE_FILTERS`; vendored files hold
  only universal logic.

A normal hygiene test calls `discover_files` with its `test_key`:

```python
# Normal hygiene test:
FILES = file_utils.discover_files(extensions=(".py",), test_key="ascii_compliance")

# Repo-local conftest.py declaring repo-specific exclusions:
REPO_HYGIENE_FILTERS = {
	"all": ["temp_scripts/**", "TEMPLATE.py"],
	"ascii_compliance": ["human_readable-*.html"],
}
```

Pass `repo_root=` in `file_utils` regression tests that point discovery at a temporary
directory:

```python
# Regression test: point discovery at a controlled temporary root.
result = file_utils.discover_files(extensions=(".py",), repo_root=tmp_root)
```

### Additional helpers in file_utils.py

Shared helpers that complement `discover_files`:

- `iter_imports(tree: ast.Module)` -- yields every `ast.Import` and `ast.ImportFrom` node from
  a parsed module tree. Use in import-checking tests instead of local AST-walk loops.
- `rel_to_root(path, repo_root=None)` -- returns a repo-relative POSIX string suitable for
  parametrize ids and assertion messages (for example `tests/foo.py`).
- `rel_id(abs_path: str) -> str` -- thin wrapper around `rel_to_root` for use as
  `ids=file_utils.rel_id` in `@pytest.mark.parametrize`.
- `run_fixer_script(script_name, target)` -- shared subprocess wrapper: runs
  `tests/<script_name> -i target` and returns `(returncode: int, stderr: str)` for every
  subprocess completion. Never raises on a non-zero exit code; callers inspect the return
  value and decide what each exit code means. Raises `RuntimeError` only for environment
  preconditions: script file not found (checked via `os.path.isfile` before launch), or
  `FileNotFoundError` from a missing Python interpreter (re-raised as `RuntimeError`
  stating the test environment is broken). Exit-code contracts for shipped fixer scripts:
  `fix_ascii_compliance.py` exits 0 = clean, 1 = issues remain, 2 = auto-fixed;
  `fix_whitespace.py` exits 0 = clean or fixed, 1 = missing or no input. Used by the
  ASCII and whitespace auto-fix tests.
- `collect_file_violations(files, check)` -- iterate `files`, call `check(rel)` per file,
  return `dict[str, list[str]]` of violations keyed by repo-relative POSIX path. Use when
  the checker handles its own parsing (for example pyflakes).
- `collect_python_violations(files, check)` -- like `collect_file_violations` but parses each
  `.py` file into an AST once; calls `check(rel, tree)`; records one `SyntaxError` entry when
  parsing fails and skips that file's rule checks.
- `format_violation_report(header, violations_by_file)` -- return a `list[str]` summary lines
  for a report file; returns `[]` when `violations_by_file` is empty (clean run).
- `format_violation_assert_message(rel, lines, report_name)` -- return a
  human-readable assertion failure message for the per-file violation lines (`lines: list[str]`);
  evaluated only on failure so no overhead on passing cases.
- `write_report_lines(report_name: str, lines: list[str]) -> str` -- write the full report when
  `lines` is non-empty (truncate-write, one `\n` per line, one trailing `\n`). Called only when
  violations exist; `clear_stale_reports` owns removal of stale clean-run reports.
- `clear_stale_reports() -> None` -- delete all `report_*.txt` files at the repo root; guarded
  once per process so multiple hygiene modules running in the same pytest session each trigger
  it but only the first invocation does the filesystem work.
- `report_name(test_file: str) -> str` -- derive the canonical report filename from a test module
  path. Pass `__file__` and get back the matching `report_<stem>.txt` name (for example
  `report_name(__file__)` in `test_bandit_security.py` returns `"report_bandit_security.txt"`).
  Every hygiene test sets `REPORT_NAME = file_utils.report_name(__file__)` so the name is always
  derived from the filename, never hardcoded.

See [../docs/PYTEST_STYLE.md](../docs/PYTEST_STYLE.md) "Hygiene report files" for the canonical
module shape and report lifecycle.

### Hygiene guard tests

Two vendored hygiene tests keep the discovery scaffold clean:

- `tests/test_function_typing.py` -- AST-based guard that enforces the typing rule repo-wide:
  the `typing` module is not used, and every `def` carries param and return type annotations.
  Use builtin generics (`list`, `dict`, `tuple`, `set`) and PEP 604 unions (`X | None`).
  Use `collections.abc` (for example `collections.abc.Callable`) for callable and iterable params.
- `tests/test_pytest_hygiene.py` -- AST guard ensuring hygiene tests keep all file-discovery
  logic in `file_utils` (the shared scratch/directory filter, `SKIP_DIRS`, `path_has_skip_dir`,
  and `gather_*` discovery live there). See the "discovery lives in file_utils" guidance above.

## See also

- [../docs/PYTEST_STYLE.md](../docs/PYTEST_STYLE.md) -- pytest test-writing rules and fast-lane discipline
- [../docs/E2E_TESTS.md](../docs/E2E_TESTS.md) -- non-browser whole-system E2E conventions
- Browser-driven test conventions: TypeScript repos include `PLAYWRIGHT_USAGE.md` in their propagated `docs/` folder
