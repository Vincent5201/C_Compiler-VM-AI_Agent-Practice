# Mini C Compiler
# x86-64 Assembly
# Generated automatically

.intel_syntax noprefix


.globl add
add:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    mov QWORD PTR [rbp-8], rdi
    mov QWORD PTR [rbp-16], rsi
    ; int c at [rbp-24]
    mov rax, QWORD PTR [rbp-8]
    push rax
    mov rax, QWORD PTR [rbp-16]
    mov rcx, rax
    pop rax
    add rax, rcx
    mov QWORD PTR [rbp-24], rax
    mov rax, QWORD PTR [rbp-24]
    push rax
    mov rax, 5
    mov rcx, rax
    pop rax
    cmp rax, rcx
    setg al
    movzx rax, al
    cmp rax, 0
    je .L0
    mov rax, 0
    mov rsp, rbp
    pop rbp
    ret
    jmp .L1
.L0:
.L1:
    mov rax, QWORD PTR [rbp-24]
    mov rsp, rbp
    pop rbp
    ret
    mov rax, 0
    mov rsp, rbp
    pop rbp
    ret

.globl main
main:
    push rbp
    mov rbp, rsp
    sub rsp, 16
    ; int x at [rbp-8]
    mov rax, 10
    mov rdi, rax
    mov rax, 20
    mov rsi, rax
    call add
    mov QWORD PTR [rbp-8], rax
    mov rax, QWORD PTR [rbp-8]
    mov rsp, rbp
    pop rbp
    ret
    mov rax, 0
    mov rsp, rbp
    pop rbp
    ret
