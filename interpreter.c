#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>

#define MAX_INSTRUCTIONS 2000
#define MAX_LABELS 200
#define MAX_LINE 256
#define MAX_ARGS 3

#define MEMORY_SIZE (2 * 1024 * 1024) /* 2 MB virtual RAM */
#define MAX_STEPS 1000000ULL          /* Safety loop limit for interpreter */

typedef uint64_t u64;
typedef int64_t s64;

/* ============================================================
 * Register Storage
 * ============================================================ */

typedef struct {
    u64 rax;
    u64 rbx;
    u64 rcx;
    u64 rdx;

    u64 rsi;
    u64 rdi;

    u64 rbp;
    u64 rsp;

    u64 r8;
    u64 r9;
    u64 r10;
    u64 r11;
    u64 r12;
    u64 r13;
    u64 r14;
    u64 r15;
} Registers;


/* ============================================================
 * Instruction Struct
 * ============================================================ */

typedef struct {
    char op[32];
    char args[MAX_ARGS][128];
    int argc;
    char source[MAX_LINE];
} Instruction;


/* ============================================================
 * Label Storage
 * ============================================================ */

typedef struct {
    char name[128];
    size_t instruction_index;
} Label;


/* ============================================================
 * Program Structure
 * ============================================================ */

typedef struct {
    Instruction instructions[MAX_INSTRUCTIONS];
    size_t instruction_count;

    Label labels[MAX_LABELS];
    size_t label_count;
} Program;


/* ============================================================
 * CPU State Struct (Virtual Machine)
 * ============================================================ */

typedef struct {
    Registers reg;
    uint8_t *memory;
    size_t rip; /* Instruction pointer */

    /* EFLAGS conditional bits */
    int ZF; /* Zero flag */
    int SF; /* Sign flag */
    int OF; /* Overflow flag */
    int CF; /* Carry flag */

    unsigned call_depth;
    int halted;
    s64 return_value;
} CPU;


/* ============================================================
 * Utility
 * ============================================================ */

static void error(const char *msg)
{
    fprintf(stderr, "Mini x86 error: %s\n", msg);
    exit(EXIT_FAILURE);
}

static void error_line(const char *msg, const char *line)
{
    fprintf(stderr, "Mini x86 error: %s\n", msg);
    fprintf(stderr, "    %s\n", line);
    exit(EXIT_FAILURE);
}

static char *trim(char *s)
{
    char *end;

    while (isspace((unsigned char)*s))
        s++;

    if (*s == '\0')
        return s;

    end = s + strlen(s) - 1;

    while (end > s && isspace((unsigned char)*end))
        end--;

    end[1] = '\0';

    return s;
}

static int equals_ignore_case(const char *a, const char *b)
{
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b))
            return 0;
        a++;
        b++;
    }
    return *a == '\0' && *b == '\0';
}

static u64 parse_number(const char *s)
{
    char *end;
    long long value;

    value = strtoll(s, &end, 0);

    if (*end != '\0')
        error("invalid integer");

    return (u64)value;
}


/* ============================================================
 * Register Mapping
 * ============================================================ */

/* Resolves standard 64-bit register pointer by its assembly name */
static u64 *get_register_ptr(Registers *r, const char *name)
{
    if (equals_ignore_case(name, "rax")) return &r->rax;
    if (equals_ignore_case(name, "rbx")) return &r->rbx;
    if (equals_ignore_case(name, "rcx")) return &r->rcx;
    if (equals_ignore_case(name, "rdx")) return &r->rdx;

    if (equals_ignore_case(name, "rsi")) return &r->rsi;
    if (equals_ignore_case(name, "rdi")) return &r->rdi;

    if (equals_ignore_case(name, "rbp")) return &r->rbp;
    if (equals_ignore_case(name, "rsp")) return &r->rsp;

    if (equals_ignore_case(name, "r8"))  return &r->r8;
    if (equals_ignore_case(name, "r9"))  return &r->r9;
    if (equals_ignore_case(name, "r10")) return &r->r10;
    if (equals_ignore_case(name, "r11")) return &r->r11;
    if (equals_ignore_case(name, "r12")) return &r->r12;
    if (equals_ignore_case(name, "r13")) return &r->r13;
    if (equals_ignore_case(name, "r14")) return &r->r14;
    if (equals_ignore_case(name, "r15")) return &r->r15;

    return NULL;
}

