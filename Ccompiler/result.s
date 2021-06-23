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

	push 2
	pop rdi
	pop rax
	mov [rax], rdi
	push rdi

	pop rax

	mov rax, rbp
	sub rax, 8
	push rax

	pop rax
	mov rax, [rax]
	push rax

	push 3
	pop rdi
	pop rax
	cmp rax, rdi
	sete al
	movzb rax, al

	push rax

	pop rax
	cmp rax, 0
	je .L1
	push 3
	jmp .L2
.L1:
	push 4
.L2:
	pop rax

#Epilogue
	mov rsp, rbp
	pop rbp
	ret
