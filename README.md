# Source2HTML (C-to-HTML Syntax Highlighter)

![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)
![Language: C](https://img.shields.io/badge/Language-C11-blue.svg)
![Build: GCC](https://img.shields.io/badge/Build-GCC%20%2F%20Make-green.svg)

**Source2HTML** is a lexical parsing tool built in C that converts C source files into styled, syntax-highlighted HTML documents viewable in any web browser. It uses an event-driven Finite State Machine (FSM) to tokenize source elements—such as preprocessor directives, keywords, strings, comments, and numeric constants—and wraps them in styled HTML spans.

---

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Architecture & State Machine](#architecture--state-machine)
- [Event & Color Mapping](#event--color-mapping)
- [Project Structure](#project-structure)
- [Build Instructions](#build-instructions)
- [Usage](#usage)
- [Example Output](#example-output)
- [Testing](#testing)
- [License](#license)

---

## Overview

Displaying raw code inside a web browser often requires clean typography and distinct colors for syntactic tokens. **Source2HTML** reads an input C file character by character, parses tokens through state transitions, emits corresponding HTML tags with specific CSS classes, and links an external stylesheet (`styles.css`).

---

## Features

- **Event-Driven Lexer**: Decouples lexical analysis (`s2html_event.c`) from HTML formatting (`s2html_conv.c`).
- **Granular Token Highlighting**:
  - **Preprocessor Directives**: `#include`, `#define`, macros.
  - **Header Files**: Distinguishes between standard library (`<...>`) and user (`"..."`) headers.
  - **Keywords**: Categorized into Data Types (`int`, `float`, `char`, etc.) and Control Flow / Non-Data (`if`, `for`, `return`, `switch`, etc.).
  - **Comments**: Full support for both single-line (`//`) and multi-line (`/* ... */`) comments.
  - **Literals & Constants**: Handles numerical constants (decimal, hex, float), string literals, and ASCII character literals.
  - **Escape Sequences**: Correctly handles escaped characters inside strings (`\"`) and char literals (`'\n'`).
- **CSS Styling**: Styled using clean classes configured inside `styles.css`.

---

## Architecture & State Machine

The parser processes the source stream using an internal State-Event transition model:

```
                  +--------------------------------+
                  |          PSTATE_IDLE           |
                  +--------------------------------+
                     /    |      |      |     \
    '/' (with '*' or '/') |      |      |      \  '#'
                   v      |      |      |       v
       [Comment States]   |      |      |   [Preprocessor]
                          |      |      |          |
                          |      |      |          v
       'a'-'z'            v      |      v    [Header Files]
       [Reserved Words] <--------+------> [Numbers / Strings / Chars]
```

1. **`PSTATE_IDLE`**: Default resting state. Inspects incoming characters to detect tokens (`/`, `#`, `"`, `'`, `0-9`, `a-z`).
2. **Specialized States**:
   - `PSTATE_SINGLE_LINE_COMMENT`: Gathers text until `\n`.
   - `PSTATE_MULTI_LINE_COMMENT`: Buffers characters until reaching `*/`.
   - `PSTATE_PREPROCESSOR_DIRECTIVE`: Parses directives and switches to sub-states for headers and constants.
   - `PSTATE_HEADER_FILE`: Detects and buffers `<filename.h>` or `"filename.h"`.
   - `PSTATE_RESERVE_KEYWORD`: Collects the full identifier, then compares against keyword tables using `strcmp`.
   - `PSTATE_STRING`: Captures double-quoted literal text, handles escaped quotes `\"`.
   - `PSTATE_NUMERIC_CONSTANT`: Captures decimal, hex (`0x...`), and floating-point literals.
   - `PSTATE_ASCII_CHAR`: Captures character literals enclosed in single quotes, handles `'\n'` etc.
3. **Event Generation**: When an entity terminates, the parser returns a `pevent_t` structure indicating the token type and data buffer, resetting back to `PSTATE_IDLE`.

---

## Event & Color Mapping

The converter generates `<span class="...">` tags matching classes defined in `styles.css`:

| Category | CSS Class | Color | Description / Example |
| :--- | :--- | :--- | :--- |
| **Comments** | `.comment` | Blue | `/* comment */` or `// comment` |
| **Data Types** | `.reserved_key1` | Green | `int`, `char`, `double`, `void`, `struct` |
| **Control Keywords** | `.reserved_key2` | Goldenrod | `if`, `else`, `while`, `for`, `return` |
| **Preprocessor Directives**| `.preprocess_dir`| Purple | `#include`, `#define`, `#ifdef` |
| **Header Files** | `.header_file` | Red | `<stdio.h>`, `"my_header.h"` |
| **String Literals** | `.string` | Magenta | `"Hello, World!"` |
| **Numeric Constants** | `.numeric_constant`| Brown | `10`, `3.14`, `0xFF` |
| **ASCII Characters** | `.ascii_char` | Firebrick | `'a'`, `'\n'` |

---

## Project Structure

```text
.
├── LICENSE             # MIT License
├── makefile            # Build script configured for GCC (C11 standard)
├── s2html_main.c       # Main entry point & file stream coordinator
├── s2html_event.h      # State enumerations, event definitions, and prototypes
├── s2html_event.c      # State machine implementation and lexical analyzer
├── s2html_conv.h       # Prototypes and constants for HTML conversion
├── s2html_conv.c       # Functions to emit HTML elements and CSS spans
├── styles.css          # CSS styles applied to generated HTML pages
├── test.c              # Sample C file covering all token types for testing
└── README.md           # Project documentation
```

---

## Build Instructions

The project includes a `makefile` configured with GCC and C11.

> **Prerequisites**: GCC must be installed.  
> **Windows**: Install [MSYS2](https://www.msys2.org/) and run `pacman -S mingw-w64-x86_64-gcc make`, then add `C:\msys64\mingw64\bin` to your PATH.  
> **Linux/macOS**: GCC is usually pre-installed; if not run `sudo apt install build-essential` (Ubuntu/Debian).

### Build Binary

```bash
make
```

Compiles all `.c` source files and creates the target executable (`s2html_conv.exe`).

### Clean Build Artifacts

```bash
make clean
```

Removes compiled object files (`*.o`) and the executable.

---

## Usage

Run the compiled executable from your command line, supplying the target source file:

```bash
# General syntax
./s2html_conv.exe <source_file.c> [destination_prefix]

# Example 1: Output defaults to <source_file>.html (e.g., sample.c.html)
./s2html_conv.exe sample.c

# Example 2: Specifying a custom destination file prefix
./s2html_conv.exe sample.c output
# Generates output.html
```

### Viewing in Browser

Ensure `styles.css` is located in the same directory as the resulting `.html` file, then open it:

```bash
# Linux
xdg-open output.html

# Windows (PowerShell)
start output.html

# Windows (from WSL)
explorer.exe output.html
```

---

## Testing

A `test.c` file is included that covers **all supported token types**:

```bash
# Build and test
make clean && make
./s2html_conv.exe test.c
explorer.exe test.c.html   # open in browser
```

**What to verify in the browser:**

| Token in test.c | Expected Color |
|-----------------|---------------|
| `#include`, `#define` | Purple |
| `<stdio.h>`, `"my_header.h"` | Red |
| `/* ... */`, `// ...` | Blue |
| `int`, `float`, `char`, `void`, `struct` | Green |
| `if`, `else`, `for`, `while`, `return`, `switch` | Goldenrod |
| `"Hello, World!\n"` | Magenta |
| `3.14`, `100`, `0xFF` | Brown |
| `'A'`, `'\n'` | Firebrick |

---

## Example Output

Given a sample C source file:
```c
#include <stdio.h>

/* Print greeting */
int main(void) {
    printf("Hello World\n");
    return 0;
}
```

**Source2HTML** generates:
```html
<!DOCTYPE html>
<html lang="en-US">
<head>
<title>source2html</title>
<meta charset="UTF-8">
<link rel="stylesheet" href="styles.css">
</head>
<body style="background-color:lightgrey;">
<pre>
<span class="preprocess_dir">#include </span><span class="header_file">&lt;stdio.h&gt;</span>

<span class="comment">/* Print greeting */</span>
<span class="reserved_key1">int</span> main(<span class="reserved_key1">void</span>) {
    printf(<span class="string">"Hello World\n"</span>);
    <span class="reserved_key2">return</span> <span class="numeric_constant">0</span>;
}
</pre>
</body>
</html>
```

---

## License

This project is licensed under the **MIT License** — see the [LICENSE](LICENSE) file for details.

```
MIT License  Copyright (c) 2026 alfaj
```
