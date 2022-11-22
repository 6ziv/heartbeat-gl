ORG 0C0h

SECTION .TEXT
STUB_startheader:
db 4Dh,5Ah		; EXE file signature
dw 90h,03h,0,04h,0,0FFFFh,0,0b8h,0,0,0,40h,0
dw 0,0,0,0
dw 0,0
dw 0,0,0,0,0,0,0,0,0,0
dd STUB_size
align 16,db 0  ;should have no effect
STUB_endheader:
; Define variables in the data section

SECTION .DATA
STUB_startdata:
	hello:     db '%PDF-1.5',10,'255 0 obj',10,'<<',10,'/Filter /FlateDecode',10,'/Length ExeLenByte',10,'>>',10,'stream',10
	helloLen:  equ $-hello
	
	align 16,db 0
STUB_enddata:

; Code goes in the text section
SECTION .TEXT
	GLOBAL _start 
STUB_startcode:
_start:
	; Terminate program
	mov eax,1            ; 'exit' system call
	mov ebx,0            ; exit with error code 0
	int 80h              ; call the kernel

align 16,db 0
STUB_endcode:
EXE_endbss:
STUB_size equ (STUB_endcode-STUB_startcode + STUB_enddata - STUB_startdata + STUB_endheader - STUB_startheader)