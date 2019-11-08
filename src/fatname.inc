

;===============================================
; Convert the filename string into a FAT
; compatible filename that can be used to load
; files from the drive.
;   In:
;     ds:si - Source to copy from.
;     es:di - Destination to copy to.
;   Out:
;     N/A
;===============================================
cmd_convertString:
    push    bp
    mov     bp,sp
    push    di

.clear:
    push    di
    push    si
    push    cx
    push    ax

    mov     al,20h      ; KEY_SPACE
    mov     cx,11
    rep stosb

    pop     ax
    pop     cx
    pop     si
    pop     di

.name:
    call    cmd_copyName

.extension:
    add     di,8
    call    cmd_copyExtension

.return:
    pop     di
    mov     sp,bp
    pop     bp
    ret


;===============================================
cmd_copyName:
    push    bp
    mov     bp,sp
    push    cx

    push    si

.len:
    mov     al,byte [ds:si]
    cmp     al,0        ; '\0'
    je      .break
    cmp     al,2Eh      ; '.'
    je      .break
    inc     si
    jmp     .len

.break:
    mov     cx,si       ; end
    pop     si          ; start
    sub     cx,si       ; end-start

.bounds:
    cmp     cx,0
    jbe     .return
    cmp     cx,8
    jbe     .copy
    mov     cx,8

.copy:
    call    cmd_copyString

.return:
    pop     cx
    mov     sp,bp
    pop     bp
    ret


;===============================================
cmd_copyExtension:
    push    bp
    mov     bp,sp
    push    si
    push    cx
    push    dx

    mov     dx,si

; Searches for the last character of the string
.end:
    mov     al,byte [ds:si]
    cmp     al,0                ; '\0'
    je      .end_break
    inc     si
    jmp     .end

.end_break:
    mov     cx,si

; Searches for the first character after the extension seperator
.start:
    mov     al,byte [ds:si]
    cmp     al,2Eh              ; '.'
    je      .start_break
    cmp     dx,si
    jae     .return             ; no extension found
    dec     si
    jmp     .start

.start_break:
    sub     cx,si

.bounds:
    cmp     cx,0
    jbe     .empty
    cmp     cx,3
    jbe     .copy
    mov     cx,3

.copy:
    call    cmd_copyString
    jmp     .return

.empty:
    mov     byte [es:di+0],42h      ; 'B'
    mov     byte [es:di+1],49h      ; 'I'
    mov     byte [es:di+2],4Eh      ; 'N'

.return:
    pop     dx
    pop     cx
    pop     si
    mov     sp,bp
    pop     bp
    ret


;===============================================
cmd_copyString:
    push    si
    push    di
    push    cx

.loop:
    mov     al,byte [ds:si]
    cmp     al,0
    je      .return

.stage1:
    cmp     al,61h      ; 'a'
    jb      .stage2
    cmp     al,7Ah      ; 'z'
    ja      .stage2
    sub     al,20h
    jmp     .copy

.stage2:
    cmp     al,41h
    jb      .stage3
    cmp     al,5Ah
    jbe     .copy

.stage3:
    cmp     al,30h
    jb      .continue
    cmp     al,39h
    ja      .continue

.copy:
    mov     byte [es:di],al
    inc     di
    dec     cx

.continue:
    inc     si
    cmp     cx,0
    jne     .loop

.return:
    pop     cx
    pop     di
    pop     si
    ret
