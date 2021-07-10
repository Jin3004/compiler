.intel_syntax noprefix
.global main
main:
#Prologue
	push rbp
	mov rbp, rsp
	sub rsp, 0



	push 1
	pop rax
	mov rdi ,rax
	push 2
	pop rax
	mov rsi ,rax
	call foo
	push 0
	pop rax
	mov rsp, rbp
	pop rbp
	ret

