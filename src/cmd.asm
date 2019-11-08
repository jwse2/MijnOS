[BITS 16]
; [ORG SEG_CMD]
jmp main


%include "src\const.inc"
%define BUFFER_SIZE     64
msg_cmd     db 'MijnOS CMD 2018-05-21', 0Dh, 0Ah, 0
cmd_buffer  times BUFFER_SIZE db 0              ; Buffer the user writes to when pressing a key
cmd_fpath   times 12 db 020h                    ; Buffer containing the file name to load
cmd_ferr    dw 0FFh                             ; Load error code
cmd_offset  dw 0                                ; Current offset in the buffer
cmd_prefix  db '> ', 0                          ; CMD commando prefix
; name (8) + seperator (1) + extension (3) + terminator (1) = 13

back_buffer db 08h, 20h, 08h, 0                 ; Instructions for backspace
err_proc_s  db 'Could not start program ', 27h, 0
err_proc_e  db 27h, 0

cmd_regs    times 2 dw 0                        ; Used to store CMD registers during program execution



;===============================================
; Entry point
;===============================================
main:
    mov     ax,SEG_CMD
    mov     ds,ax
    mov     es,ax
    add     ax,100h     ; 4kb
    mov     ss,ax
    mov     sp,1000h    ; 4kb

.init:
    mov     ax,INT_CLEAR_SCREEN
    int     70h
    mov     si,msg_cmd
    mov     ax,INT_PRINT_STRING
    int     70h
    call    print_prefix

.keypress:
    mov     ax,INT_KEYPRESS
    int     70h
    cmp     ax,KEY_BACKSPACE
    ;je      .clear
    je      .backspace
    cmp     ax,KEY_ENTER
    je      .exec

.cmp0:                                          ; 0-9
    cmp     ax,KEY_0
    jb      .cmp1
    cmp     ax,KEY_9
    jbe     .simpleChar

.cmp1:                                          ; A-Z
    cmp     ax,KEY_UC_A
    jb      .cmp2
    cmp     ax,KEY_UC_Z
    jbe     .simpleChar

.cmp2:                                          ; a-z
    cmp     ax,KEY_LC_A
    jb      .cmp3
    cmp     ax,KEY_LC_Z
    ja      .cmp3
    sub     ax,20h                              ; toUpper
    jmp     .simpleChar

.cmp3:
    cmp     ax,KEY_SPACE
    je      .simpleChar
    cmp     ax,KEY_PERIOD
    je      .simpleChar

.continue:
    jmp     .keypress

.simpleChar:
    mov     dx,word [cmd_offset]
    cmp     dx,BUFFER_SIZE-1
    jae     .keypress

    ; Store the character in the buffer
    mov     bx,cmd_buffer
    add     bx,word [cmd_offset]
    mov     byte [ds:bx],al

    ; Increment the offset
    add     word [cmd_offset],1

    ; Print the character to the screen
    mov     cx,ax
    mov     ax,INT_PRINT_CHAR
    int     70h
    jmp     .keypress

.clear:
    mov     ax,INT_CLEAR_SCREEN
    int     70h
    call    print_prefix
    jmp     .keypress

; == SPECIAL KEYS ==============================
.backspace:
    mov     dx,word [cmd_offset]
    cmp     dx,0
    jle     .keypress

    ; Clear the last byte in the buffer
    push    bx                  
    mov     bx,cmd_buffer
    add     bx,dx
    mov     byte [bx],0
    pop     bx

    ; Decrement the offset
    sub     word [cmd_offset],1

    ; Replaces the last character on the screen
    ; with a space and decrements the pointer
    mov     si,back_buffer
    mov     ax,INT_PRINT_STRING
    int     70h
    jmp     .keypress

.exec:
    ; Put a zero terminator
    mov     dx,word [cmd_offset]
    push    bx
    mov     bx,cmd_buffer
    add     bx,dx
    mov     byte [bx],0
    pop     bx

    ; Translate to a loadable filename
    push    si
    push    di

    mov     si,cmd_buffer
    mov     di,cmd_fpath
    call    cmd_convertString

    pop     di
    pop     si

    ; Load and execute the file...
    call    start_program

    ; Clear of old data
    mov     byte [cmd_buffer],0
    mov     word [cmd_offset],0

    ; Program returned...
    mov     ax,INT_PRINT_NEWLINE
    int     70h
    call    print_prefix
    jmp     .keypress

    jmp     $



;===============================================
; Prints the CMD prefix to the screen.
;===============================================
print_prefix:
    push    si
    push    ax
    mov     si,cmd_prefix
    mov     ax,INT_PRINT_STRING
    int     70h
    pop     ax
    pop     si
    ret



;===============================================
; Called to start the actual program.
;===============================================
start_program:
    push    bp
    mov     bp,sp
    sub     sp,2
    pusha
    push    ds
    push    es

    ;mov     ds,ds
    mov     si,cmd_fpath

    mov     bx,SEG_PROGRAM                      ; Program segment
    mov     es,bx
    xor     di,di

    mov     cx,cmd_ferr
    mov     ax,INT_LOAD_FILE
    int     70h
    mov     ax,word [cmd_ferr]

    test    ax,ax
    jne     .error

.success:
    push    ax
    mov     ax,INT_PRINT_NEWLINE
    int     70h
    pop     ax

    call    cmd_callProgram
    jmp     .return

.error:
    ;mov     cx,ax
    ;mov     ax,INT_PRINT_HEX
    ;int     70h

    push    si

    mov     ax,INT_PRINT_NEWLINE
    int     70h

    mov     si,err_proc_s
    mov     ax,INT_PRINT_STRING
    int     70h

    mov     si,cmd_fpath
    mov     ax,INT_PRINTN_STRING
    mov     cx,11
    int     70h

    mov     si,err_proc_e
    mov     ax,INT_PRINT_STRING
    int     70h

    pop     si

.return:
    pop     es
    pop     ds
    popa
    mov     sp,bp
    pop     bp
    ret



;===============================================
; Called to start the actual program.
;===============================================
cmd_callProgram:

    ; 1) Store the registers
    pusha               ; ax, bx, cx, dx, sp, bp, si, di
    push    es
    push    ds
    mov     word [ds:cmd_regs],sp

    ; 2) Change the segment and stack to that of the calling program
    mov     ax,SEG_PROGRAM
    mov     ds,ax
    mov     es,ax
    sub     ax,100h     ; 4kb stack BEFORE the program
    mov     ss,ax
    mov     sp,1000h    ; 4kb

    ; 3) Launch the actual program
    call    SEG_PROGRAM:0

    ; 4) Restore the pointers
    mov     ax,SEG_CMD
    mov     ds,ax
    mov     es,ax
    add     ax,100h     ; 4kb
    mov     ss,ax

    mov     sp,word [ds:cmd_regs]
    pop     ds
    pop     es
    popa

    ; 5) Restore GUP to text mode
    push    ax
    mov     ax,INT_GPU_TEXT
    int     70h
    pop     ax

    ret

%include "src/fatname.inc"
