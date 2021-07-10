.intel_syntax noprefix
.global main
main:
#Prologue
	push rbp
	mov rbp, rsp
	sub rsp, 0



	call foo
	push 0
	pop rax
	mov rsp, rbp
	pop rbp
	ret