/* Reads CPU register value supporting 64-bit, 32-bit (zero-extended) and 8-bit registers */
static u64 read_register(CPU *cpu, const char *name)
{
    u64 *ptr = get_register_ptr(&cpu->reg, name);
    if (ptr != NULL) return *ptr;

    /* 32-bit registers mapped directly to 64-bit lower halves */
    if (equals_ignore_case(name, "eax")) return (uint32_t)cpu->reg.rax;
    if (equals_ignore_case(name, "ebx")) return (uint32_t)cpu->reg.rbx;
    if (equals_ignore_case(name, "ecx")) return (uint32_t)cpu->reg.rcx;
    if (equals_ignore_case(name, "edx")) return (uint32_t)cpu->reg.rdx;
    if (equals_ignore_case(name, "esi")) return (uint32_t)cpu->reg.rsi;
    if (equals_ignore_case(name, "edi")) return (uint32_t)cpu->reg.rdi;
    if (equals_ignore_case(name, "ebp")) return (uint32_t)cpu->reg.rbp;
    if (equals_ignore_case(name, "esp")) return (uint32_t)cpu->reg.rsp;

    /* 8-bit registers mapped directly to lowest byte */
    if (equals_ignore_case(name, "al")) return cpu->reg.rax & 0xff;
    if (equals_ignore_case(name, "bl")) return cpu->reg.rbx & 0xff;
    if (equals_ignore_case(name, "cl")) return cpu->reg.rcx & 0xff;
    if (equals_ignore_case(name, "dl")) return cpu->reg.rdx & 0xff;

    error("unknown register");
    return 0;
}

/* Updates CPU register value. Writing to 32-bit zero-extends to 64-bit automatically */
static void write_register(CPU *cpu, const char *name, u64 value)
{
    u64 *ptr = get_register_ptr(&cpu->reg, name);
    if (ptr != NULL) {
        *ptr = value;
        return;
    }

    if (equals_ignore_case(name, "eax")) { cpu->reg.rax = (uint32_t)value; return; }
    if (equals_ignore_case(name, "ebx")) { cpu->reg.rbx = (uint32_t)value; return; }
    if (equals_ignore_case(name, "ecx")) { cpu->reg.rcx = (uint32_t)value; return; }
    if (equals_ignore_case(name, "edx")) { cpu->reg.rdx = (uint32_t)value; return; }
    if (equals_ignore_case(name, "esi")) { cpu->reg.rsi = (uint32_t)value; return; }
    if (equals_ignore_case(name, "edi")) { cpu->reg.rdi = (uint32_t)value; return; }
    if (equals_ignore_case(name, "ebp")) { cpu->reg.rbp = (uint32_t)value; return; }
    if (equals_ignore_case(name, "esp")) { cpu->reg.rsp = (uint32_t)value; return; }

    if (equals_ignore_case(name, "al")) { cpu->reg.rax = (cpu->reg.rax & ~0xffULL) | (value & 0xff); return; }
    if (equals_ignore_case(name, "bl")) { cpu->reg.rbx = (cpu->reg.rbx & ~0xffULL) | (value & 0xff); return; }
    if (equals_ignore_case(name, "cl")) { cpu->reg.rcx = (cpu->reg.rcx & ~0xffULL) | (value & 0xff); return; }
    if (equals_ignore_case(name, "dl")) { cpu->reg.rdx = (cpu->reg.rdx & ~0xffULL) | (value & 0xff); return; }

    error("unknown register");
}


/* ============================================================
 * Virtual RAM Access (with secure integer-overflow check)
 * ============================================================ */

