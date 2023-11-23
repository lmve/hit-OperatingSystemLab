!setup's size & position of segment
SETUPLEN=2 
SETUPSEG=0x07e0

entry _start
_start:
!Red cursor position 
!Prepare to output the logo on the screen
!Use BOIS interrupt 10

	mov ah,#0x03
	xor bh,bh
	int 0x10

!Display string "LM system is loading powered by Lucky Ma ..."
!Display string's length
	mov cx,#50
	mov bx,#0x0007
 	mov bp,#msg1

!Display string address 
	mov ax,#0x07c0
	mov es,ax
!Write string and move cursor
	mov ax,#0x1301
	int 0x10

!It's a loop
!inf_loop:
!	jmp inf_loop

!It's time to setup
load_setup:
	mov dx,#0x0000
	mov cx,#0x0002
	mov bx,#0x0200
 	mov ax,#0x0200+SETUPLEN
	int 0x13
	jnc ok_load_setup
	mov dx,#0x0000
	mov ax,#0x0000
	int 0x13
	jmp load_setup
ok_load_setup:
       jmpi    0,SETUPSEG
!End Setup
msg1: 
    .byte     13,10
    .ascii    "LM system is loading powered by Lucky Ma..."
    .byte     13,10,13,10
.org 510
boot_flag:
    .word     0xAA55
