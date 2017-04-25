.intel_syntax noprefix
.global show_eax, clearScr, writeScr, kbd_enter, lidtr, outportb, go, schedule, init_timer_dev, pushf_cli_fun, popf_fun, sched_fun
.extern convert_num, kbd_handler, dequeue, enqueue, currPCB

clr_scr: .ascii " \0"
eax_msg: .ascii "The value in the EAX register is: "
eax_val: .asciz "          " /*ten spaces, the max number of digits in an unsigned int*/

//Program 2 data vars
got_key: .ascii "Got a Key: "
scan_code: .asciz "          " /*ten spaces, the max number of digits in an unsigned int*/

show_eax:
	push ebp		//Set up ebp
	mov ebp, esp
	pushfd

	call clearScr		//Clear the screen of all text 24x79
	push OFFSET eax_val	//Push offset of eax_val onto stack
	push eax		//Push val of eax onto stack
	call convert_num	//Call the c convert_num function
	add esp, 8		//Clean up stack

	push DWORD PTR 20	//COLUMN
	push DWORD PTR 12	//ROW
	push OFFSET eax_msg
	call writeScr		//Print eax_msg
	add esp, 12		//Clean up stack

	popfd
	pop ebp
	ret

clearScr:
	push ebp
	mov ebp, esp
	pushfd
	push eax
	push ebx
	mov eax, 0

startLoop:
	mov ebx, 0

nestedLoop:
	push ebx
	push eax
	push OFFSET clr_scr
	call writeScr
	add esp, 12

	inc ebx
	cmp ebx, 80
	jl nestedLoop
	inc eax
	cmp eax, 25
	jl startLoop

	pop ebx
	pop eax
	popfd
	pop ebp
	ret

writeScr:
	push ebp
	mov ebp, esp
	pushfd
	push eax
	push ebx
	push esi
	push edi

	mov ebx, [ebp + 16]	//Move column into ebx
	mov eax, [ebp + 12]	//Move row into eax
	mov esi, [ebp + 8]	//Move address of string into esi

	//Calculate offset using equation
	//offset = 0xB8000 + 2(row*80 + column)

	imul eax, 80		//row*80
	add eax, ebx		//row*80 + column
	imul eax, 2		//2(row*80 + column)
	add eax, 0xB8000	//0xB8000 + 2(row*80 + column)
	mov edi, eax		//Move offset into edi

loop:
	cmpb [esi], 0
	je done
	movsb //es:[edi], ds:[esi]
	movb es:[edi], 10
	inc edi
	jmp loop

done:
	pop edi
	pop esi
	pop ebx
	pop eax
	popfd
	pop ebp
	ret

kbd_enter:
	pushfd
	cli		//Clear interrupts
	pusha

	in al, 0x64	//see if the keyboard has something available
	and al, 0x01
	jz _kbd_skip	//if not, skip to the end
	mov eax, 0
	in al, 0x60	//read the keyboard scan code

	push eax
	call kbd_handler
	add esp, 4

	//Convert scan_code to number
	/*push OFFSET scan_code 	//Push offset of scanb_code onto stack
	push eax			//Push val of al onto stack
	call convert_num	//Call the c convert_num function
	add esp, 8		//Clean up stack*/

	//print "Got a Key: "
	/*push DWORD PTR 0	//COLUMN
	push DWORD PTR 0	//ROW
	push OFFSET got_key
	call writeScr		//Print message
	add esp, 12		//Clean up stack*/

_kbd_skip:
	mov al, 0x20
	out 0x20, al	//send EOI
	popa
	popfd
	sti
	iret

lidtr:
	lidt [esp+4]
	ret

outportb:
	push ebp
	mov ebp, esp
	push eax
	push edx

	mov eax, [ebp+12] 	//data
	mov edx, [ebp+8]	//port
	out dx, al

	pop edx
	pop eax
	pop ebp
	ret

schedule:
	pushad
	push ds
	push es
	push fs
	push gs

	//save esp register in the current processes pcb
	mov eax, currPCB
	mov [eax], esp

	//enqueue pcb for the current process
	push eax
	call enqueue
	pop eax

	//dequeue pcb for the next process and make it current
	call dequeue
	
	//set the esp register to esp value in the new current processes pcb
	mov currPCB, eax
	mov esp, [eax]

	pop gs
	pop fs
	pop es
	pop ds
	popad
	push eax

	//SEND EOI TO PIC
	mov al, 0x20
	out 0x20, al
	pop eax
	iret

init_timer_dev:
	push ebp
	mov ebp, esp
	pushfd
	pushad
	
	mov edx, [ebp+8]
	mov ax, 1193
	imul dx, ax

	mov al, 0b00110110 // 0x43 is the Write control word
	out 0x43, al
	// 5) Load the LSB first then the MSB.
	// 0x40 = Load counter 0 with the following code: 
	mov ax, dx
	out 0x40, al //LSB
	xchg ah, al
	out 0x40, al //MSB

	popad
	popfd
	pop ebp
	ret

sched_fun:
	pushad
	push ds
	push es
	push fs
	push gs

	mov eax, currPCB
	push eax
	call vterm_enqueue
	pop eax

	call dequeue
	mov currPCB, eax
	mov esp, [eax]
	pop gs
	pop fs
	pop es
	pop ds
	popad
	ret

go:
	call dequeue
	mov currPCB, eax
	mov esp, [eax]
	pop gs
	pop fs
	pop es
	pop ds
	popad
	iret

pushf_cli_fun:
	pushf
  	cli
	pop eax
	ret

popf_fun:
	sti
	push ebp
	mov ebp, esp
	push [ebp+8]
	popf
	pop ebp
	ret