static u64 memory_read(CPU *cpu, u64 address, int size)
{
    u64 value = 0;

    /* Prevent integer overflow bypass during bounds checking */
    if (address > (u64)MEMORY_SIZE - (u64)size)
        error("memory read out of range");

    for (int i = 0; i < size; i++) {
        value |= ((u64)cpu->memory[address + i]) << (i * 8);
    }

    return value;
}

static void memory_write(CPU *cpu, u64 address, u64 value, int size)
{
    /* Prevent integer overflow bypass during bounds checking */
    if (address > (u64)MEMORY_SIZE - (u64)size)
        error("memory write out of range");

    for (int i = 0; i < size; i++) {
        cpu->memory[address + i] = (uint8_t)((value >> (i * 8)) & 0xff);
    }
}


/* ============================================================
 * Memory Operand Address Parsing
 *
 * Support patterns: [rbp-8], [rbp-16], [rsp], [rax+8]
 * ============================================================ */

static u64 memory_address(CPU *cpu, const char *operand)
{
    const char *left = strchr(operand, '[');
    const char *right = strchr(operand, ']');

    if (!left || !right)
        error("invalid memory operand");

    size_t len = right - left - 1;
    char expression[256];

    if (len >= sizeof(expression))
        error("memory expression too long");

    memcpy(expression, left + 1, len);
    expression[len] = '\0';

    char compact[256];
    int j = 0;
    for (size_t i = 0; expression[i] != '\0'; i++) {
        if (!isspace((unsigned char)expression[i]))
            compact[j++] = expression[i];
    }
    compact[j] = '\0';
    strcpy(expression, compact);

    char base[64];
    long offset = 0;
    char *plus = strchr(expression, '+');
    char *minus = strchr(expression, '-');
    char *sign = plus ? plus : minus;

    if (sign != NULL) {
        size_t base_len = sign - expression;
        if (base_len >= sizeof(base))
            error("invalid memory base");

        memcpy(base, expression, base_len);
        base[base_len] = '\0';
        offset = strtol(sign, NULL, 10);
    } else {
        strcpy(base, expression);
    }

    u64 base_value = read_register(cpu, base);
    return base_value + offset;
}


/* ============================================================
 * Operand Decoding
 * ============================================================ */

static int operand_size(const char *operand)
{
    if (strstr(operand, "QWORD PTR") || strstr(operand, "qword ptr")) return 8;
    if (strstr(operand, "DWORD PTR") || strstr(operand, "dword ptr")) return 4;
    if (strstr(operand, "WORD PTR")  || strstr(operand, "word ptr"))  return 2;
    if (strstr(operand, "BYTE PTR")  || strstr(operand, "byte ptr"))  return 1;
    return 8;
}

/* Fetches operand value: handles Memory [address], Register, or Immediate constants */
static u64 read_operand(CPU *cpu, const char *operand)
{
    operand = trim((char *)operand);

    if (strchr(operand, '[') != NULL) {
        int size = operand_size(operand);
        u64 address = memory_address(cpu, operand);
        return memory_read(cpu, address, size);
    }

    if (get_register_ptr(&cpu->reg, operand) != NULL ||
        equals_ignore_case(operand, "eax") || equals_ignore_case(operand, "ebx") ||
        equals_ignore_case(operand, "ecx") || equals_ignore_case(operand, "edx") ||
        equals_ignore_case(operand, "esi") || equals_ignore_case(operand, "edi") ||
        equals_ignore_case(operand, "ebp") || equals_ignore_case(operand, "esp") ||
        equals_ignore_case(operand, "al")  || equals_ignore_case(operand, "bl")  ||
        equals_ignore_case(operand, "cl")  || equals_ignore_case(operand, "dl")) {
        return read_register(cpu, operand);
    }

    return parse_number(operand);
}

/* Writes value back to either virtual Memory or Register location */
static void write_operand(CPU *cpu, const char *operand, u64 value)
{
    operand = trim((char *)operand);

    if (strchr(operand, '[') != NULL) {
        int size = operand_size(operand);
        u64 address = memory_address(cpu, operand);
        memory_write(cpu, address, value, size);
        return;
    }

    write_register(cpu, operand, value);
}


