[BITS 16]

init_gdt:
    cli
    pusha
    lgdt    [gdt_descriptor]
    sti
    popa
    ret


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
