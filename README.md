# `mini_cc`: An Educational C Compiler

Welcome to **`mini_cc`**, a lightweight, self-contained C compiler implemented entirely in C. It compiles a structured subset of the C programming language into **Intel-syntax x86-64 assembly** targeting the **Linux System V AMD64 ABI**.

This project is carefully structured to serve as an **educational playground** for engineers and students learning compiler engineering. Rather than relying on third-party parser generators (like Lex/Yacc or Flex/Bison), every phase—from reading raw characters to emitting assembly—is built by hand using pure C.

---

## Table of Contents
1. [High-Level Architecture](#1-high-level-architecture)
2. [Supported Language Features](#2-supported-language-features)
3. [Deep Dive: The Compilation Stages](#3-deep-dive-the-compilation-stages)
   - [Phase A: Lexical Analysis (Scanner)](#phase-a-lexical-analysis-scanner)
   - [Phase B: Syntax Analysis (Recursive Descent Parser)](#phase-b-syntax-analysis-recursive-descent-parser)
   - [Phase C: Code Generation (Stack-Based Assembly Generator)](#phase-c-code-generation-stack-based-assembly-generator)
4. [File Directory Tour](#4-file-directory-tour)
5. [End-to-End Walkthrough: `test1.c`](#5-end-to-end-walkthrough-test1c)
6. [Build, Test, and Execution Guide](#6-build-test-and-execution-guide)

---

## 1. High-Level Architecture

The compilation pipeline operates as a sequence of deterministic transformations, passing data from one structural format to another:

```text
 C Source Text (.c)
        │  [Input: Raw String]
        ▼
   ┌─────────┐
   │ Scanner │ (Lexical Analysis)
   └─────────┘
        │  [Output: TokenList]
        ▼
   ┌─────────┐
   │ Parser  │ (Syntax Analysis via Recursive Descent)
   └─────────┘
        │  [Output: Abstract Syntax Tree (AST)]
        ▼
   ┌─────────┐
   │ CodeGen │ (Assembly Synthesis)
   └─────────┘
        │  [Output: Intel x86-64 Assembly (.asm / output.txt)]
        ▼
  Linker (GCC/ld) -> Machine Code Executable
```

---

## 2. Supported Language Features

`mini_cc` supports a subset of C. Below is the reference of supported syntax:

### Keywords
- `int`: Type declaration for local variables and function parameters.
- `return`: Exit function and return expression value.
- `if`, `else`: Conditional branching.
- `while`: Iterative loop structure.

### Operators & Delimiters
| Category | Supported Syntax |
| :--- | :--- |
| **Arithmetic** | `+`, `-`, `*`, `/` |
| **Comparison** | `==`, `!=`, `<`, `>`, `<=`, `>=` |
| **Assignment** | `=` |
| **Delimiters** | `(`, `)`, `{`, `}`, `,`, `;` |

*Note: All integers are treated as 64-bit.*

---

## 3. Deep Dive: The Compilation Stages

---

### Phase A: Lexical Analysis (Scanner)
*Files: `scanner.c`, `scanner.h`*

The scanner converts a single continuous string representing the C file contents into an array of token structures (`TokenList`).

#### Mechanics & How It Works:
1.  **State Machine / Iteration**: It scans the source string character-by-character using a pointer index `i`.
2.  **Whitespace Skipping**: Characters like spaces (` `), tabs (`\t`), and line breaks (`\n`, `\r`) are identified via standard `isspace()` checks and skipped.
3.  **Pattern Recognition**:
    *   **Identifiers & Keywords**: When an alphabetical character or underscore (`_`) is found, the scanner reads subsequent alphanumeric characters. The string is matched against keywords (`int`, `return`, `if`, `else`, `while`). If it matches, the corresponding keyword token type is assigned (e.g., `TOKEN_IF`); otherwise, it becomes a general `TOKEN_IDENTIFIER`.
    *   **Numbers**: Digits are grouped together to create a single `TOKEN_NUMBER`.
    *   **Double-Character Operators**: Check-ahead checks identify two-character operators (`==`, `!=`, `<=`, `>=`) before defaulting to single characters.
    *   **Single-Character Operators / Delimiters**: Matched via a `switch` statement mapping characters like `;`, `(`, `{` to their respective token types.
4.  **Error Handling**: If an unexpected character is read (e.g., `#` or unsupported unicode symbols), a scanner error is reported, halting compilation immediately.

**Token Data Structures:**
```c
typedef struct {
    TokenType type;
    char text[64];
} Token;

typedef struct {
    Token *tokens;
    int count;
    int capacity;
} TokenList;
```

---

### Phase B: Syntax Analysis (Recursive Descent Parser)
*Files: `parser.c`, `parser.h`*

The parser converts the flat list of tokens (`TokenList`) into a tree-like hierarchy known as the **Abstract Syntax Tree (AST)**. 

#### Grammar & Operator Precedence:
To prevent grammar ambiguity (e.g., ensuring `1 + 2 * 3` evaluates as `1 + (2 * 3)` rather than `(1 + 2) * 3`), the parser uses a strict **operator precedence hierarchy** from lowest to highest:

$$\text{Expression} \rightarrow \text{Comparison} \rightarrow \text{Addition} \rightarrow \text{Multiplication} \rightarrow \text{Primary}$$

This grammar is mapped directly into parsing functions:
*   `parse_expression()` parses comparisons.
*   `parse_comparison()` loops through `==`, `!=`, `<`, `>`, `<=`, `>=`.
*   `parse_addition()` loops through `+` and `-`.
*   `parse_multiplication()` loops through `*` and `/`.
*   `parse_primary()` processes constants, identifiers (variables/function calls), and parenthesized expressions `( expression )`.

#### Visualizing an AST Node Hierarchy:
For the statement `x = 10 + 20 * y;`, the parser constructs the following nodes:

```text
                     Statement (STMT_ASSIGN, name="x")
                                     │
                                     ▼
                           Expr (EXPR_BINARY, op="+")
                                 /       \
                               /           \
                             /               \
              Expr (EXPR_NUMBER, value=10)   Expr (EXPR_BINARY, op="*")
                                                   /       \
                                                 /           \
                                  Expr (EXPR_NUMBER, val=20)  Expr (EXPR_VAR, name="y")
```

---

### Phase C: Code Generation (Stack-Based Assembly Generator)
*Files: `codegen.c`, `codegen.h`*

The code generator translates the AST nodes into sequential machine instructions. Rather than implementing complex register allocation, `mini_cc` uses a **stack-based evaluation design** to perform expressions.

#### 1. System V AMD64 ABI Compliance:
Our compiler targets x86-64 assembly and conforms to standard calling conventions:
*   **Caller Arguments**: Passed in sequence using registers `RDI`, `RSI`, `RDX`, `RCX`, `R8`, and `R9`.
*   **Return Values**: Stored and passed back in the `RAX` register.

#### 2. Stack Frame Layout (`RBP` Relative Variables):
When a function is called, a local frame is configured (the prologue). Variable locations on the stack are statically allocated offsets relative to the **Base Frame Pointer (`RBP`)**:

```text
High Address ┌──────────────────────┐
             │ Caller Stack Frame   │
             ├──────────────────────┤
             │ Return Address       │
             ├──────────────────────┤
             │ Saved Caller RBP     │ <-- RBP Points Here
             ├──────────────────────┤
             │ Local Var x (rbp-8)  │
             ├──────────────────────┤
             │ Local Var y (rbp-16) │
             ├──────────────────────┤
             │ Temporary Stack      │ <-- RSP (Stack Pointer) Moves Here
Low Address  └──────────────────────┘
```

#### 3. Stack-Based Expression Evaluation:
Expressions are evaluated recursively, with results always finalized in `RAX`. When a binary operation (like `Left OP Right`) is executed:
1.  Recursively evaluate the **Left Expression**. The result is now in `RAX`.
2.  Emit `push rax` to save this result on the hardware stack.
3.  Recursively evaluate the **Right Expression**. The result is now in `RAX`.
4.  Emit `mov rcx, rax` to transfer the right-hand operand to `RCX`.
5.  Emit `pop rax` to recover the saved left-hand operand from the stack back into `RAX`.
6.  Emit the operation (e.g., `add rax, rcx` or `imul rax, rcx`). The final merged result resides in `RAX`.

#### 4. Control Flow (Loops & Conditionals):
Control flow constructs use jump instructions (`jmp`, `je`) and conditional flags to jump over labels.
*   **If-Else**: Evaluates the condition. If it is 0, jump (`je`) to the `else` label. Otherwise, fall through to the `then` body, jump (`jmp`) to the `end` label, emit the `else` label, execute its body, and finally emit the `end` label.
*   **While Loops**: Emits a loop start label, evaluates the condition, and jumps to the end label if the condition is 0. Otherwise, it executes the block and loops back via `jmp` to the loop start label.

---

## 4. File Directory Tour

*   **`main.c`**: Entry point of the program. Performs arguments validation, reads inputs, and drives the compiler interface.
*   **`compiler.c` / `compiler.h`**: The top-level compilation controller. Handles binary file loading (`"rb"`), starts the memory allocations, coordinates scanner, parser, and code-generation, and cleans up the buffers.
*   **`scanner.c` / `scanner.h`**: Responsible for converting raw files into structural tokens. Defines all keywords, operators, and dynamic memory list rules.
*   **`parser.c` / `parser.h`**: Contains the grammar and nodes definitions for building AST branches using Recursive Descent.
*   **`codegen.c` / `codegen.h`**: Emits System V compatible x86-64 assembly instructions to output. Does stack adjustments, parameters initialization, and controls labels.

---

## 5. End-to-End Walkthrough: `test1.c`

Let us trace how the compiler compiles the arithmetic in `test1.c`:

```c
int main()
{
    int x;
    x = 10 + 20;
    return x;
}
```

### The Emitted Assembly (Annotated `output.txt`):
```assembly
.intel_syntax noprefix

.globl main
main:
    push rbp               # Save caller's frame pointer
    mov rbp, rsp           # Create local stack frame
    sub rsp, 16            # Allocate 16 bytes on stack (aligned to 16-bytes)
    
    ; int x at [rbp-8]     # Symbol declaration comment
    
    mov rax, 10            # Evaluate Left Operand (10)
    push rax               # Save 10 to stack
    mov rax, 20            # Evaluate Right Operand (20)
    mov rcx, rax           # Move 20 to RCX
    pop rax                # Retrieve 10 back to RAX
    add rax, rcx           # RAX = RAX (10) + RCX (20)
    
    mov QWORD PTR [rbp-8], rax # Assignment: Write 30 into variable 'x'
    
    mov rax, QWORD PTR [rbp-8] # Load 'x' (30) to RAX (return register)
    mov rsp, rbp           # Restore Stack Pointer (deallocate frame)
    pop rbp                # Restore caller's Base Pointer
    ret                    # Return back to operating system / wrapper
```

---

## 6. Build, Test, and Execution Guide

### Compilation & Building
To compile the compiler binary itself using standard GCC:
```bash
gcc -std=c11 -Wall -Wextra main.c compiler.c scanner.c parser.c codegen.c -o mini_cc
```

### Compiling a C Script
Use your compiled compiler binary on any local target source:
```bash
./mini_cc tests/test5.c
```
On success, this outputs assembly text in `output.txt`.

### Linking & Execution (Using WSL / Linux GCC)
Because the output assembly conforms to Linux x86-64 calling conventions and ABI, you must run and link it in a Linux environment or within **WSL (Windows Subsystem for Linux)**:

1.  Enter WSL:
    ```bash
    wsl
    ```
2.  Compile the generated assembly into an executable (using `gcc` to assemble and link standard system dependencies):
    ```bash
    gcc -no-pie output.txt -o my_executable
    ```
3.  Run the executable and verify the returned code:
    ```bash
    ./my_executable
    echo $?  # Prints the return code of main()
    ```

---

## License

This project is licensed under the MIT License.