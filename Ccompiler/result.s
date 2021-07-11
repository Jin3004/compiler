.intel_syntax noprefix
.global main


main:

#Prologue
	push rbp
	mov rbp, rsp
	sub rsp, 16
#End of Prologue

	mov rax, rbp
	sub rax, 8
	push rax

	push 3
	pop rdi
	pop rax
	mov [rax], rdi
	push rdi

	mov rax, rbp
	sub rax, 16
	push rax

	push 5
	pop rdi
	pop rax
	mov [rax], rdi
	push rdi


#Epilogue
	mov rax, rbp
	sub rax, 16
	push rax

	push 8
	pop rdi
	pop rax
	add rax, rdi
	push rax
	pop rax
	mov rax, [rax]
	push rax
	pop rax
	mov rsp, rbp
	pop rbp
	ret
#End of Epilogue

