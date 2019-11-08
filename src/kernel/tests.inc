[BITS 16]


file_cmd    db 'CMD     BIN' 
cmd_found   db 'CMD found', 0Dh, 0Ah, 0
cmd_nfound  db 'CMD not found', 0Dh, 0Ah, 0



test_common:
    call    test_loadSector
    call    print_newline

    call    test_search
    call    print_newline

    call    test_fcluster
    call    print_newline

    call    test_strncpy
    call    print_newline

    call    test_loadf
    call    print_newline

    call    test_loadById
    call    print_newline

    ret



test_search:
    push    bp
    mov     bp,sp
    sub     sp,32       ; Will hold our FAT entry

    lea     di,[bp-32]
    ;mov     si,file_cmd
    mov     si,test_file

    push    es
    push    bx
    mov     bx,ss
    mov     es,bx
    push    si
    push    di
    call    fat_searchEntry
    add     sp,4
    pop     bx
    pop     es

    test    ax,ax
    jne     .not_found

    mov     si,cmd_found
    call    print
    call    print_newline

    push    ds
    push    bx
    mov     bx,ss
    mov     ds,bx
    mov     word [bp-21],0
    lea     ax,[bp-32]
    mov     si,ax
    call    print
    call    print_newline
    pop     bx
    pop     ds

    mov     ax,word [bp-6]          ; 0005h / 0016h
    call    print_hex
    call    print_newline

    jmp     .return

.not_found:
    mov     si,cmd_nfound
    call    print

.return:
    mov     sp,bp
    pop     bp
    ret



test_loadSector:
    push    bp
    mov     bp,sp
    sub     sp,512
    push    di

    push    es
    push    bx
    mov     bx,ss
    mov     es,bx
    lea     di,[bp-512]
    mov     ax,0
    call    fat_loadRootSector
    pop     bx
    pop     es

    mov     ax,word [bp-512]
    call    print_hex
    mov     ax,word [bp-480]
    call    print_hex
    mov     ax,word [bp-448]
    call    print_hex
    mov     ax,word [bp-416]
    call    print_hex

    pop     di
    mov     sp,bp
    pop     bp
    ret



test_fcluster:
    push    bp
    mov     bp,sp
    mov     ax,0002h    ; kernel
    ;mov     ax,341     ; sector 0-1 boundary check

.next:
    call    fat_findNextCluster
    mov     cx,ax
    call    print_hex
    cmp     cx,0FF0h
    jb      .next

.return:
    mov     sp,bp
    pop     bp
    ret



test_strncpy:
    push    bp
    mov     bp,sp
    sub     sp,010h
    pusha
    push    es
    push    ds

    mov     word [bp-2],04142h
    mov     word [bp-4],04344h
    mov     word [bp-6],04546h
    mov     word [bp-8],04748h

    lea     si,[bp-008h]
    lea     di,[bp-010h]

    mov     bx,ss
    mov     ds,bx
    mov     es,bx

    mov     cx,8
    call    strncpy

    lea     si,[bp-010h]
    call    print
    call    print_newline
    call    print_hex
    call    print_newline

    pop     ds
    pop     es
    popa
    mov     sp,bp
    pop     bp
    ret



test_file   db 'DMMY00  TXT'
test_error  db 'Error', 0
test_start  db 'Start', 0
test_buffer times 512 db 00h

test_loadf:
    push    bp
    mov     bp,sp
    pusha

    mov     si,test_file
    mov     di,test_buffer
    call    fat_loadFile
    test    ax,ax
    jne     .error

    mov     di,test_buffer
    mov     ax,word [di]
    call    print_hex

.succces:
    mov     si,test_start
    call    print
    mov     si,test_buffer
    call    print
    jmp     .return

.error:
    mov     si,test_error
    call    print
    jmp     .return

.return:
    popa
    mov     sp,bp
    pop     bp
    ret



test_loadById:
    push    bp
    mov     bp,sp
    sub     sp,32
    sub     sp,2
    push    es
    push    di
    push    bx

    mov     word [ss:bp-34],4

.loopTest:

    lea     di,[bp-32]
    mov     bx,ss
    mov     es,bx

    mov     ax,word [ss:bp-34]
    call    fat_getEntryById
    call    print_hex
    call    print_newline

    push    ds
    push    bx

    mov     bx,ss
    mov     ds,bx

    lea     si,[bp-32]
    mov     cx,11
    call    printn
    call    print_newline

    pop     bx
    pop     ds

    sub     word [ss:bp-34],1
    cmp     word [ss:bp-34],0
    ja      .loopTest


    pop     bx
    pop     di
    pop     es
    mov     sp,bp
    pop     bp
    ret
