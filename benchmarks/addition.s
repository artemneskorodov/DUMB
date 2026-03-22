global _start:
section .text
extern __std_input
extern __std_output
_start:
mov rbp, rsp
sub rbp, 8
add rsp, 0
.LOC_0:
call main
mov rax, 60
xor rdi, rdi
syscall
main:
mov rbp, rsp
sub rbp, 8
add rsp, 32
.LOC_1:
mov rax, 0
mov [rbp - 8], rax
mov rax, 0
mov [rbp - 16], rax
mov rsi, STR_CONST_0
mov rdx, 8
push rbp
call __std_input
pop rbp
mov [rbp - 8], rax
mov rsi, STR_CONST_1
mov rdx, 9
push rbp
call __std_input
pop rbp
mov [rbp - 16], rax
mov rax, [rbp - 8]
mov rbx, [rbp - 16]
add rax, rbx
mov [rbp - 24], rax
mov rax, [rbp - 24]
mov [rbp - 32], rax
mov rsi, STR_CONST_2
mov rdx, 17
mov rcx, [rbp - 32]
push rbp
call __std_output
pop rbp
mov rax, 0
sub rsp, 32
ret
section .data
STR_CONST_0 db "First = "
STR_CONST_1 db "Second = "
STR_CONST_2 db "First + Second = "
