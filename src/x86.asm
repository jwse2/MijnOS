[BITS 16]
[ORG 0x7c00]
    jmp bootstrap

;===========
; BOOTSTRAP (448-bytes)
;===========
bootstrap:
    mov     ax,07C0h
    mov     ds,ax
    mov     es,ax
    add     ax,20h
    mov     ss,ax
    mov     sp,200h
    jmp     0:stage2


stage2:
    xor     ax,ax
    mov     ds,ax

    cli

    lgdt    [gdt_descriptor]
    lidt    [idt_descriptor]

    ; A20
    in al,0x92
    or al,2
    out 0x92,al

    ; Protected Mode
    mov     eax,cr0
    or      eax,1
    mov     cr0,eax

    ; Jump to the code to run in Protected Mode
    jmp     GDT_CODE:pmode

 

[BITS 32]
pmode:
    mov     ax,GDT_DATA
    mov     ds,ax
    mov     ss,ax
    mov     es,ax
    mov     fs,ax
    mov     gs,ax
    mov     esp,10000h

    ; Test
    ;   - Should not be allowed without an IDT.
    ;   - Should call our ISR with an IDT.
    mov     ah,0Eh
    mov     al,41h      ; 'A'
    int     10h
    mov     ah,0Eh
    mov     al,42h      ; 'B'
    int     10h

    jmp     $


;===============================================
; GLOBAL DESCRIPTOR TABLE (GDT)
;===============================================
gdt_start:

    gdt_null:
        dd  0           ; null descriptor  
        dd  0

    gdt_code:
        dw  0FFFFh      ; limit_low
        dw  0           ; base_low
        db  0           ; base_middle
        db  10011010b   ; access
        db  11001111b   ; granularity
        db  0           ; base_high

    gdt_data:
        dw  0FFFFh      ; limit_low
        dw  0           ; base_low
        db  0           ; base_middle
        db  10010010b   ; access
        db  11001111b   ; granularity
        db  0           ; base_high

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

GDT_CODE    equ gdt_code - gdt_start
GDT_DATA    equ gdt_data - gdt_start


;===============================================
; INTERRUPT DESCRIPTOR TABLE (IDT)
;===============================================
idt_start:

    ; bit  0..15    Lower address of pointer   
    ; bit 16..31    Selector in GDT
    ; bit 32..39    ZERO
    ; bit 40..43    IDT gates (0b1110 = 32-bit interrupt gate)
    ; bit 44        Set 0 for interrupts and traps
    ; bit 45..46    Privilege level,
    ; bit 47        0 for unused interrupts
    ; bit 48-63     High address of pointer


    idt_int00:
        dw  isr00
        dw  GDT_CODE
        db  0
        db  0b10001110
        dw  0

    idt_int01:
        dw  isr01
        dw  GDT_CODE
        db  0
        db  0b10001110
        dw  0

    idt_int02:
        dw  isr02
        dw  GDT_CODE
        db  0
        db  0b10001110
        dw  0

    idt_int03:
        dw  isr03
        dw  GDT_CODE
        db  0
        db  0b10001110
        dw  0

    idt_int04:
        dw  isr04
        dw  GDT_CODE
        db  0
        db  0b10001110
        dw  0

    idt_int05:
        dw  isr05
        dw  GDT_CODE
        db  0
        db  0b10001110
        dw  0

    idt_int06:
        dw  isr06
        dw  GDT_CODE
        db  0
        db  0b10001110
        dw  0

    idt_int07:
        dw  isr07
        dw  GDT_CODE
        db  0
        db  0b10001110
        dw  0

    idt_int08:
        dw  isr08
        dw  GDT_CODE
        db  0
        db  0b10001110
        dw  0

    idt_int09:
        dw  isr09
        dw  GDT_CODE
        db  0
        db  0b10001110
        dw  0

    idt_int0A:
        dw  isr0A
        dw  GDT_CODE
        db  0
        db  0b10001110
        dw  0

    idt_int0B:
        dw  isr0B
        dw  GDT_CODE
        db  0
        db  0b10001110
        dw  0

    idt_int0C:
        dw  isr0C
        dw  GDT_CODE
        db  0
        db  0b10001110
        dw  0

    idt_int0D:
        dw  isr0D
        dw  GDT_CODE
        db  0
        db  0b10001110
        dw  0

    idt_int0E:
        dw  isr0E
        dw  GDT_CODE
        db  0
        db  0b10001110
        dw  0

    idt_int0F:
        dw  isr0F
        dw  GDT_CODE
        db  0
        db  0b10001110
        dw  0

    idt_int10:
        dw  isr10
        dw  GDT_CODE
        db  0
        db  0b10001110
        dw  0

    idt_int11:
        dw  isr11
        dw  GDT_CODE
        db  0
        db  0b10001110
        dw  0

    idt_int12:
        dw  isr12
        dw  GDT_CODE
        db  0
        db  0b10001110
        dw  0

    idt_int13:
        dw  isr13
        dw  GDT_CODE
        db  0
        db  0b10001110
        dw  0

    idt_int14:
        dw  isr14
        dw  GDT_CODE
        db  0
        db  0b10001110
        dw  0

    idt_int15:
        dw  isr15
        dw  GDT_CODE
        db  0
        db  0b10001110
        dw  0

    idt_int16:
        dw  isr16
        dw  GDT_CODE
        db  0
        db  0b10001110
        dw  0

    idt_int17:
        dw  isr17
        dw  GDT_CODE
        db  0
        db  0b10001110
        dw  0

    idt_int18:
        dw  isr18
        dw  GDT_CODE
        db  0
        db  0b10001110
        dw  0

    idt_int19:
        dw  isr19
        dw  GDT_CODE
        db  0
        db  0b10001110
        dw  0

    idt_int1A:
        ;dw  isr1A
        dw  isr00
        dw  GDT_CODE
        db  0
        db  0b10001110
        dw  0

    idt_int1B:
        ;dw  isr1B
        dw  isr00
        dw  GDT_CODE
        db  0
        db  0b10001110
        dw  0

    idt_int1C:
        ;dw  isr1C
        dw  isr00
        dw  GDT_CODE
        db  0
        db  0b10001110
        dw  0

    idt_int1D:
        ;dw  isr1D
        dw  isr00
        dw  GDT_CODE
        db  0
        db  0b10001110
        dw  0

    idt_int1E:
        ;dw  isr1E
        dw  isr00
        dw  GDT_CODE
        db  0
        db  0b10001110
        dw  0

    idt_int1F:
        ;dw  isr1F
        dw  isr00
        dw  GDT_CODE
        db  0
        db  0b10001110
        dw  0