/* ============================================================
 * Stack Emulator
 * ============================================================ */

static void push(CPU *cpu, u64 value)
{
    cpu->reg.rsp -= 8;
    memory_write(cpu, cpu->reg.rsp, value, 8);
}

static u64 pop(CPU *cpu)
{
    u64 value = memory_read(cpu, cpu->reg.rsp, 8);
    cpu->reg.rsp += 8;
    return value;
}


/* ============================================================
 * Assembly Opcode Argument Splitting
 * ============================================================ */

static int split_args(const char *text, char args[][128])
{
    int count = 0;
    int bracket_depth = 0;
    size_t start = 0;
    size_t len = strlen(text);

    for (size_t i = 0; i <= len; i++) {
        char c = text[i];

        if (c == '[') bracket_depth++;
        if (c == ']') bracket_depth--;

        if ((c == ',' && bracket_depth == 0) || c == '\0') {
            size_t n = i - start;

            if (n >= 128)
                error("argument too long");

            memcpy(args[count], text + start, n);
            args[count][n] = '\0';
            trim(args[count]);

            count++;
            if (count >= MAX_ARGS)
                error("too many operands");

            start = i + 1;
        }
    }

    return count;
}


/* ============================================================
 * Label Table Map
 * ============================================================ */

static void add_label(Program *program, const char *name)
{
    if (program->label_count >= MAX_LABELS)
        error("too many labels");

    strcpy(program->labels[program->label_count].name, name);
    program->labels[program->label_count].instruction_index = program->instruction_count;
    program->label_count++;
}

static size_t find_label(const Program *program, const char *name)
{
    for (size_t i = 0; i < program->label_count; i++) {
        if (strcmp(program->labels[i].name, name) == 0) {
            return program->labels[i].instruction_index;
        }
    }

    fprintf(stderr, "Mini x86 error: unknown label %s\n", name);
    exit(EXIT_FAILURE);
}


/* ============================================================
 * Assembly File Parser
 * ============================================================ */

static void parse_program(Program *program, const char *filename)
{
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror(filename);
        exit(EXIT_FAILURE);
    }

    char line[MAX_LINE];

    while (fgets(line, sizeof(line), fp)) {
        char original[MAX_LINE];
        strcpy(original, line);

        char *s = trim(line);
        if (*s == '\0') continue;

        char *comment = strchr(s, ';');
        if (comment) *comment = '\0';

        s = trim(s);
        if (*s == '\0') continue;

        /* Check label declarations first to handle dot-prefixed branches (.L0:) */
        size_t len = strlen(s);
        if (len > 0 && s[len - 1] == ':') {
            s[len - 1] = '\0';
            s = trim(s);
            add_label(program, s);
            continue;
        }

        /* Ignore assembler directives */
        if (strncmp(s, ".intel_syntax", 13) == 0 ||
            strncmp(s, ".globl", 6) == 0 ||
            *s == '.') continue;

        char *space = s;
        while (*space && !isspace((unsigned char)*space))
            space++;

        char op[32];
        if (*space) {
            *space = '\0';
            space++;
            space = trim(space);
        } else {
            space = "";
        }

        strcpy(op, s);
        for (char *p = op; *p; p++) {
            *p = (char)tolower((unsigned char)*p);
        }

        Instruction *ins = &program->instructions[program->instruction_count];
        strcpy(ins->op, op);
        ins->argc = split_args(space, ins->args);
        strcpy(ins->source, original);

        program->instruction_count++;
        if (program->instruction_count >= MAX_INSTRUCTIONS)
            error("too many instructions");
    }

    fclose(fp);
    find_label(program, "main");
}


/* ============================================================
 * Flag Evaluator (CMP instruction)
 * ============================================================ */

