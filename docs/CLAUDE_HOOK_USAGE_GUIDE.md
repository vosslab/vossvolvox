# Claude hook usage guide

Best practices for AI agents working in repos that use the `claude-code-permissions-hook`.
This guide covers what commands are allowed, denied, and passed through, along with
preferred alternatives for denied patterns.

This doc is Claude-specific and does not apply to Codex.

## Trust model

The hook optimizes for high task completion with bounded blast radius: allow routine
local work, deny/steer on machine-changing actions, prompt on high-impact operations.

## Overview

The permissions hook intercepts every Claude Code tool call and evaluates it against
TOML config rules. Each call gets one of three outcomes:

| Outcome | Meaning |
| --- | --- |
| **Allow** | Tool call proceeds automatically |
| **Deny** | Tool call is blocked with an error message |
| **Passthrough** | Falls back to Claude Code's default permission flow (user prompt) |

### Command decomposition

The hook splits compound Bash commands (`&&`, `||`, `;`, pipes) into leaf sub-commands
and checks each leaf independently:

- **Deny**: if ANY leaf matches a deny rule, the entire command is denied
- **Allow**: ALL leaves must match an allow rule for the command to be allowed
- **Passthrough**: if any leaf has no matching rule (and none are denied)

The hook also unwraps `bash -c "..."` patterns and extracts commands inside `$(...)`.

Environment-variable assignments (e.g., `NODE_PATH=/foo`) are stripped from leaf
commands by the decomposer, so `NODE_PATH=/foo node script.js` is evaluated as
just `node script.js`.

### Max chain length

Commands with more than **5** chained sub-commands are denied automatically. Break
long chains into smaller commands or write a script file.

## Allowed commands

### Python

```bash
source source_me.sh && python3 script.py
python3 -m pytest tests/test_foo.py
pytest tests/test_foo.py -k test_name
pyflakes script.py
```

