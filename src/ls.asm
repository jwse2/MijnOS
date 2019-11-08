[BITS 16]
; [ORG SEG_PROGRAM]
jmp main

%include "src\const.inc"
%define BUFFER_SIZE     64
msg_test    db 'ls', 0Dh, 0Ah, 0



;===============================================
; Entry point
;===============================================
main:
    push    si
    push    ax

    mov     si,msg_test
    mov     ax,INT_PRINT_STRING
    int     70h

    pop     ax
    pop     si
    retf