static void cmp(CPU *cpu, u64 a, u64 b)
{
    u64 result = a - b;

    cpu->ZF = (result == 0);
    cpu->SF = (result >> 63) & 1;
    cpu->CF = (a < b);

    s64 sa = (s64)a;
    s64 sb = (s64)b;
    s64 sr = (s64)result;

    /* Signed overflow condition checks */
    cpu->OF = ((sa >= 0 && sb < 0 && sr < 0) ||
               (sa < 0 && sb >= 0 && sr >= 0));
}


/* ============================================================
 * Condition Code Dispatcher
 * ============================================================ */

static int condition(CPU *cpu, const char *cond)
{
    if (strcmp(cond, "e") == 0 || strcmp(cond, "z") == 0) return cpu->ZF;
    if (strcmp(cond, "ne") == 0 || strcmp(cond, "nz") == 0) return !cpu->ZF;

    if (strcmp(cond, "l") == 0 || strcmp(cond, "nge") == 0) return cpu->SF != cpu->OF;
    if (strcmp(cond, "le") == 0 || strcmp(cond, "ng") == 0) return cpu->ZF || (cpu->SF != cpu->OF);
    if (strcmp(cond, "g") == 0 || strcmp(cond, "nle") == 0) return !cpu->ZF && (cpu->SF == cpu->OF);
    if (strcmp(cond, "ge") == 0 || strcmp(cond, "nl") == 0) return cpu->SF == cpu->OF;

    if (strcmp(cond, "b") == 0 || strcmp(cond, "c") == 0 || strcmp(cond, "nae") == 0) return cpu->CF;
    if (strcmp(cond, "be") == 0 || strcmp(cond, "na") == 0) return cpu->CF || cpu->ZF;
    if (strcmp(cond, "a") == 0 || strcmp(cond, "nbe") == 0) return !cpu->CF && !cpu->ZF;
    if (strcmp(cond, "ae") == 0 || strcmp(cond, "nb") == 0 || strcmp(cond, "nc") == 0) return !cpu->CF;

    error("unsupported condition code");
    return 0;
}


/* ============================================================
 * Instruction Dispatch Loop (CPU Core emulator)
 * ============================================================ */

