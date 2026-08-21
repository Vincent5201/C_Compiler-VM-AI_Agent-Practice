# Mini C Compiler
# x86-64 Assembly
# Generated automatically

.intel_syntax noprefix


.globl factorial
factorial:
    push rbp
    mov rbp, rsp
    sub rsp, 16
    mov QWORD PTR [rbp-8], rdi
    mov rax, QWORD PTR [rbp-8]
    push rax
    mov rax, 1
    mov rcx, rax
    pop rax
    cmp rax, rcx
    setle al
    movzx rax, al
    cmp rax, 0
    je .L0
    mov rax, 1
    mov rsp, rbp
    pop rbp
    ret
    jmp .L1
.L0:
.L1:
    mov rax, QWORD PTR [rbp-8]
    push rax
    mov rax, QWORD PTR [rbp-8]
    push rax
    mov rax, 1
    mov rcx, rax
    pop rax
    sub rax, rcx
    mov rdi, rax
    call factorial
    mov rcx, rax
    pop rax
    imul rax, rcx
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
    ; int result at [rbp-8]
    ; int n at [rbp-16]
    mov rax, 5
    mov QWORD PTR [rbp-16], rax
    mov rax, QWORD PTR [rbp-16]
    mov rdi, rax
    call factorial
    mov QWORD PTR [rbp-8], rax
    mov rax, QWORD PTR [rbp-8]
    mov rsp, rbp
    pop rbp
    ret
    mov rax, 0
    mov rsp, rbp
    pop rbp
    ret
