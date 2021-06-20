.intel_syntax noprefix
.global main
main:
#Prologue
	push rbp
	mov rbp, rsp
	sub rsp, 16



	mov rax, rbp
	sub rax, 8
	push rax

	push 3
	pop rdi
	pop rax
	mov [rax], rdi
	push rdi

	pop rax

	mov rax, rbp
	sub rax, 16
	push rax

	mov rax, rbp
	sub rax, 8
	push rax

	pop rax
	mov rax, [rax]
	push rax

	push 4
	pop rdi
	pop rax
	imul rax, rdi

	push rax

	pop rdi
	pop rax
	mov [rax], rdi
	push rdi

	pop rax

	mov rax, rbp
	sub rax, 16
	push rax

	pop rax
	mov rax, [rax]
	push rax

	pop rax
	mov rsp, rbp
	pop rbp
	ret

	pop rax

	mov rsp, rbp
	pop rbp
	ret
