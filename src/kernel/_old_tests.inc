
test_guid   db 'DMMY10  TXT',0
test_cfile  db 'DMMY11  TXT',0


.tests_start:
    jmp     .test_new_writeFile

;    mov     bx,ds
;    mov     es,bx
;    mov     di,test_var
;
;    mov     ax,2                    ; CMD.BIN
;    call    fat_rootGetEntry
;    test    ax,ax
;    jne     .skip
;
;    mov     si,test_var
;    mov     cx,8
;    call    printn
;
;    mov     byte [ds:si],041h       ; AMD.BIN
;    mov     ax,2
;    call    fat_rootSetEntry
;    test    ax,ax
;    je      .clear
;
;.skip:
;    mov     si,test_err
;    call    print

;.write_test:
;    push    es
;    push    ds

;    mov     cx,32
;    sub     sp,cx
;    mov     bp,sp
;    lea     di,[bp]
;    mov     si,test_name
;    call    fat_writeFile

;    pop     ds
;    pop     es

.test_old_getRootEntry:
    mov     bx,ds
    mov     es,bx
    mov     di,test_var

    mov     ax,2                    ; CMD.BIN
    call    fat_rootGetEntry
    call    print_hex
    call    print_newline

    mov     si,test_var
    mov     cx,11
    call    printn
    call    print_newline

.test_new_calc:
    mov     ax,11
    call    fat_calcClusters
    call    print_hex
    call    print_newline

.test_new_alloc:
    mov     ax,027h
    mov     cx,5
    call    fat_allocClusters
    call    print_hex
    call    print_newline
    mov     ax,cx
    push    ax
    call    print_hex
    call    print_newline
    pop     ax
    jmp     .test_new_reloc

.test_new_free:
    mov     ax,029h
    call    fat_freeClusters
    call    print_hex
    call    print_newline

.test_new_write:
    mov     ax,028h
    call    fat_getLastFileCluster
    call    print_hex
    call    print_newline

.test_new_reloc:
    push    ax
    mov     cx,ax
    mov     ax,2                    ; SHRINK: not implemented so ok :-)
    call    fat_relocClusters2
    call    print_hex
    call    print_newline
    pop     ax

    mov     cx,ax
    mov     ax,7                    ; GROW: successful
    call    fat_relocClusters2
    call    print_hex
    call    print_newline

.test_new_getEntryId:
    mov     si,test_guid
    call    fat_getEntryId
    push    ax

    call    print_hex
    call    print_newline

    mov     si,test_guid
    call    print
    call    print_newline

.test_new_fileResize:
    mov     si,test_guid
    mov     cx,word [data_size]
    pop     ax
    call    fat_fileResize
    call    print_hex
    call    print_newline
    jmp     .test_new_writeFile

.test_new_createFile:
    mov     si,test_cfile
    call    fat_createFile
    call    print_hex
    call    print_newline
    jmp     .keypress

.test_new_readOrCreate:
    push    bp
    mov     bp,sp
    sub     sp,32

    mov     bx,ds
    mov     es,bx
    lea     di,[bp-32]
    ;mov     si,test_guid
    mov     si,test_cfile

    call    fat_fileReadOrCreateEntry
    call    print_hex
    call    print_newline

    test    ax,ax
    jne     .test_new_roc_return

    lea     si,[bp-32]
    mov     cx,11
    call    printn
    call    print_newline

.test_new_roc_return:
    mov     sp,bp
    pop     bp
    jmp     .keypress

.test_new_writeData:
    push    29h         ; DMMY10.TXT / 0x8E00
    mov     ax,word [data_size]
    push    ax
    push    data_buff
    push    ds
    call    fat_writeData
    add     sp,8

    call    print_hex
    call    print_newline
    jmp     .keypress

.test_new_writeFile:
    push    word [data_size]        ; file size
    push    data_buff               ; data offset
    push    ds                      ; data stack
    push    data_name               ; file name offset
    push    ds                      ; file name stack
    call    fat_writeFile2
    add     sp,10

    call    print_hex
    call    print_newline
    jmp     .tests_end

    ; int fat_writeFile2( 0x202, 0x14C9, 0xAE0, 0x14BD, 0xAE0 )

.tests_end:
    mov     si,test_name
    call    print
    call    print_newline
