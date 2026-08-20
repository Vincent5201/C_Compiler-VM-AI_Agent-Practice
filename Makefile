# ==============================================================================
# Mini C Compiler Makefile
# ==============================================================================
# Supports both Unix/Linux/macOS and Windows environments.

# Compiler and Flags
CC = gcc
CFLAGS = -std=c11 -Wall -Wextra
SRCS = main.c compiler.c scanner.c parser.c codegen.c
TARGET = mini_cc

# OS Detection
ifeq ($(OS),Windows_NT)
    BINARY = $(TARGET).exe
    RM_CMD = del /Q /F
else
    BINARY = $(TARGET)
    RM_CMD = rm -f
endif

.PHONY: all clean test

# Default build target
all: $(BINARY)

# Compile the mini_cc executable
$(BINARY): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(BINARY)

# Run tests in the tests folder
test: $(BINARY)
	@echo ==============================================================================
	@echo Running Mini C Compiler tests...
	@echo ==============================================================================
	@echo [Test 1] Compiling tests/test1.c ...
	./$(BINARY) tests/test1.c
	@echo ------------------------------------------------------------------------------
	@echo [Test 2] Compiling tests/test2.c ...
	./$(BINARY) tests/test2.c
	@echo ------------------------------------------------------------------------------
	@echo [Test 3] Compiling tests/test3.c ...
	./$(BINARY) tests/test3.c
	@echo ------------------------------------------------------------------------------
	@echo [Test 4] Compiling tests/test4.c ...
	./$(BINARY) tests/test4.c
	@echo ==============================================================================
	@echo All tests compiled successfully!
	@echo Generated assembly was written to output.txt.
	@echo ==============================================================================

# Clean build artifacts (supports both cmd/PowerShell and bash)
clean:
	-$(RM_CMD) $(BINARY) output.txt 2>nul
	-rm -f $(BINARY) output.txt
