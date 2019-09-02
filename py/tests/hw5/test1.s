some_unreachable_pad:
    nop
    nop
    nop

#
# rax -> rax
#  rbx = t1
#  rcx = t2
#  rdx = temp
#  rdi = i
#
.globl fibonacci
.type fibonacci, @function
fibonacci:
    movq g_1(%rip), %rdi
    mov %rdi, %rbx
    mov g_0(%rip), %rbx # t1
    mov g_1(%rip), %rcx # t2

.fi_loop:
    cmp %rax, %rdi
    je .fi_out

    add $1, %rdi # i++

    mov %rbx, %rdx
    add %rcx, %rdx # rdx = t1 + t2
    mov %rcx, %rbx # t1 = t2
    mov %rdx, %rcx # t2 = rdx

    jmp .fi_loop
.fi_out:
    mov %rbx, %rax
    ret

.globl _start
_start:
    pushq %rbp
    mov g_reps(%rip), %rax
    callq fibonacci
    mov g_reps(%rip), %rax
    callq fibonacci_recursive
    popq %rbp

#
# rax -> rax
#
.globl fibonacci_recursive
.type fibonacci_recursive, @function
fibonacci_recursive:
	push %rbx # preserve rbx

	cmpq %rax, g_1(%rip)
	je .fi2_ret

	mov %rax, %rbx # store n

    mov g_neg1(%rip), %rsi
	add %rsi, %rax
	call fibonacci_recursive

	mov %rbx, %rcx # pop n
	mov %rax, %rbx # store L result
	mov %rcx, %rax

	mov g_neg1(%rip), %rsi
	add %rsi, %rax
    add %rsi, %rax
	call fibonacci_recursive

	add %rbx, %rax

.fi2_ret:
	pop %rbx
	ret



.section .data
.globl g_reps
.globl g_0
.globl g_1
.globl g_2
.globl g_neg1
g_reps:
	.long 0xA
g_0:
    .long 0x0
g_1:
    .long 0x1
g_2:
    .long 0x2
g_neg1:
    .long -1