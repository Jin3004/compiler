.intel_syntax noprefix
.global main
main:
#Prologue
	push rbp
	mov rbp, rsp
	sub rsp, 208




	mov rax, rbp
	sub rax, 8
	push rax

	push 12
	pop rdi
	pop rax
	mov [rax], rdi
	push rdi

	pop rax

	mov rax, rbp
	sub rax, 16
	push rax

	push 24
	pop rdi
	pop rax
	mov [rax], rdi
	push rdi

	pop rax

	mov rsp, rbp
	pop rbp
	ret