Always use `source source_me.sh && python3` for running Python. Direct `python3`
invocations also work. Command substitution (`` ` `` or `$(...)`) is blocked in
Python commands.

### Git

Allowed git subcommands:

`add`, `branch`, `check-ignore`, `checkout`, `diff`, `ls-files`, `ls-tree`,
`log`, `mv`, `pull`, `remote`, `restore`, `rev-parse`, `show`, `status`, `worktree`

The `-C <path>` flag is supported before the subcommand.

```bash
git status
git diff --staged
git log --oneline -10
git -C /path/to/repo status
git mv old_name.py new_name.py
```

### Cargo

All cargo subcommands are allowed except: `publish`, `yank`, `login`, `logout`,
`owner`, `install`. Command substitution is blocked.

```bash
cargo build
cargo test
cargo clippy -- -D warnings
cargo fmt --check
```

### Shell scripts

```bash
bash script.sh
bash -n script.sh        # syntax check only
./script.sh
./script.py
./subdir/script.py
tools/runner.py          # bare relative-path scripts
scripts/build.sh
```

### Safe utilities

These commands are allowed as single commands. Command substitution is blocked.

**File and text processing:**
`awk`, `cat`, `colordiff`, `comm`, `cut`, `diff`, `expand`, `file`, `fmt`, `fold`,
`grep`, `head`, `jq`, `mediainfo`, `nl`, `od`, `paste`, `pdftotext`, `rg`, `sed`,
`seq`, `shuf`, `sort`, `tac`, `tail`, `tee`, `tr`, `unexpand`, `uniq`, `wc`, `xargs`

**Filesystem navigation:**
`basename`, `cd`, `chmod`, `cp`, `dirname`, `du`, `df`, `ls`, `lsof`, `mkdir`,
`mktemp`, `open`, `readlink`, `realpath`, `stat`, `tar`, `touch`, `tree`, `type`,
`unzip`, `which`

**Process and system info:**
`curl`, `date`, `echo`, `env`, `export`, `expr`, `false`, `id`, `ln`, `md5`,
`nproc`, `numfmt`, `pgrep`, `pkill`, `printenv`, `printf`, `ps`, `pwd`,
`screencapture`, `sleep`, `source`, `test`, `timeout`, `true`, `tty`, `uname`,
`unlink`, `wc`, `whoami`, `xcrun`, `xxd`

Note: Some of these (like `cat`, `grep`, `head`, `tail`) have deny rules that block
them when used with file path arguments. See the denied commands section. Use the
dedicated tools (Read, Grep, Glob) instead.

### Local runtimes

**Node.js:**
`node` is allowed for running `.js`, `.mjs`, and `.cjs` files, syntax checking with
`-c` or `--check`, inline evaluation with `-e` or `--eval`, and `--version` queries.

```bash
node script.js
node -c script.js
node -e "require('./data.json')"
node --version
```

**npx (whitelisted packages):**
`npx` is allowed for a whitelist of known-safe local dev tool packages: `tsc`,
`eslint`, `prettier`, `playwright`, `esbuild`. Unknown packages still require
user approval (passthrough).

```bash
npx tsc --noEmit              # allowed
npx eslint src/               # allowed
npx prettier --check .        # allowed
npx playwright screenshot ... # allowed
npx esbuild src/x.ts          # allowed
npx some-package              # requires approval
```

If `npx tsc` fails because TypeScript is not installed, stop and tell the user
to run `npm install --save-dev typescript` (or `npm install -g typescript` for
a global install). Do not work around the failure by calling
`./node_modules/.bin/tsc` or `node_modules/typescript/bin/tsc` directly --
those paths are denied (see "Denied commands" below).

**eslint and prettier (direct):**
`eslint` and `prettier` are allowed as direct commands for linting and formatting.

```bash
eslint src/app.ts
prettier --write src/
```

**Deno:**
`deno` is allowed for `run`, `check`, `fmt`, `lint`, and `test` subcommands,
plus `--version` queries. Remote URLs and `deno eval` are not auto-allowed.

```bash
deno run script.ts
deno check script.ts
deno fmt --check
deno lint
deno test
deno --version
```

### Podman (containers)

Read-only inspection, build, compose, lifecycle, and exec are allowed:

```bash
podman ps
podman pod ls
podman images
podman logs web
podman inspect web
podman info
podman build -t foo .
podman compose up -d
podman compose down
podman start web
podman stop web
podman restart web
podman exec web ls /var/www
podman exec web cat /etc/hostname
```

Destructive operations are denied: `podman rm -f`, `podman rmi -f`,
`podman kill`, `podman stop -t 0`, `podman system prune`,
`podman volume rm|prune`, `podman network rm|prune`, `podman image rm|prune`.
Ask the user to run these manually if truly needed.

Note: arguments to `podman exec` are not re-decomposed by the hook, so a
destructive shell inside a container (`podman exec web rm -rf /tmp/x`) is not
blocked. Destructive behavior inside a container is a container-level concern.

### File deletion (safe patterns)

The `rm` command is denied by default, but these specific patterns are allowed:

| Pattern | Example |
| --- | --- |
| Underscore-prefixed files | `rm _temp.py`, `rm -f /path/to/_scratch.sh` |
| `/tmp/` paths | `rm /tmp/test_output.json` |
| Cache directories | `rm -rf __pycache__`, `rm -r ~/Library/Caches/foo` |
| `git rm` with relative paths | `git rm old_file.py` |

### Package managers

**pip read-only:**
`pip show`, `pip list`, `pip freeze`, `pip check`

**npm read-only and run:**
`npm list`, `npm root`, `npm ls`, `npm show`, `npm view`, `npm info`, `npm search`,
`npm outdated`, `npm doctor`, `npm prefix`, `npm version`, `npm --version`

`npm run` is allowed for executing scripts defined in local `package.json`.

**brew read-only:**
`brew list`, `brew info`, `brew search`, `brew --prefix`

```bash
pip show numpy
npm list --depth=0
npm run build
brew info python
```

### File access zones

| Tool | Allowed paths |
| --- | --- |
| Read | `~/nsh/`, `~/.<dotdirs>`, site-packages, `/tmp/`, `/var/folders/` |
| Write | `~/nsh/`, `~/.claude/`, `/tmp/` |
| Edit | `~/nsh/`, `~/.claude/`, `/tmp/` |
| Glob | `~/nsh/`, `~/.claude/`, `/tmp/` |
| Grep | `~/nsh/`, `~/.claude/`, `/tmp/` |

All file tools block path traversal (`..`). Reading `.env` and `.secret` files is denied.

### Web tools

`WebFetch` and `WebSearch` are allowed without restrictions.

### Agent types

Any agent with a valid name matching the pattern `^[a-zA-Z][a-zA-Z0-9_:-]*$` is
allowed. This includes built-in types (Explore, Plan, general-purpose) and custom
agents in `~/.claude/agents/`. Missing `subagent_type` falls through to user prompt.

### Orchestration tools

These tools are auto-allowed: `TaskOutput`, `TaskCreate`, `TaskList`, `TaskGet`,
`TaskUpdate`, `TaskStop`, `Skill`, `SendMessage`, `TeamCreate`, `TeamDelete`,
`NotebookEdit`.

Playwright MCP browser tools (`mcp__plugin_playwright_playwright__browser_*`)
are also allowed.

## Denied commands

### `rm` (file deletion)

**Blocked:** `rm file.txt`, `rm -rf dir/`

**Why:** Prevents accidental deletion of important files.

**Instead:** Use underscore-prefixed filenames for scratch files (`_temp.py`),
write to `/tmp/`, or use `git rm` for tracked files.

### `git commit`, `git stash`, `git clean`

**Blocked:** All variations including flag insertion (`git -C /tmp commit`).

**Why:** Humans make commits. `git clean` is destructive and removes untracked files.

**Instead:** Stage changes with `git add` and update `docs/CHANGELOG.md`. The user
commits manually.

### `cat`/`head`/`tail` with file paths

**Blocked:** `cat /path/to/file`, `head -20 /abs/path/file.txt`

**Why:** The Read tool provides a better experience with line numbers and offset/limit.

**Instead:** Use the Read tool with optional `offset` and `limit` parameters.
Pipeline usage without file paths (e.g., consuming stdin) is still allowed.

### `grep`/`rg` with file paths

**Blocked:** `grep pattern /path/to/file`, `rg pattern /abs/search/dir`

**Why:** The Grep tool provides structured output modes and context lines.

**Instead:** Use the Grep tool with `pattern`, `path`, `glob`, `-A`/`-B`/`-C`,
`output_mode`, and `head_limit` parameters. Pipeline filtering (no file path)
is still allowed.

### `find`

**Blocked:** `find . -name "*.py"`

**Why:** The Glob tool is faster and supports recursive patterns. Also has a deny rule.

**Instead:** Use `Glob(pattern='**/*.py', path='/search/dir')`.

### `sed -n`

**Blocked:** `sed -n '10,20p' file.txt`

**Why:** The Read tool with offset and limit does this better.

**Instead:** Use `Read(file_path='file.txt', offset=10, limit=11)`.
Other sed operations (substitution, etc.) are allowed.

### `tsc` via `node_modules` paths

**Blocked:** `./node_modules/.bin/tsc`, `./node_modules/typescript/bin/tsc`,
`/abs/path/node_modules/typescript/bin/tsc`,
`node node_modules/typescript/bin/tsc`

**Why:** Project-local `tsc` paths are a workaround for `npx tsc` failing.
Retrying different invocation forms wastes turns and masks missing installs.

**Instead:** Use `npx tsc` (whitelisted). If `npx tsc` fails because TypeScript
is not installed, run exactly one of these two commands (both are whitelisted):

```bash
npm install --save-dev typescript   # allowed
npm install -g typescript           # allowed
```

Any other `npm install` variation (different flags, version pins, extra
packages, bare `npm install`) still passes through for user approval.

Do not work around the failure with absolute paths, `node node_modules/...`,
or `source source_me.sh &&` chains.

### `perl` on `.pg`/`.pgml` files

**Blocked:** `perl -c problem.pgml`, `perl problem.pg`

**Why:** PGML is not standard Perl. Running perl on these files produces misleading
results.

**Instead:** Use the `/webwork-writer` skill lint guide to validate WeBWorK problems.

### Heredocs (`<<EOF`)

**Blocked:** `python3 - <<EOF`, `bash <<'SCRIPT'`

**Why:** Heredocs are hard to read, lint, and test.

**Instead:** Write code to a `_temp.py` or `_temp.sh` file using the Write tool,
then run it with `source source_me.sh && python3 _temp.py` or `bash _temp.sh`.
Underscore-prefixed files can be removed freely.

### `for` and `while` loops

**Blocked:** `for f in *.py; do ...`, `while read line; do ...`

**Why:** Loop logic belongs in script files, not inline Bash.

**Instead:** Write the logic in a `_temp.py` or `_temp.sh` file and execute it.

### `bash -c` / `bash -lc`

**Blocked:** `bash -c "command"`, `bash -lc "source && python3 ..."`

**Why:** The Bash tool already runs bash. `bash -c` is redundant bash-in-bash.

**Instead:** Run the command directly: `source source_me.sh && python3 script.py`.
Running script files (`bash script.sh`, `bash -n script.sh`) is still allowed.

### `sudo`

**Blocked:** `sudo command`

**Why:** Do not escalate to root. Ask the user to run privileged commands manually.

**Instead:** Ask the user to run the command as root if truly necessary.

### `git reset --hard`

**Blocked:** `git reset --hard`, `git reset -hard HEAD~1`

**Why:** Destructive history rewrite. Use safer alternatives.

**Instead:** `git checkout -- file` or `git restore file` to discard working changes.

### `git push --force` (including --force-with-lease)

**Blocked:** `git push --force`, `git push origin main --force-with-lease`

**Why:** Destructive remote history change.

**Instead:** Ask the user to push manually if rebase is necessary.

### `deno run` with URLs

**Blocked:** `deno run https://example.com/script.ts`

**Why:** Remote code execution. Download and review first.

**Instead:** Download with `curl` to a file, review it, then run locally.

### `curl`/`wget` piped to runtime

**Blocked:** `curl https://example.com/install.sh | bash`, `wget -O - url | python3`

**Why:** Executes remote code without local review.

**Instead:** Download to a file first with `curl -o script.sh https://...`, review,
then run.

### Write/Edit to system directories

**Blocked:** Writing to `/etc/`, `/usr/`, `/opt/`, `/System/`, `/Library/`

**Why:** System files should only be modified by root or package managers.

**Instead:** Write to `~/nsh/` or `/tmp/` instead.

### `mv`

**Blocked:** `mv old.py new.py`

**Why:** Use `git mv` for tracked files to preserve history.

**Instead:** `git mv old.py new.py`. For untracked files, use `cp` + `rm` or ask
the user.

### `VAR=$(...)` assignments

**Blocked:** `PROJECT=$(basename $PWD)`, `OUTPUT=$(python3 script.py)`

**Why:** Command substitution in assignments creates hidden side effects.

**Instead:** Use `source source_me.sh` for environment setup or inline the command
directly.

### `$PYTHON` variable

**Blocked:** `$PYTHON script.py`, `${PYTHON} -m pytest`

**Why:** Use the actual interpreter name for clarity.

**Instead:** `python3 script.py`

### `PYTHONDONTWRITEBYTECODE` / `PYTHONUNBUFFERED`

**Blocked:** Setting these environment variables manually.

**Why:** `source_me.sh` already exports these.

**Instead:** `source source_me.sh && python3 ...`

### Bare variable assignments

**Blocked:** `REPO_ROOT=/path/to/repo` (with no command following)

**Why:** The decomposer splits `A=x && cmd` into leaves; a bare `A=x` leaf is
useless.

**Instead:** Use space-separated env prefixes on one line: `REPO_ROOT=/path python3 script.py`

### `gh` CLI

**Blocked:** All `gh` commands.

**Why:** `gh` is not installed on this system.

**Instead:** N/A. GitHub operations are not available via CLI.

### Homebrew python `-c`

**Blocked:** `/opt/homebrew/bin/python3 -c "print('hello')"`

**Why:** Inline code is hard to lint and debug.

**Instead:** Write a `_temp.py` file and run it with
`source source_me.sh && python3 _temp.py`.

## Passthrough (requires user approval)

These commands intentionally require user approval (passthrough) because they have
significant side effects or security implications:

- **npx (non-whitelisted)**: May fetch remote packages from npm registry
- **npm install**: Modifies machine state, adds/updates dependencies
- **pip install**: Modifies machine state, adds/updates Python packages
- **git rebase**: Rewrites repository history
- **deno eval**: Executes arbitrary inline code

## Passthrough (interactive tools)

These tools intentionally passthrough to Claude Code's default permission flow
so the user sees interactive dialogs:

| Tool | Reason |
| --- | --- |
| `AskUserQuestion` | User must see and answer the question dialog |
| `EnterWorktree` | User must consent to worktree creation |
| `ExitWorktree` | User must consent to keep/remove decision |
| `CronCreate` | User should approve scheduled recurring jobs |
| `CronDelete` | User should approve canceling scheduled jobs |
| `CronList` | Kept consistent with other Cron tools |

Do NOT add these tools to any allow rule. Auto-approving them bypasses Claude Code's
interactive UI dialogs, causing blank answers or skipped consent screens.

## Best practices

- Always use `source source_me.sh && python3` for Python execution
- Use dedicated tools (Read, Grep, Glob) instead of their Bash equivalents
- Write scratch code to `_temp.py` or `_temp.sh` (underscore prefix = safe to delete)
- Keep compound commands under 5 chained sub-commands
- No command substitution (`` ` `` or `$(...)`) in variable assignments
- Use relative paths for project files where possible
- For loops or conditionals, write a script file instead of inline Bash
- Stage changes and update `docs/CHANGELOG.md`; let the user commit

## Common patterns

| Task | Wrong | Right |
| --- | --- | --- |
| Run Python | `python3 script.py` | `source source_me.sh && python3 script.py` |
| Read a file | `cat /path/to/file.py` | Read tool: `file_path="/path/to/file.py"` |
| Search files | `grep -r "pattern" src/` | Grep tool: `pattern="pattern"`, `path="src/"` |
| Find files | `find . -name "*.py"` | Glob tool: `pattern="**/*.py"` |
| Read lines 10-20 | `sed -n '10,20p' file.txt` | Read tool: `offset=10`, `limit=11` |
| Delete temp file | `rm temp.py` | Name it `_temp.py`, then `rm _temp.py` |
| Rename file | `mv old.py new.py` | `git mv old.py new.py` |
| Loop over files | `for f in *.py; do ...` | Write `_temp.sh` with the loop, run `bash _temp.sh` |
| Inline Python | `python3 -c "print(1)"` | Write `_temp.py`, run with source_me.sh |
| Set env + run | `REPO_ROOT=/x && python3 s.py` | `REPO_ROOT=/x python3 s.py` (one line) |
| Run heredoc | `python3 - <<EOF ...` | Write `_temp.py`, run with source_me.sh |
| GitHub CLI | `gh pr list` | Not available (`gh` not installed) |
