# Mini C Compiler

A simple C compiler implemented in C for compiler practice.

## Architecture

```text
C Source (.c)
     ↓
  Scanner
     ↓
   Parser
     ↓
    AST
     ↓
Code Generator
     ↓
Assembly (.txt)
```

- **Scanner**: Converts source code into tokens.
- **Parser**: Converts tokens into an Abstract Syntax Tree (AST).
- **Code Generator**: Converts the AST into x86-64 assembly.
- **main.c**: Reads the input `.c` file and starts compilation through `compile_file()`.

## Project Structure

```text
mini_cc/
├── main.c
├── compiler.c / compiler.h
├── scanner.c / scanner.h
├── parser.c / parser.h
└── codegen.c / codegen.h
```

## Supported Features

- `int` variables
- Assignment
- `+ - * /`
- `== != < > <= >=`
- `if / else`
- `while`
- Functions and parameters
- Function calls
- `return`

## Register Management

The compiler targets **x86-64** and uses the **System V AMD64 ABI**.

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

Temporary values are stored on the stack when necessary, while local variables are
stored at offsets from `RBP`.

## Build

```bash
gcc -std=c11 -Wall -Wextra \
    main.c compiler.c scanner.c parser.c codegen.c \
    -o mini_cc
```

## Run

```bash
./mini_cc test.c
```

The generated assembly is written to:

```text
output.txt
```

## Example

Input:

```c
int main()
{
    int x;
    x = 10 + 20;
    return x;
}
```

Output:

```text
output.txt
```

contains the generated x86-64 assembly.

> This is an educational compiler. It intentionally does not implement pointers,
> arrays, structs, floating-point types, optimization, or advanced register allocation.