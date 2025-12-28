# PYTHON_STYLE.md

Language Model guide to Neil python3 programming

## Python version

* I like using one of the latest versions of python, but not the latest, of python3, currently **3.12**.

## USE TABS

* Always use tabs for indentation in python3 code, never spaces!
* I disagree with PEP 8 requirement of spaces. Using spaces causes a lot more deleting and formatting work.
* I use tabs exclusively.
* Tabs provide flexibility, because I can change the tab size to 2, 3, 4, or 8 spaces depending on the complexity of the code.

## CODE STRUCTURE

- Use a `def main()` for the backbone of the code, have a `if __name__ == '__main__': main()` to run main
- I prefer single task sub-functions over one large function.
- Whenever making a URL or API internet request, add a time.sleep(random.random()) to avoid overloading the server, unless the official API specifies otherwise.
- For error handling, avoid try/except block if you can, I find they cause more problems than they solve.
- Use try/except rarely, and if needed use for at most two lines
- Avoid using `sys.exit(1)` prefer to raise Errors.
- Use f-strings, in older code I used `.format()` or `'%'` system, update to f-strings.
- I prefer string concatenation `'+='` over multiline strings.
- Start off python3 programs with the line `#!/usr/bin/env python3` to make them executable
- Return statements should be simple and should not perform calculations, fill out a dict, or build strings. Store computed values and assembled strings in variables first, including any multiline HTML or text, then return the variable.
- add comments within the code to describe what different lines are doing, to make for better readability later. especially for complex lines!
- Please only use ascii characters in the script, if utf characters are need they should be escape e.g. `&alpha;` `&lrarr;`

## QUOTING
* Avoid backslash escaping quotes inside strings when possible.
* Prefer alternating quote styles instead:
* Use double quotes on the outside with single quotes inside.
* Or use single quotes on the outside with double quotes inside.
* This is especially useful for HTML like "<span style='...'>text</span>".

## HTML UNITS IN MONOSPACE
* When generating HTML for lab problems, render numeric values and their units in monospace for readability and alignment.
* Use a span like:

volume_text = f"<span style='font-family: monospace;'>{vol1:.1f} mL</span>"

## TESTING

- I like to test the code with **pyflakes** and **mypy**
- For simple functions only, provide an **assert** command.
- create a folder in most projects called tests for storing test scripts
- a good test script is run_pyflakes.sh
```bash
#!/usr/bin/env bash
set -euo pipefail
cd "${PYTHON_ROOT}"
pyflakes $(find "${PYTHON_ROOT}" -type f -name "*.py" -print) > pyflakes.txt 2>&1 || true
RESULT=$(wc -l < pyflakes.txt)
if [ "${RESULT}" -eq 0 ]; then
	echo "No errors found!!!"
	exit 0
fi
echo "Found ${RESULT} pyflakes errors"
exit 1
```

## DO NOT USE HEREDOCS

* Do not use shell heredocs to run inline Python code.
* Avoid patterns like `python3 - <<EOF`.
* Python code should live in `.py` files or be passed explicitly as files or modules.
* Heredocs make code harder to read, harder to lint, and harder to test.

Here is a tightened version that keeps the rule and examples, without extra explanation.

## ENVIRONMENT VARIABLES

The program should not require custom environment variables to function. Configuration must be explicit and visible via config files or command line arguments. Environment variables may be read only when they are standard OS or ecosystem variables, not variables invented to control program behavior.

### Allowed

```python
home_dir = os.environ["HOME"]
editor = os.environ.get("EDITOR", "nano")
tmp_dir = os.environ.get("TMPDIR", "/tmp")
is_ci = os.environ.get("CI") == "true"
```

### Disallowed

```python
api_key = os.environ["API_KEY"]
preserve_temp = os.environ.get("PRESERVE_TEMP_FILES") == "1"
debug_mode = os.environ.get("DEBUG_MODE") == "true"
```

For api keys, mode settings, or temp variables use argparse, config files, or function arguments instead. Rule of thumb: If a variable exists only because the script exists, it does not belong in the environment.

## COMMENTING

- Assume all import statements have been made at the top of the script file
- Visually separate function with comment of only equal signs. It makes it easier to visually see the functions in the code. For example:
```python
#============================================
```

* Use Google style documentation style for functions. https://github.com/google/styleguide
* I prefer to keep line lengths less than 100 characters
* Comments should be on a line of their own before the code they are commenting
* No emoji or special characters in comments, only ascii characters

## ASSERT
* For simple functions only, provide an assert command.
* Do not do complex asserts that would require more than 4 lines or go over 100 characters in length
* Do not add assert to functions that require user input or read/write to files

### Good examples of Assert

* Simple assertion test for the function: 'check_due_date'
```python
result = check_due_date("1970/10/25", {'deadline': {'due date': 'Oct 25, 1970'}})
assert result == (0.0, 'On-Time', '')
```

* Simple assertion test for the function: 'get_final_score'
```python
test_entry = {}
test_config = {'total points': '10', 'assignment name': 'HW1'}
get_final_score(test_entry, test_config)
assert test_entry['Final Score'] == '10.00'
```

* Simple assertion test for the function: 'make_key'
```python
result = make_key({'ID': 12, 'Name': 'JoHN  '}, ('ID', 'Name'))
assert result == '12 john'
```

