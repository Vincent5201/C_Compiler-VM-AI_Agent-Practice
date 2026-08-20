# Mini C Compiler Practice

This repository is dedicated to practicing C compiler development and exploring integrations with the Gemini CLI. It contains **mini_cc**, a simple, educational C compiler implemented in C.

## Overview

The primary goals of this project are:
- To practice building a C compiler from scratch (including lexical analysis, parsing, Abstract Syntax Tree construction, and x86-64 assembly generation).
- To experiment with and utilize **Gemini CLI** features, workflows, and custom subagents in software engineering.

---

## Architecture

```text
C Source (.c)
     ↓
  Scanner (Lexer)
     ↓
   Parser
     ↓
    AST (Abstract Syntax Tree)
     ↓
Code Generator
     ↓
Assembly (output.txt)
```

- **Scanner**: Converts C source code into a stream of tokens.
- **Parser**: Converts tokens into an Abstract Syntax Tree (AST).
- **Code Generator**: Converts the AST into x86-64 assembly.
- **main.c**: Reads the input `.c` file and initiates compilation through `compile_file()`.

---

## Project Structure

```text
C_compilor_practice/
├── main.c           # Compiler entry point
├── compiler.c/h     # Core compilation driver
├── scanner.c/h      # Lexical analyzer (tokenizes source)
├── parser.c/h       # Syntax analyzer & AST definitions
├── codegen.c/h      # x86-64 assembly generator
├── Makefile         # Build automation for compiler and testing
├── GEMINI.md        # Guidelines for Gemini CLI
├── README.md        # Project documentation (this file)
├── setting.md       # Original compiler specifications
└── tests/           # Directory with sample test programs
    ├── test1.c      # Variable declaration, assignment, arithmetic, & return
    ├── test2.c      # Function definitions and function calls with arguments
    ├── test3.c      # If-else conditional statements
    └── test4.c      # While-loop iteration
```

---

## Supported Features

- `int` variables (local declarations)
- Variable assignment
- Arithmetic operations: `+`, `-`, `*`, `/`
- Comparison operators: `==`, `!=`, `<`, `>`, `<=`, `>=`
- Control flow: `if / else`
- Loops: `while`
- Functions with multiple parameters & function calls
- Return statements (`return`)

---

## Register Management

The compiler targets **x86-64** and conforms to the **System V AMD64 ABI**.

| Purpose | Register |
|---|---|
| Expression / return value | `RAX` |
| 1st argument | `RDI` |
| 2nd argument | `RSI` |
| 3rd argument | `RDX` |
| 4th argument | `RCX` |
| 5th argument | `R8` |
| 6th argument | `R9` |
| Stack pointer | `RSP` |
| Stack frame pointer | `RBP` |

Temporary values are stored on the stack when necessary, while local variables are stored at offsets from `RBP`.

---

## Build & Test

### Prerequisites
- GCC compiler supporting C11 (`-std=c11`)
- Make utility (GNU Make)

### Build the Compiler
To build the compiler executable `mini_cc` (or `mini_cc.exe` on Windows):
```bash
make
```

Alternatively, you can compile manually using GCC:
```bash
gcc -std=c11 -Wall -Wextra main.c compiler.c scanner.c parser.c codegen.c -o mini_cc
```

### Run Tests
To automatically compile and run the compiler against all test cases inside the `tests/` folder:
```bash
make test
```
This runs `mini_cc` on `tests/test1.c`, `tests/test2.c`, `tests/test3.c`, and `tests/test4.c`.

### Clean up
To remove compiled executables, intermediate artifacts, and generated output assembly:
```bash
make clean
```

---

## Usage

To run the compiler on a custom C input file:
```bash
./mini_cc <input.c>
```
On successful compilation, the output x86-64 assembly is written to **`output.txt`**.

### Example (`tests/test1.c`)
Input:
```c
int main()
{
    int x;

    x = 10 + 20;

    return x;
}
```

Running `./mini_cc tests/test1.c` generates the x86-64 assembly in `output.txt`.

> **Note**: This is an educational compiler. It intentionally does not implement pointers, arrays, structs, floating-point types, optimization, or advanced register allocation.

---

## License

This project is open-source and available under the MIT License.
