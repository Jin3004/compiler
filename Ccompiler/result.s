.intel_syntax noprefix
.global main


func:

#Prologue
	push rbp
	mov rbp, rsp
	push rdi
	push rsi
	sub rsp, 0
#End of Prologue

push rdi
push rsi

#Epilogue
	mov rax, rbp
	sub rax, 8
	push rax

	pop rax
	mov rax, [rax]
	push rax

	mov rax, rbp
	sub rax, 16
	push rax

	pop rax
	mov rax, [rax]
	push rax

	pop rdi
	pop rax
	add rax, rdi
	push rax
	pop rax
	mov rsp, rbp
	pop rbp
	ret
#End of Epilogue

main:

#Prologue
	push rbp
	mov rbp, rsp
	sub rsp, 0
#End of Prologue


#Epilogue
	push 5
	pop rax
	mov rdi, rax
	push 7
	pop rax
	mov rsi, rax
	call func
	mov rsp, rbp
	pop rbp
	ret
#End of Epilogue