static void execute(CPU *cpu, const Program *program)
{
    const Instruction *ins = &program->instructions[cpu->rip];
    const char *op = ins->op;

    if (strcmp(op, "mov") == 0) {
        if (ins->argc != 2) error_line("mov requires 2 operands", ins->source);
        u64 value = read_operand(cpu, ins->args[1]);
        write_operand(cpu, ins->args[0], value);
        cpu->rip++;
        return;
    }

    if (strcmp(op, "push") == 0) {
        if (ins->argc != 1) error_line("push requires 1 operand", ins->source);
        u64 value = read_operand(cpu, ins->args[0]);
        push(cpu, value);
        cpu->rip++;
        return;
    }

    if (strcmp(op, "pop") == 0) {
        if (ins->argc != 1) error_line("pop requires 1 operand", ins->source);
        u64 value = pop(cpu);
        write_operand(cpu, ins->args[0], value);
        cpu->rip++;
        return;
    }

    if (strcmp(op, "add") == 0 || strcmp(op, "sub") == 0 || strcmp(op, "imul") == 0) {
        if (ins->argc != 2) error_line("arithmetic instruction requires 2 operands", ins->source);
        u64 a = read_operand(cpu, ins->args[0]);
        u64 b = read_operand(cpu, ins->args[1]);
        u64 result;

        if (strcmp(op, "add") == 0) result = a + b;
        else if (strcmp(op, "sub") == 0) result = a - b;
        else result = (u64)((s64)a * (s64)b);

        write_operand(cpu, ins->args[0], result);
        cpu->rip++;
        return;
    }

    if (strcmp(op, "cqo") == 0) {
        if (cpu->reg.rax & (1ULL << 63))
            cpu->reg.rdx = UINT64_MAX;
        else
            cpu->reg.rdx = 0;
        cpu->rip++;
        return;
    }

    if (strcmp(op, "idiv") == 0) {
        if (ins->argc != 1) error_line("idiv requires 1 operand", ins->source);
        s64 divisor = (s64)read_operand(cpu, ins->args[0]);
        if (divisor == 0) error_line("division by zero", ins->source);

        s64 dividend = (s64)cpu->reg.rax;
        if (dividend == INT64_MIN && divisor == -1) {
            error_line("idiv quotient overflow", ins->source);
        }

        s64 quotient = dividend / divisor;
        s64 remainder = dividend % divisor;

        cpu->reg.rax = (u64)quotient;
        cpu->reg.rdx = (u64)remainder;
        cpu->rip++;
        return;
    }

    if (strcmp(op, "cmp") == 0) {
        if (ins->argc != 2) error_line("cmp requires 2 operands", ins->source);
        u64 a = read_operand(cpu, ins->args[0]);
        u64 b = read_operand(cpu, ins->args[1]);
        cmp(cpu, a, b);
        cpu->rip++;
        return;
    }

    if (strcmp(op, "movzx") == 0) {
        if (ins->argc != 2) error_line("movzx requires 2 operands", ins->source);
        u64 value = read_operand(cpu, ins->args[1]) & 0xff;
        write_operand(cpu, ins->args[0], value);
        cpu->rip++;
        return;
    }

    if (strncmp(op, "set", 3) == 0) {
        if (ins->argc != 1) error_line("setcc requires 1 operand", ins->source);
        const char *cond = op + 3;
        int result = condition(cpu, cond);
        write_operand(cpu, ins->args[0], result);
        cpu->rip++;
        return;
    }

    if (strcmp(op, "je") == 0) {
        if (ins->argc != 1) error_line("je requires 1 operand", ins->source);
        if (cpu->ZF)
            cpu->rip = find_label(program, ins->args[0]);
        else
            cpu->rip++;
        return;
    }

    if (strcmp(op, "jmp") == 0) {
        if (ins->argc != 1) error_line("jmp requires 1 operand", ins->source);
        cpu->rip = find_label(program, ins->args[0]);
        return;
    }

    if (strcmp(op, "call") == 0) {
        if (ins->argc != 1) error_line("call requires 1 operand", ins->source);
        push(cpu, cpu->rip + 1);
        cpu->call_depth++;
        cpu->rip = find_label(program, ins->args[0]);
        return;
    }

    if (strcmp(op, "ret") == 0) {
        if (cpu->call_depth == 0) {
            cpu->return_value = (s64)cpu->reg.rax;
            cpu->halted = 1;
            return;
        }
        cpu->rip = (size_t)pop(cpu);
        cpu->call_depth--;
        return;
    }

    error_line("unsupported instruction", ins->source);
}


/* ============================================================
 * Interpreter Program Execution
 * ============================================================ */

static s64 run_program(const Program *program)
{
    CPU cpu;
    memset(&cpu, 0, sizeof(cpu));

    cpu.memory = calloc(MEMORY_SIZE, 1);
    if (!cpu.memory)
        error("cannot allocate CPU memory");

    /* Map RSP to the end of the Virtual RAM segment */
    cpu.reg.rsp = MEMORY_SIZE - 16;
    cpu.rip = find_label(program, "main");

    for (uint64_t steps = 0; !cpu.halted; steps++) {
        if (steps >= MAX_STEPS)
            error("program exceeded maximum instruction count (infinite loop?)");

        if (cpu.rip >= program->instruction_count)
            error("instruction pointer out of range");

        execute(&cpu, program);
    }

    s64 result = cpu.return_value;
    free(cpu.memory);
    return result;
}


/* ============================================================
 * Run program (Public Interface)
 * ============================================================ */

int64_t run_interpreter(const char *filename)
{
    Program *program = (Program *)calloc(1, sizeof(Program));
    if (!program) {
        error("cannot allocate Program memory");
    }

    parse_program(program, filename);

    s64 result = run_program(program);

    free(program);
    return (int64_t)result;
}