;    idt_int20:
;        dw  isr20
;        dw  GDT_CODE
;        db  0
;        db  0b10001110
;        dw  0

idt_end:

idt_descriptor:
    dw idt_end - idt_start - 1
    dd idt_start


;===============================================
; INTERRUPT SERVICE ROUTINES (ISR)
;===============================================
isr00:
    pusha
    ; interrupt handler for idt_int00
    popa
    iret

isr01:
    pusha
    ; interrupt handler for idt_int01
    popa
    iret

isr02:
    pusha
    ; interrupt handler for idt_int02
    popa
    iret

isr03:
    pusha
    ; interrupt handler for idt_int03
    popa
    iret

isr04:
    pusha
    ; interrupt handler for idt_int04
    popa
    iret

isr05:
    pusha
    ; interrupt handler for idt_int05
    popa
    iret

isr06:
    pusha
    ; interrupt handler for idt_int06
    popa
    iret

isr07:
    pusha
    ; interrupt handler for idt_int07
    popa
    iret

isr08:
    pusha
    ; interrupt handler for idt_int08
    popa
    iret

isr09:
    pusha
    ; interrupt handler for idt_int09
    popa
    iret

isr0A:
    pusha
    ; interrupt handler for idt_int0A
    popa
    iret

isr0B:
    pusha
    ; interrupt handler for idt_int0B
    popa
    iret

isr0C:
    pusha
    ; interrupt handler for idt_int0C
    popa
    iret

isr0D:
    pusha
    ; interrupt handler for idt_int0D
    popa
    iret

isr0E:
    pusha
    ; interrupt handler for idt_int0E
    popa
    iret

isr0F:
    pusha
    ; interrupt handler for idt_int0F
    popa
    iret

isr10:
    pusha

    xor     ecx,ecx

    ; Get the lower byte offset
    mov     dx,03D4h                            ; 03D4h = Set / 03D5h = Get
    mov     al,0Fh      ; Cursor offset
    out     dx,al
    mov     dx,03D5h    ; I/O port
    in      al,dx

    ; Get the higher byte offset
    mov     dx,03D4h
    mov     al,0Eh      ; Cursor offset
    out     dx,al
    mov     dx,03D5h
    in      al,dx
;
;    ; Get the address
;    ;shl     ax,2
;    ;movzx   eax,ax
;    imul    ax,2


    ; interrupt handler for idt_int10
    ;mov     ah,0Eh
    ;mov     al,41h      ; 'A'
    ;int     10h
    ;mov     ebx,0B8000h ; Video memory address
;    mov     ebx,0B8002h ; Video memory address
;    mov     al,cl
;    mov     ah,0Fh      ; The color
;    mov     word [ebx],ax

    xor     eax,eax

    mov     ebx,0B80A0h
    mov     dword [ebx],07690748h               ; 25 rows / 80 columns

    popa
    iret

isr11:
    pusha
    ; interrupt handler for idt_int11
    popa
    iret

isr12:
    pusha
    ; interrupt handler for idt_int12
    popa
    iret

isr13:
    pusha
    ; interrupt handler for idt_int13
    popa
    iret

isr14:
    pusha
    ; interrupt handler for idt_int14
    popa
    iret

isr15:
    pusha
    ; interrupt handler for idt_int15
    popa
    iret

isr16:
    pusha
    ; interrupt handler for idt_int16
    popa
    iret

isr17:
    pusha
    ; interrupt handler for idt_int17
    popa
    iret

isr18:
    pusha
    ; interrupt handler for idt_int18
    popa
    iret

isr19:
    pusha
    ; interrupt handler for idt_int19
    popa
    iret

;isr1A:
;    pusha
;    ; interrupt handler for idt_int1A
;    popa
;    iret

;isr1B:
;    pusha
;    ; interrupt handler for idt_int1B
;    popa
;    iret

;isr1C:
;    pusha
;    ; interrupt handler for idt_int1C
;    popa
;    iret

;isr1D:
;    pusha
;    ; interrupt handler for idt_int1D
;    popa
;    iret

;isr1E:
;    pusha
;    ; interrupt handler for idt_int1E
;    popa
;    iret

;isr1F:
;    pusha
;    ; interrupt handler for idt_int1F
;    popa
;    iret

;isr20:
;    pusha
;    ; interrupt handler for idt_int20
;    popa
;    iret

;===========
; BOOT SIG (2-bytes)
;===========
times 510-($-$$) db 0
dw 0AA55h
