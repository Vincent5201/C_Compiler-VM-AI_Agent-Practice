# `mini_cc`: An Educational C Compiler

Welcome to **`mini_cc`**, a lightweight, self-contained C compiler implemented entirely in C. It compiles a structured subset of the C programming language into **Intel-syntax x86-64 assembly** targeting the **Linux System V AMD64 ABI**.

This project is carefully structured to serve as an **educational playground** for engineers and students learning compiler engineering. Rather than relying on third-party parser generators (like Lex/Yacc or Flex/Bison), every phase—from reading raw characters to emitting assembly—is built by hand using pure C.

---

## Table of Contents
1. [High-Level Architecture](#1-high-level-architecture)
2. [Supported Language Features](#2-supported-language-features)
3. [Deep Dive: The Compilation Stages](#3-deep-dive-the-compilation-stages)
4. [File Directory Tour](#4-file-directory-tour)
5. [End-to-End Walkthrough: `test1.c`](#5-end-to-end-walkthrough-test1c)
6. [Build, Test, and Execution Guide](#6-build-test-and-execution-guide)

---

## 1. High-Level Architecture

The compilation pipeline operates as a sequence of deterministic transformations:

```text
 C Source Text (.c)
        │
   ┌─────────┐
   │ Scanner │ (Lexical Analysis)
   └─────────┘
        │
   ┌─────────┐
   │ Parser  │ (Syntax Analysis via Recursive Descent)
   └─────────┘
        │
   ┌─────────┐
   │ CodeGen │ (Assembly Synthesis)
   └─────────┘
        │
  Assembly Output (.txt)
```

---

## 2. Supported Language Features

`mini_cc` supports a subset of C.

### Supported Keywords
| Keyword | Description |
| :--- | :--- |
| `int` | Type declaration for local variables and parameters (64-bit). |
| `return` | Exit function and return expression value. |
| `if`, `else` | Conditional branching structures. |
| `while` | Iterative loop structure. |

### Supported Operators & Delimiters
| Category | Syntax |
| :--- | :--- |
| **Arithmetic** | `+`, `-`, `*`, `/` |
| **Comparison** | `==`, `!=`, `<`, `>`, `<=`, `>=` |
| **Assignment** | `=` |
| **Delimiters** | `(`, `)`, `{`, `}`, `,`, `;` |

---

## 3. Deep Dive: The Compilation Stages

### Phase A: Lexical Analysis (Scanner)
*Files: `scanner.c`, `scanner.h`*
Converts raw source code into a `TokenList` array. It handles:
- **Whitespace skipping**.
- **Pattern recognition** for identifiers, keywords, numbers, and operators.
- **Error reporting** for unsupported characters.

### Phase B: Syntax Analysis (Parser)
*Files: `parser.c`, `parser.h`*
Builds an Abstract Syntax Tree (AST) using recursive descent parsing. It enforces **Operator Precedence**:
`Primary` → `Multiplication` → `Addition` → `Comparison` → `Expression`.

### Phase C: Code Generation (CodeGen)
*Files: `codegen.c`, `codegen.h`*
Traverses the AST to emit x86-64 assembly.

#### Register Usage (System V AMD64 ABI):
| Register | Purpose |
| :--- | :--- |
| `RAX` | Expression result / Return value |
| `RDI` | 1st function argument |
| `RSI` | 2nd function argument |
| `RDX` | 3rd function argument |
| `RCX` | 4th function argument |
| `R8` | 5th function argument |
| `R9` | 6th function argument |
| `RBP` | Frame pointer |
| `RSP` | Stack pointer |

### Supported Assembly Instructions
The compiler generates x86-64 assembly in Intel syntax. Below are the core instructions utilized:

| Instruction | Description |
| :--- | :--- |
| `mov` | Move data (register to register, immediate to register, memory to register/memory). |
| `push` | Push value onto the stack. |
| `pop` | Pop value from the stack to a register. |
| `add` | Integer addition. |
| `sub` | Integer subtraction. |
| `imul` | Signed integer multiplication. |
| `idiv` | Signed integer division (requires `cqo` for sign extension). |
| `cmp` | Compare two operands (sets EFLAGS). |
| `set[cond]` | Sets byte to 1 if condition met (e.g., `setl` for less, `setg` for greater). |
| `movzx` | Move with zero-extension (used for comparison results). |
| `je` | Jump if equal (used for control flow branching). |
| `jmp` | Unconditional jump. |
| `call` | Call a function. |
| `ret` | Return from function. |

### Grammatical Hierarchy & Parsing Logic
The compiler uses a **Recursive Descent Parser**, where the grammatical structure is decomposed into a hierarchical set of functions. Each function handles a specific non-terminal symbol in the grammar.

#### The Grammatical Hierarchy
```text
Program (Root)
 └── Function[]
      ├── Parameters
      └── Statement[] (Body)
           ├── Expression (Condition, Assignment value, Return value)
           └── Nested Statements (If-body, While-body)
```

#### Parsing Breakdown
| Level | Parsing Logic |
| :--- | :--- |
| **Program** | Iteratively calls `parse_function()` until `TOKEN_EOF` is encountered. |
| **Function** | Expects `int` keyword, function name, `(`, parameter list, and a block of statements (`{...}`). |
| **Statement** | Based on the current token, dispatches to specific parsers: `int` → `STMT_VAR_DECL`, `return` → `STMT_RETURN`, `if` → `STMT_IF`, `while` → `STMT_WHILE`, or identifier → `STMT_ASSIGN`. |
| **Expression** | Implements operator precedence using chained function calls: `Expression` → `Comparison` → `Addition` → `Multiplication` → `Primary`. |

---

### Variable Management
*File: `codegen.c`*

The compiler maintains a simple symbol table for local variables and parameters within each function scope.

- **Storage Structure**: Variables are tracked in a static array `variables[MAX_VARIABLES]`, where each entry holds the variable's name and its assigned stack offset.
- **Allocation (`add_variable`)**: 
    - When the parser encounters a variable declaration (`STMT_VAR_DECL`), `add_variable` is called.
    - Each `int` is assigned 8 bytes on the stack.
    - `stack_size` is incremented by 8, and the current `stack_size` value is stored as the variable's negative offset from the `RBP` frame pointer.
- **Lookup (`find_variable`)**: During code generation, when a variable is used, `find_variable` searches the `variables` array by name to retrieve its pre-calculated `RBP`-relative offset.
- **Assembly Generation (`print_memory`)**: Variables are accessed in assembly using the `QWORD PTR [rbp-offset]` syntax, ensuring consistent access regardless of the current stack state during expression evaluation.

---

## 4. File Directory Tour

| File | Description |
| :--- | :--- |
| `main.c` | Entry point. Validates args and calls `compile_file()`. |
| `compiler.c/h` | Driver. Reads source, coordinates scanner/parser/codegen. |
| `scanner.c/h` | Tokenizer logic. |
| `parser.c/h` | AST builder using recursive descent. |
| `codegen.c/h` | Emits x86-64 assembly instructions. |

---

## 5. End-to-End Walkthrough: `test1.c`

```c
int main() { int x; x = 10 + 20; return x; }
```

### Emitted Assembly:
```assembly
.globl main
main:
    push rbp
    mov rbp, rsp
    sub rsp, 16            # Allocate stack frame
    mov rax, 10            # Left operand
    push rax
    mov rax, 20            # Right operand
    mov rcx, rax
    pop rax                # Left operand back to RAX
    add rax, rcx           # Add RAX + RCX
    mov QWORD PTR [rbp-8], rax # Save to 'x'
    mov rax, QWORD PTR [rbp-8] # Return value
    mov rsp, rbp
    pop rbp
    ret
```

---

## 6. Build, Test, and Execution Guide

### Compilation
```bash
gcc -std=c11 -Wall -Wextra main.c compiler.c scanner.c parser.c codegen.c -o mini_cc
```

### Running the Compiler
```bash
./mini_cc <input.c>
```

---

## License

This project is licensed under the MIT License.