entry _start
_start:
	mov ah,#0x03
	xor bh,bh
	int 0x10
	
	mov cx,#25
	mov bx,#0x0007
	mov bp,#msg2
	mov ax,cs
	mov es,cs
	mov ax,#0x1301
	int 0x10
inf_loop:
	jmp inf_loop
msg2:
     .byte    13,10
     .ascii   "Now we are in SETUP"
     .byte    13,10,13,10
.org 510
boot_flag:
     .word 0xAA55
