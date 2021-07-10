.intel_syntax noprefix
.global main
func:

#Prologue
	push rbp
	mov rbp, rsp
	sub rsp, 8
#End of Prologue


#Epilogue
	mov rax, rbp
	sub rax, 8
	push rax

	pop rax
	mov rax, [rax]
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
	call func
	pop rax
	mov rsp, rbp
	pop rbp
	ret
#End of Epilogue

