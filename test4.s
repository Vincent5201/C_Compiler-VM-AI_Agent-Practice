# Mini C Compiler
# x86-64 Assembly
# Generated automatically

.intel_syntax noprefix


.globl loop
loop:
    push rbp
    mov rbp, rsp
    sub rsp, 16
    mov QWORD PTR [rbp-8], rdi
    ; int c at [rbp-16]
    mov rax, QWORD PTR [rbp-8]
    mov QWORD PTR [rbp-16], rax
.L0:
    mov rax, QWORD PTR [rbp-16]
    push rax
    mov rax, 0
    mov rcx, rax
    pop rax
    cmp rax, rcx
    setg al
    movzx rax, al
    cmp rax, 0
    je .L1
    mov rax, QWORD PTR [rbp-8]
    push rax
    mov rax, QWORD PTR [rbp-16]
    mov rcx, rax
    pop rax
    add rax, rcx
    mov QWORD PTR [rbp-8], rax
    mov rax, QWORD PTR [rbp-16]
    push rax
    mov rax, 1
    mov rcx, rax
    pop rax
    sub rax, rcx
    mov QWORD PTR [rbp-16], rax
    jmp .L0
.L1:
    mov rax, QWORD PTR [rbp-8]
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
    mov QWORD PTR [rbp-8], rax
    mov rax, QWORD PTR [rbp-8]
    mov rdi, rax
    call loop
    mov QWORD PTR [rbp-8], rax
    mov rax, QWORD PTR [rbp-8]
    mov rsp, rbp
    pop rbp
    ret
    mov rax, 0
    mov rsp, rbp
    pop rbp
    ret
