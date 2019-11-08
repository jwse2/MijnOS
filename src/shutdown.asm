[BITS 16]
; [ORG 0x15E00]
jmp main

%include "src\const.inc"
; Based on the example at https://wiki.osdev.org/APM


;===============================================
; Entry point
;===============================================
main:
    pusha

.stage1:                ; APM installation check
    mov     ah,53h
    mov     al,00h
    xor     bx,bx
    int     15h
    jc      .return

.stage2:                ; APM connect
    mov     ah,53h
    mov     al,01h      ; Real Mode
    xor     bx,bx
    int     15h
    jc      .return

.stage3:                ; Enable power management
    mov     ah,53h
    mov     al,08h
    mov     bx,0001h
    mov     cx,0001h
    int     15h
    jc      .return

.stage4:                ; Shutdown
    mov     ah,53h
    mov     al,07h
    mov     bx,0001h
    mov     cx,03h
    int     15h
    jc      .return

.stage5:                ; APM disconnect
    mov     ah,53h
    mov     al,04h
    xor     bx,bx
    int     15h
    ;jc      .return

.return:
    popa
    retf
