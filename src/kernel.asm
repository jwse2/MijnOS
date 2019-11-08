[BITS 16]
; [ORG SEG_KERNEL]
jmp near kernel

%include "src/const.inc"

msg_success db "Kernel has been loaded...", 0Dh, 0Ah, 0
cmd_bin     db 'CMD     BIN', 0
cmd_error   db 'Could not load CMD.bin', 0Dh, 0Ah, 0

kernel_var  dw 512

test_err    db 'Test failed', 0Dh, 0Ah, 0
test_var    times 32 db 0

test_name   db 'ABCDEFGHEXT',0



;===============================================
; Entry point of the kernel module.
;===============================================
kernel:
    mov     ax,cs
    mov     ds,ax
    mov     es,ax
    mov     gs,ax       ; NOTE: This is used for interrupt calls
    add     ax,400h     ; 16kb
    mov     ss,ax
    mov     sp,4000h    ; 16kb

    ; Indicator that we lsuccesfully loaded the kernel
    mov     si,msg_success
    call    print

%ifndef TESTING

; Regular operations
.clear:
    call    register_interrupts
    call    exec_cmd

%else

; Catches and displays the pressed key(s)
.keypress:
    mov     ah,00h
    int     16h
    movzx   ax,al
    call    print_hex
    call    print_newline
    jmp     .keypress

%endif

    jmp     $



;===============================================
; [Internal]
;   Starts the execution of the CMD program.
;===============================================
exec_cmd:
    push    ds
    push    es
    push    si
    push    di
    push    bx

    ;mov     ds,ds
    mov     si,cmd_bin

    mov     bx,SEG_CMD
    mov     es,bx
    xor     di,di

    call    fat_loadFile
    test    ax,ax
    jne     .error

.success:
    call    SEG_CMD:0
    jmp     .return

.error:
    mov     si,cmd_error
    call    print

.return:
    pop     bx
    pop     di
    pop     si
    pop     es
    pop     ds
    ret



;===============================================
; [Internal]
;   Registers the interrupts.
;===============================================
register_interrupts:
    push    si

    mov     si,kernel_interrupts
    mov     ax,70h
    call    set_interrupt

    pop     si
    ret

set_interrupt:
    push    es
    push    bx

    mov     bx,0        ; Segment ZERO
    mov     es,bx

    mov     bx,ax
    shl     bx,2

    mov     word [es:bx],si         ; Function
    mov     word [es:bx+2],cs       ; Segment

    pop     bx
    pop     es
    ret


;===============================================
; [External]
;   Set ax and than call int 70h. The input and
;   output differs per request made.
;===============================================
kernel_interrupts:
    push    gs
    push    bx
    mov     bx,SEG_KERNEL                       ; Fail-safe
    mov     gs,bx
    pop     bx

    cmp     ax,INT_LOAD_FILE
    je      .loadFile
    cmp     ax,INT_READ_FILE
    je      .readFile
    cmp     ax,INT_WRITE_FILE
    je      .writeFile
    cmp     ax,INT_EXEC_PROGRAM
    je      .execProgram
    cmp     ax,INT_GPU_GRAPHICS
    je      .gpuGraphics
    cmp     ax,INT_GPU_TEXT
    je      .gpuText

    cmp     ax,INT_KEYPRESS
    je      .getChar
    cmp     ax,INT_GET_CURSOR_POS
    je      .getCursorPos
    cmp     ax,INT_SET_CURSOR_POS
    je      .setCursorPos

    cmp     ax,INT_DRAW_PIXEL
    je      .drawPixel
    cmp     ax,INT_DRAW_BUFFER
    je      .drawBuffer

    cmp     ax,INT_CLEAR_SCREEN
    je      .clearScreen
    cmp     ax,INT_PRINT_STRING
    je      .printString
    cmp     ax,INT_PRINT_HEX
    je      .printHex
    cmp     ax,INT_PRINT_CHAR
    je      .printChar
    cmp     ax,INT_PRINT_NEWLINE
    je      .printNewLine
    cmp     ax,INT_PRINTN_STRING
    je      .printNString
    cmp     ax,INT_PRINT_COLORED
    je      .printColored

.return:
    pop     gs
    iret


; short ax loadFile( void * es:di , char * ds:si )
; short ax loadFile( void * dest, char * error )
.loadFile:
    push    bx
    mov     bx,cx
    call    fat_loadFile
    mov     word [ds:bx],ax
    pop     bx
    jmp     .return


; Reads a file into memory
;   In:
;     [ds:si] - File_name
;     [es:di] - File_data
;     cx - Maximum number of bytes
;   Out:
;     ax - File_size (-1 if error)
.readFile:
    push    bp
    mov     bp,sp
    sub     sp,4

    mov     word [bp-2],0FFFFh
    mov     word [bp-4],cx