## TYPE HINTING
* Use the python3-style explicit variable type hinting. I think it is good practice. Very little of my code uses it now, but I want to change that. For example,
```python
def greater_than(a: int, b: int) -> bool:
	return a > b
```
* Avoid using the typing module, only do top level for typing:
* GOOD: def func(arg: dict)-> tuple:
* BAD: def func(arg: typing.Dict[str, typing.Any]) -> typing.Tuple[str, str, str]

## IMPORTING
* Never use import *
* I prefer to keep the original module names, just import numpy is preferred over import numpy as np.
* This one is flexible but I also prefer to avoid using 'from' in the import statements. 'import PIL.Image' over 'from PIL import Image'. I like to know where commands are coming from.
* Place import modules in the following order (1) external python/pip modules vs. local repo modules with commented headings (2) length of module name with the shortest first, (3) alphabetical 'os' comes before 're'. For example:

```python
# Standard Library
import os
import re
import sys
import time
import random
import argparse

# PIP3 modules
import numpy
import PIL.Image #python-pillow

# local repo modules
import bptools
import aminoacidlib
```

## ARGPARSE
* Always provide argparse for inputs and outputs and any variables to adjusted.
* Argparse value should have both a single letter cli flag and a full word cli flag. You should always specify the destination for the value, dest='flag_name'. See example below.
* When doing an argparse boolean value. Provide both on and off flags and set the default with a different command, For example,
```python
	parser.add_argument('-e', '--send-email', dest='dry_run', help='send emails', action='store_false')
	parser.add_argument('-n', '--dry-run', dest='dry_run', help='only display email', action='store_true')
	parser.set_defaults(dry_run=True)
```
* I like using argparse groups
```python
	question_group = parser.add_mutually_exclusive_group(required=True)
	question_group.add_argument(
		'-f', '--format', dest='question_format', type=str,
		choices=('num', 'mc'),
	)
	question_group.add_argument(
		'-m', '--mc', dest='question_format', action='store_const', const='mc',
	)
	question_group.add_argument(
		'-n', '--num', dest='question_format', action='store_const', const='num',
		help='Set question format to numeric'
	)
	parser.set_defaults(question_format='mc')
```
* Use a separate function for argparse that returns args to the main() function at start
```python
def parse_args():
	"""
	Parse command-line arguments.
	"""
	parser = argparse.ArgumentParser(description="<replace with script function>")
	parser.add_argument('-i', '--input', dest='input_file', required=True, help="Input file")
	args = parser.parse_args()
	return args
```

## DATA FILES
* YAML favorite, readable, editable
* CSV spreadsheet input/output
* JSON good for large, wordy data
* PNG images, graphics, figures, pixel data

## AVAILABLE MODULES

I keep several pip3 modules installed on all my systems that are available for use

```bash
% cat pip_requirements.txt
accelerate  # Hugging Face helper for running PyTorch training
biopython  # Bioinformatics tools for sequences, structures
brickse  # Brickset related utilities or API client for LEGO set date
crcmod  # Fast CRC checksum functions (CRC-32, CRC-16)
curl_cffi  # HTTP client using libcurl via cffi, for browser-like TLS
einops  # Clean tensor reshaping and rearranging (works with numpy, torch, etc.)
face_recognition  # Simple face detection and recognition built on dlib
google-api-python-client  # Official Google API client for many Google services
gtts  # Google Text-to-Speech client for generating speech audio
imagehash  # Perceptual image hashing for similarity and duplicate detection
lxml  # Fast XML and HTML parsing with XPath support
matplotlib  # Plotting and figure generation
mkdocs-include-markdown-plugin  # MkDocs plugin to include markdown files
mkdocs-material  # Material theme for MkDocs documentation sites
mutagen  # Read and write audio metadata tags (MP3, FLAC, etc.)
num2word  # Convert numbers into words (for example 42 to forty two)
numpy  # Core n-dimensional arrays and vectorized numerical computing
ollama  # Client for interacting with a local Ollama model server
opencv-python  # OpenCV bindings for computer vision and image processing
openpyxl  # Read and write Excel .xlsx files
pandas  # DataFrames for tabular data analysis and ETL
pillow  # Image I O and basic image processing (PIL fork)
pillow_heif  # HEIF and HEIC support for Pillow
py-applescript  # Run AppleScript from Python (macOS automation)
pyexiftool  # Python wrapper for exiftool metadata extraction and editing
pyflakes  # Static analysis to find unused imports and simple errors
pygame  # SDL based multimedia and simple game framework
pytesseract  # Python wrapper for Tesseract OCR
python-bricklink-api  # BrickLink API client for LEGO parts and orders
pyyaml  # YAML parsing and serialization
qrcode  # QR code generation
rebrick  # Rebrickable API client for LEGO set and parts data
rembg[cli]  # Remove image backgrounds using segmentation models
requests  # Simple HTTP library
rich  # Pretty console output, logging, progress bars, and tracebacks
scipy  # Scientific computing
tabulate  # Pretty print tables in plain text
torch  # PyTorch tensor library and deep learning framework
torchvision  # Vision datasets, transforms, and pretrained models for PyTorch
transformers  # Hugging Face transformer models, tokenizers, and pipelines
transliterate  # Convert text between scripts (for example Cyrillic to Latin)
unidecode  # Best effort conversion of Unicode text to ASCII
wikipedia  # Simple Python interface to Wikipedia search and pages
xmltodict  # Convert XML to nested Python dicts and back
yt-dlp[default]  # Video and audio downloader
```
