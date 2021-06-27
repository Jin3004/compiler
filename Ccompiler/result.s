.intel_syntax noprefix
.global main
main:
#Prologue
	push rbp
	mov rbp, rsp
	sub rsp, 8



	mov rax, rbp
	sub rax, 8
	push rax

	push 3
	pop rdi
	pop rax
	mov [rax], rdi
	push rdi

	mov rax, rbp
	sub rax, 8
	push rax

	pop rax
	mov rax, [rax]
	push rax

	push 2
	pop rdi
	pop rax
	cmp rax, rdi
	sete al
	movzb rax, al
	push rax

	pop rax
	cmp rax, 0
	je .L1
	push 0
	pop rax
	mov rsp, rbp
	pop rbp
	ret

	jmp .L2
.L1:
	push 1
	pop rax
	mov rsp, rbp
	pop rbp
	ret

.L2:
