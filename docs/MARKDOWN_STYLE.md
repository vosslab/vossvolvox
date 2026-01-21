# Markdown Style

Keep documentation concise, scannable, and consistent.

## Content
- use ASCII and ISO-8859-1 character encoding
- escape UTF-8 symbols such as &alpha;, &beta;, etc.

## Headings
- Use sentence case.
- Start at `#` for the document title, then `##`, `###` as needed.
- Keep headings short (3-6 words).

## Lists
- Prefer `-` for bullets.
- One idea per bullet.
- Keep bullet lines short; wrap at ~100 chars.

## Code
- Use fenced code blocks with language where practical.
- Use inline backticks for file paths, CLI flags, and identifiers.

## Tables and diagrams
- Use ASCII-only tables and diagrams. Do not use Unicode box-drawing or checkmark symbols.
- For boxed layouts, use `+`, `-`, and `|` inside fenced code blocks.
- Replace checkmarks with `OK`, `YES`, or `[x]` and crosses with `NO`, `FAIL`, or `[ ]`.
- For progress bars or fills, use `#` and `.` (or `-`) instead of block characters.
- If the content is tabular, prefer Markdown tables unless alignment in a code block is required.
- Simple Markdown table example:
  | Field | Description |
  | --- | --- |
  | input | Path to input file |
  | output | Path to output file |

## Links
- Use relative links inside the repo.
- Prefer descriptive link text, not raw URLs.
- When referencing another doc, always link it (avoid bare filenames).
- Example: [docs/FORMAT.md](docs/FORMAT.md), [docs/CLI.md](docs/CLI.md)

## Examples
- Show a minimal example before a complex one.
- Label sample output explicitly if needed.

## Tone
- Write in the present tense.
- Prefer active voice.
- Avoid filler and speculation.
