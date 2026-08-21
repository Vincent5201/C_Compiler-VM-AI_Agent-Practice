# `mini_cc`: An Educational Compiler & Virtual Machine

Welcome to **`mini_cc`**, an educational, self-contained compiler and x86-64 instruction set emulator implemented entirely from scratch in pure C. It compiles a structured subset of C into Intel-syntax x86-64 assembly targeting the Linux System V AMD64 ABI, and then executes it using a built-in custom virtual machine (interpreter).

As an **educational playground**, `mini_cc` completely avoids third-party generator tools like Flex (lexical scanners) or Bison/Yacc (LALR parsers). Writing every component by hand offers a highly transparent, ground-up perspective on compiler engineering, structural parsing, abstract syntax trees (AST), stack-machine code generation, and instruction set emulation.

---

## Table of Contents
1. [Overall Pipeline & Architecture](#1-overall-pipeline--architecture)
2. [Language & Grammar Specifications](#2-language--grammar-specifications)
3. [Phase 1: Lexical Analysis (The Scanner)](#3-phase-1-lexical-analysis-the-scanner)
4. [Phase 2: Syntactic Analysis (The Parser)](#4-phase-2-syntactic-analysis-the-parser)
5. [Phase 3: Code Generation (The CodeGen)](#5-phase-3-code-generation-the-codegen)
6. [Phase 4: Execution (The Assembly Interpreter/VM)](#6-phase-4-execution-the-assembly-interpretervm)
7. [Step-by-Step Educational Walkthrough](#7-step-by-step-educational-walkthrough)
8. [Build, Test, and Execution Guide](#8-build-test-and-execution-guide)

---

## 1. Overall Pipeline & Architecture

When running `mini_cc` with a C source file, the program passes through five sequential stages:

```text
       C Source Text File (e.g. test.c)
                     │
                     ▼
           ┌──────────────────┐
           │     Scanner      │ (Lexical Analysis)
           │   `scanner.c`    │ Parses raw chars into a sequential list of tokens.
           └──────────────────┘
                     │
                     ▼
           ┌──────────────────┐
           │      Parser      │ (Syntactic Analysis)
           │    `parser.c`    │ Builds an Abstract Syntax Tree (AST) using
           └──────────────────┘ Recursive Descent.
                     │
                     ▼
           ┌──────────────────┐
           │     CodeGen      │ (Assembly Synthesis)
           │   `codegen.c`    │ Translates AST into a flat stack-machine format 
           └──────────────────┘ emitting Intel x86-64 assembly (`output.txt`).
                     │
                     ▼
           ┌──────────────────┐
           │   Interpreter    │ (Virtual Machine Execution)
           │  `interpreter.c` │ Decodes and executes the assembly instructions 
           └──────────────────┘ in a simulated CPU core with virtual RAM.
                     │
                     ▼
             Execution Result (main return value printed to stdout)
```

---

## 2. Language & Grammar Specifications

To keep the compiler simple yet mathematically rich, `mini_cc` implements a clean subset of the C language.

### Supported Language Features
*   **Datatypes**: Only 64-bit signed integers (`int` internally maps to x86-64 QWORDs).
*   **Control Flow**: Conditional structures (`if`, `else`) and loop structures (`while`).
*   **Arithmetic Operators**: `+`, `-`, `*`, `/` (division implements standard signed integer division).
*   **Logical Comparisons**: `==`, `!=`, `<`, `>`, `<=`, `>=`.
*   **Functions**: Declaring functions, passing up to 6 parameters (passed via System V registers), and local block scoped variables.

### The Formal Context-Free Grammar (EBNF-like)
The syntactic structure is defined by the following formal grammar rules:

```ebnf
Program        ::= Function*
Function       ::= "int" Identifier "(" ParameterList? ")" Block
ParameterList  ::= "int" Identifier ("," "int" Identifier)*
Block          ::= "{" Statement* "}"

Statement      ::= "int" Identifier ";"                                     /* Variable declaration */
                 | Identifier "=" Expression ";"                            /* Variable assignment  */
                 | "return" Expression ";"                                  /* Return statement     */
                 | "if" "(" Expression ")" Block ("else" Block)?            /* Conditional branch   */
                 | "while" "(" Expression ")" Block                         /* Loop iteration       */

Expression     ::= Comparison
Comparison     ::= Addition (ComparisonOp Addition)*
ComparisonOp   ::= "==" | "!=" | "<" | ">" | "<=" | ">="
Addition       ::= Multiplication (("+" | "-") Multiplication)*
Multiplication ::= Primary (("*" | "/") Primary)*
Primary        ::= Number
                 | Identifier "(" ArgumentList? ")"                         /* Function call        */
                 | Identifier                                               /* Variable reference   */
                 | "(" Expression ")"                                       /* Parenthesized group  */
ArgumentList   ::= Expression ("," Expression)*
```

---

## 3. Phase 1: Lexical Analysis (The Scanner)

*Files: `scanner.h`, `scanner.c`*

The **Scanner**'s job is **tokenization**. It reads raw stream files containing strings of code character-by-character and groups them into logical units called **Tokens**. 

### Under the Hood
1.  **Whitespace Skipping**: Ignores space `' '`, tabs `'\t'`, and newlines `'\n'`.
2.  **Identifier & Keyword Resolution**: When scanning letters (`[a-zA-Z_]`), it aggregates characters sequentially to match words. It then checks if the resulting word is a keyword (`int`, `return`, `if`, `else`, `while`). If not, it becomes a `TOKEN_IDENTIFIER`.
3.  **Numbers**: Strings of digits (`[0-9]`) are converted into numeric literal tokens (`TOKEN_NUMBER`).
4.  **Operator Lookahead (Maximal Munch)**: In lexical scanning, operators like `=` and `==`, `<` and `<=`, `!` and `!=` have overlapping prefixes. The scanner looks one character ahead. If the next character completes a dual character operator, it consumes both to yield a double token (e.g. `TOKEN_EQ` for `==`). Otherwise, it yields a single token (e.g. `TOKEN_ASSIGN` for `=`).
5.  **EOF Guard**: Appends a `TOKEN_EOF` to mark the logical end of compile scanning.

---

## 4. Phase 2: Syntactic Analysis (The Parser)

*Files: `parser.h`, `parser.c`*

The **Parser** reads the token list sequentially and validates if the sentence structures match the formal grammar definitions. It constructs an **Abstract Syntax Tree (AST)**—a hierarchical tree representing the logical flow of the code.

```text
                  AST Node representation of `x = 10 + 20`
                  
                               Statement (ASSIGN)
                                 ├── Name: "x"
                                 └── Expr (BINARY "+")
                                       ├── Left: Expr (NUMBER: 10)
                                       └── Right: Expr (NUMBER: 20)
```

### The Recursive Descent Technique
Recursive Descent is a top-down parsing pattern where the parser contains mutually recursive C functions representing grammar rules. 

#### Enforcing Operator Precedence (The Precedence Ladder)
To parse mathematical expressions such as `2 * 3 + 4` and guarantee `*` binds tighter than `+`, we chain parse functions together:
*   `parse_expression()` delegates to `parse_comparison()`.
*   `parse_comparison()` evaluates relational operators (`==`, `!=`, etc.) and delegates to `parse_addition()`.
*   `parse_addition()` handles (`+`, `-`) and delegates to `parse_multiplication()`.
*   `parse_multiplication()` handles (`*`, `/`) and delegates to `parse_primary()`.
*   `parse_primary()` processes atomic units like literals, parenthesized expressions, or variables.

This architectural chain creates a parse hierarchy where nodes located lower in the ladder are nested deeper inside the AST, forcing them to execute first.

---

## 5. Phase 3: Code Generation (The CodeGen)

*Files: `codegen.h`, `codegen.c`*

The **CodeGen** traverses the hierarchical AST starting from the root node and translates the compiler's logical tree representation into a flat list of System V AMD64 Intel assembly instructions.

### The Stack Machine Architecture Model
Because x86-64 has a limited set of physical registers, implementing complex nested expressions (like `(a + b) * (c - d)`) directly using register-allocation algorithms is complex. Instead, `mini_cc` uses a **Stack Machine Model** for expression generation:
1.  To evaluate any expression, we recursively generate instructions for the sub-nodes.
2.  The resulting evaluation of any sub-expression is placed in register `RAX`.
3.  We `push rax` onto the CPU stack.
4.  We evaluate the right-side expression, placing the result in `RAX`.
5.  We move the right-side result into a temporary register (e.g. `RCX`).
6.  We `pop rax` to recover the left-side evaluation.
7.  We perform the mathematical operation between `RAX` and `RCX`, with the result residing back in `RAX`.

### Variables & Stack Frame Allocation (RBP offsets)
Every local variable is mapped to a location on the stack frame relative to the base frame pointer `RBP`. 
*   **Variable Collection**: Prior to generating assembly for a function, `collect_variables()` pre-scans all statements in the function's AST to allocate stack space.
*   **Stack Alignment**: Modern operating systems and compiler protocols require stack pointers to be 16-byte aligned before calling functions. CodeGen enforces this by padding `stack_size` to a multiple of 16 bytes:
    ```c
    if (stack_size % 16 != 0)
        stack_size += 16 - (stack_size % 16);
    ```

---

## 6. Phase 4: Execution (The Assembly Interpreter/VM)

*Files: `interpreter.h`, `interpreter.c`*

Rather than relying on local assembly tools or host operating systems to run code, `mini_cc` embeds a custom virtual machine designed to emulate an x86-64 microprocessor.

### VM Core Components
*   **Registers**: Struct mapping standard x86 registers (`rax`, `rbx`, `rcx`, `rdx`, `rbp`, `rsp`, `rip`, etc.).
*   **Virtual RAM**: A block of allocated memory (2 MB) acting as the computer's physical address space.
*   **Stack Pointer (`RSP`)**: Set to start at the top of memory (`MEMORY_SIZE - 16`) growing downwards.
*   **CPU Flag registers (EFLAGS)**: Simulates standard ALU evaluation flags (`ZF` (Zero), `SF` (Sign), `OF` (Overflow), `CF` (Carry)).
*   **The Execution Loop**: Matches instructions mapped by the instruction pointer `rip`, decodes arguments, runs address calculations, modifies memory/registers, and adjusts ALU flags.

### Defect Security: Safe Virtual RAM Boundaries
To prevent boundary leaks or buffer overflow exploits inside VM virtual registers, memory reads and writes use a secure bounds checking rule that is proof against integer overflow wrap-around:
```c
if (address > (u64)MEMORY_SIZE - (u64)size)
    error("memory write/read out of range");
```

---

## 7. Step-by-Step Educational Walkthrough

Let's trace how the compiler compiles and runs this program:

```c
int main() {
    int x;
    x = 10 + 20;
    return x;
}
```

### Step 1: Lexer Output (Tokens)
The scanner identifies the following token chain:
`TOKEN_INT`, `TOKEN_IDENTIFIER("main")`, `TOKEN_LPAREN`, `TOKEN_RPAREN`, `TOKEN_LBRACE`, `TOKEN_INT`, `TOKEN_IDENTIFIER("x")`, `TOKEN_SEMICOLON`, `TOKEN_IDENTIFIER("x")`, `TOKEN_ASSIGN`, `TOKEN_NUMBER("10")`, `TOKEN_PLUS`, `TOKEN_NUMBER("20")`, `TOKEN_SEMICOLON`, `TOKEN_RETURN`, `TOKEN_IDENTIFIER("x")`, `TOKEN_SEMICOLON`, `TOKEN_RBRACE`, `TOKEN_EOF`.

### Step 2: Parser Output (AST)
The AST structure represents:
*   **Function Name**: `main`
    *   **Local Variable Declarations**:
        *   `STMT_VAR_DECL` Name: "x" (Mapped to `rbp - 8` on the stack frame)
    *   **Body Statements**:
        *   `STMT_ASSIGN` Variable: "x"
            *   Value: `EXPR_BINARY("+")`
                *   Left: `EXPR_NUMBER(10)`
                *   Right: `EXPR_NUMBER(20)`
        *   `STMT_RETURN`
            *   Expression: `EXPR_VARIABLE("x")`

### Step 3: CodeGen Output (Intel Assembly)
```assembly
.intel_syntax noprefix

.globl main
main:
    push rbp                     # Save caller's base pointer
    mov rbp, rsp                 # Establish stack frame boundary
    sub rsp, 16                  # Allocate space for local variables (16-byte aligned)
    
    # Evaluate x = 10 + 20
    mov rax, 10                  # Load immediate 10 into RAX
    push rax                     # Push RAX to temporary stack location
    mov rax, 20                  # Load immediate 20 into RAX
    mov rcx, rax                 # RCX is loaded with right-side result (20)
    pop rax                      # Pop left-side value (10) back to RAX
    add rax, rcx                 # Add RCX to RAX (RAX becomes 30)
    mov QWORD PTR [rbp-8], rax   # Assign result (30) to variable 'x' on stack
    
    # Evaluate return x
    mov rax, QWORD PTR [rbp-8]   # Load value of 'x' (30) into RAX
    mov rsp, rbp                 # Teardown stack frame
    pop rbp                      # Restore caller's base pointer
    ret                          # Return from function
```

### Step 4: Interpreter Emulator Execution
The custom VM decodes and executes `main` instruction by instruction:
1.  `push rbp` decreases `RSP` by 8 and writes current `RBP` to virtual memory.
2.  `mov rbp, rsp` aligns `RBP` with `RSP` to establish the frame.
3.  `sub rsp, 16` reserves stack space. Variable `x` is assigned to `[rbp-8]`.
4.  Evaluates `10 + 20` using the VM register instructions, saving the result `30` in the simulated RAM address corresponding to `[rbp-8]`.
5.  `return x` moves `30` into VM register `rax`.
6.  The stack frame is cleared, and `ret` halts VM execution, outputting the return code `30`.

---

## 8. Build, Test, and Execution Guide

### Using the Makefile
The project includes a multi-platform `Makefile` to compile the compiler:

*   **Build the compiler executable**:
    ```bash
    make
    ```
    This outputs the executable binary `mini_cc` (or `mini_cc.exe` on Windows).

*   **Run the compiler tests**:
    ```bash
    make test
    ```
    This automatically builds the compiler and compiles all tests located in the `tests/` directory, interpreting their output.

*   **Cleanup build binaries**:
    ```bash
    make clean
    ```

### Manual Compilation
If `make` is not available, you can compile the files manually using `gcc`:
```bash
gcc -std=c11 -Wall -Wextra main.c compiler.c scanner.c parser.c codegen.c interpreter.c -o mini_cc
```

### Running the Compiler
Execute a C program through the compiler directly:
```bash
./mini_cc tests/test1.c
```
The compilation console prints:
```text
Compilation successful.
Execution result: 30
```
