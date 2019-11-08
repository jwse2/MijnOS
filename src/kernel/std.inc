
;===============================================
; Prints a random string to the screen.
;   void print( const char * str )
;===============================================
print:
    pusha
    mov     ah,0Eh

.loop:
    lodsb                                       ; DS:SI
    cmp     al,0
    je      .done
    int     10h
    jmp     .loop

.done:
    popa
    ret


;===============================================
; Prints a single character to the screen.
;   void print_char( const char c )
;===============================================
print_char:
    push    ax
    mov     ah,0Eh
    int     10h
    pop     ax
    ret


;===============================================
; Prints a single character with attributes.
;===============================================
print_colored:
    mov     ah,09h
    int     10h
    ;ret     ; FIXME:

    ; colored does not advance the cursor, as
    ; such advance it manually.
    xor     bx,bx
    mov     ah,03h
    int     10h

    ; Ensure we do not reach beyond the screen
    cmp     dl,79
    je      .fold
    add     dl,1
    jmp     .set

    ; Fold the line
.fold:
    mov     dl,0
    cmp     dh,24
    jne     .increase
    mov     dh,0
    jmp     .set

.increase:
    add     dh,1

.set:
    xor     bx,bx
    mov     ah,02h
    int     10h
    ret


;===============================================
; Prints a string till either the null-terminator
; or the maximum, n, has been reached.
;   void print( const char * str, short n )
;===============================================
printn:
    pusha
    mov     ah,0Eh

.loop:
    cmp     cx,0
    jbe     .done
    lodsb
    cmp     al,0
    je      .done
    int     10h
    sub     cx,1
    jmp     .loop

.done:
    popa
    ret


;===============================================
; Prints a new line.
;   void print( void )
;===============================================
print_newline:
    push    ax
    mov     ah,0Eh
    mov     al,0Dh
    int     10h
    mov     al,0Ah
    int     10h
    pop     ax
    ret


;===============================================
; Prints a HEX byte onto the screen.
;   void print( WORD hex )
;===============================================
print_hex:
    pusha

    mov     cx,ax       ; Swap the bits for proper output
    xor     dx,dx

    shl     ax,4
    and     ax,0F00h
    or      dx,ax

    mov     ax,cx
    shr     ax,4
    and     ax,0F0h
    or      dx,ax

    mov     ax,cx
    shr     ax,12
    or      dx,ax

    mov     ax,cx
    shl     ax,12
    or      dx,ax

    mov     ax,dx       ; Swapped result is now in ax

    mov     bh,0        ; Teletype
    mov     bl,0Fh

    mov     cx,4        ; 2-bytes

    ; Minimum is 1-byte
.loop:
    and     ax,0Fh

    cmp     al,10
    jl      .low
    add     ax,07h                              ; 41h - 30h - 0Ah = 07h

.low:
    add     ax,30h
    mov     ah,0Eh
    int     10h

    mov     ax,dx       ; Shift and store
    shr     ax,4
    mov     dx,ax

    loop    .loop

.done:
    popa
    ret


;===============================================
; Copies the maximum number of characters from
; the source string to the destination string.
; The function ensures zero-termination of the
; destination string.
;   int strncpy(char *dest, char *src, int n)
; In:
;   ds:si - Source string
;   es:di - Destination string
;   cx - The maximum number of characters
; Out:
;   ax - Number of characters copied
;===============================================
strncpy:
    push    bp
    mov     bp,sp
    push    si
    push    di
    push    cx

.loop:
    mov     al,byte [ds:si]
    mov     byte [es:di],al
    test    al,al
    je      .end
    add     si,1
    add     di,1
    sub     cx,1
    cmp     cx,0
    jne     .loop

.limit:
    sub     di,1                                ; ensure zero-termination
    mov     byte [es:di],0

.end:
    mov     ax,cx
    pop     cx
    push    cx
    push    dx

    xor     dx,dx
    sub     ax,cx
    mov     cx,0FFFFh
    mul     cx

    pop     dx
    pop     cx
    pop     di
    pop     si
    mov     sp,bp
    pop     bp
    ret



;===============================================
; Compares up to the maximum number of
; characters from the source string to the
; destination string.
;   int strncmp(char *dest, char *src, int n)
; In:
;   ds:si - Source string
;   es:di - Destination string
;   cx - The maximum number of characters
; Out:
;   ax - Zero if equal; otherwise, non-zero.
;===============================================
strncmp:
    push    bp
    mov     bp,sp
    push    cx
    push    dx
    push    si
    push    di

.loop:
    mov     al,byte [ds:si]
    mov     dl,byte [es:di]

    test    al,al       ; string termination
    je      .not_equal

    cmp     al,dl       ; strings differ
    jne     .not_equal

    add     si,1
    add     di,1
    loop    .loop       ; max length reached?

.equal:
    xor     ax,ax

.return:
    pop     di
    pop     si
    pop     dx
    pop     cx
    mov     sp,bp
    pop     bp
    ret

.not_equal:
    mov     ax,0FFFFh   ; NOTE: not the standard C/C++ direction
    jmp     .return