.ro_size:
    call    fat_getFileSize
    cmp     ax,0FFFFh
    je      .ro_error0           ; error
    cmp     ax,word [bp-4]
    ja      .ro_error1           ; prevent buffer overflow
    mov     word [bp-2],ax

.ro_read:
    call    fat_loadFile
    test    ax,ax
    jne     .ro_error2          ; error

.ro_return:
    mov     ax,word [bp-2]
    mov     sp,bp
    pop     bp
    jmp     .return

.ro_error0:
    mov     word [bp-2],0FFFFh
    jmp     .ro_return

.ro_error1:
    mov     word [bp-2],0FFFEh
    jmp     .ro_return

.ro_error2:
    mov     word [bp-2],0FFFDh
    jmp     .ro_return


; Writes to a file
; [ds:si]   File_name
; [es:di]   File_data
; cx        File_size
.writeFile:
    push    ds          ; preserve
    push    es
    push    si
    push    di
    push    cx

    push    cx          ; parameters are altered
    push    di          ; so we can not pop them
    push    es          ; off the stack to get
    push    si          ; their original values
    push    ds          ; back
    call    fat_writeFile2
    add     sp,10

    pop     cx          ; restore
    pop     di
    pop     si
    pop     es
    pop     ds

    jmp     .return

; void ax execProgram( char * ds:si )
.execProgram:
    ; NOTE:
    ;   Impossible as is, thus cmd should load
    ;   it to the proper address and boot from
    ;   that point onwards.
    jmp     .return

; void func(void)
.gpuGraphics:
    push    ax
    mov     ah,0
    mov     al,13h      ; VGA / 16-colors / 320x200
    int     10h
    pop     ax
    jmp     .return

; void func(void)
.gpuText:
    push    ax
    mov     ah,0
    mov     al,03h      ; Text / 16-colors / 80x25
    int     10h
    pop     ax
    jmp     .return

; short ax getChar( void )
.getChar:
    mov     ah,00h
    int     16h
    movzx   ax,al
    jmp     .return

; ax, cx, dx
.getCursorPos:
    mov     bh,0
    mov     ah,3
    int     10h
    jmp     .return

; NULL
.setCursorPos:
    mov     ah,2
    mov     bh,0
    ;mov     dh,byte [row]
    ;mov     dl,byte [column]
    int     10h
    jmp     .return

; ax = INT_DRAW_PIXEL
; bx = color
; cx = x-pos
; dx = y-pos
.drawPixel:
    push    ax
    push    bx
    mov     ah,0Ch
    mov     al,bl
    xor     bx,bx
    int     10h
    pop     bx
    pop     ax
    jmp     .return

; ax = INT_DRAW_BUFFER
; es:bx = source buffer
.drawBuffer:            ; TODO:
    pusha
    push    ds
    push    es

    ; Offsets
    mov     si,bx
    xor     di,di

    ; Setup the new segments
    mov     bx,es       ; Source buffer
    mov     ds,bx
    mov     bx,0A000h   ; Video memory
    mov     es,bx

    ; The count
    mov     cx,(160*100)        ; All the pixels
    rep movsb

    pop     es
    pop     ds
    popa
    jmp     .return


; void clearScreen( void )
.clearScreen:
    push    ax
    mov     al,3
    mov     ah,0
    int     10h
    pop     ax
    jmp     .return     ; Cleared using video reset

;    push    ax          ; Clearing using \r\n
;    push    bx
;    xor     ax,ax
;    xor     bx,bx
;    mov     bh,0
;    mov     ah,2
;    mov     dh,24       ; HEIGHT: 25-characters
;    mov     dl,79       ; WIDTH : 80-characters
;    int     10h
;    pop     bx
;    pop     ax
;    push    cx
;    mov     cx,25
;.continue:
;    call    print_newline
;    loop    .continue
;    pop     cx
;    jmp     .return

; void printString( char * ds:si )
.printString:
    call    print
    jmp     .return

; void printHex( short cx )
.printHex:
    mov     ax,cx
    call    print_hex
    jmp     .return

; void printChar( char cl )
.printChar:
    pusha
    mov     bh,0
    mov     bl,7
    mov     ax,cx
    call    print_char
    popa
    jmp     .return

; void printNewLine( void )
.printNewLine:
    call    print_newline
    jmp     .return

; void printNString( char * ds:si, short cx )
.printNString:
    call    printn
    jmp     .return

.printColored:
    pusha
    mov     ax,cx
    mov     cx,1
    mov     bh,0
    call    print_colored
    popa
    jmp     .return


;===========
; DEPENDENCIES
;===========
%include "src/kernel/std.inc"
%include "src/kernel/fat12.inc"
%include "src/kernel/tests.inc"


;===========
; test_data
;===========
data_name   db 'COPY01  TXT',0
data_buff   times 510 db 1
.cluster1   times 4 db 2
data_size   dw $-data_buff

