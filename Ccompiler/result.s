.intel_syntax noprefix
.global main
main:
	push 20
	push 1
	push 3
	push 6
	pop rdi
	pop rax
	imul rax, rdi

	push rax

	pop rdi
	pop rax
	add rax, rdi

	push rax

	push 1
	pop rdi
	pop rax
	add rax, rdi

	push rax

	pop rdi
	pop rax
	cmp rax, rdi
	sete al
	movzb rax, al

	push rax

	pop rax
	ret
