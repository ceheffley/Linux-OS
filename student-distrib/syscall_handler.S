#define ASM 1

.globl syscall_handler_asm

syscall_handler_asm:

	#push saved values
	pushl %ebp
	pushl %esi
	pushl %edi

	#push args
	pushl %edx
	pushl %ecx
	pushl %ebx

	#valid system calls are between 1 and 10
	cmp $10, %eax
	jg invalid_syscall

	cmp $1, %eax
	jl invalid_syscall

	#call correct system call using jumptable below
	movl syscall_jump(, %eax, 4), %eax

	sti
	call *%eax
	jmp complete

invalid_syscall:
	movl $-1, %eax

complete:

	#pop args
	popl %ebx
	popl %ecx
	popl %edx

	#pop saved values
	popl %edi
	popl %esi
	popl %ebp

	iret

syscall_jump:
	.long 0x0, syscall_halt, syscall_execute, syscall_read, syscall_write, syscall_open, syscall_close, syscall_getargs, syscall_vidmap
	#.long syscall_set_handler
	#.long syscall_sigreturn
