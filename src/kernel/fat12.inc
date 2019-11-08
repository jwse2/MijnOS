[BITS 16]


;===========
; ERROR CODES
;===========
%define FERR_LOAD_FILE_0            0FFFFh      ; -1
%define FERR_LOAD_FILE_1            0FFFEh      ; -2
%define FERR_ENTRY_BY_ID            0FFFDh      ; -3
%define FERR_LOAD_GENERIC_0         0FFFCh      ; -4
%define FERR_LOAD_GENERIC_1         0FFFBh      ; -5


;===========
; Information needed for FAT12 I/O
;===========
iFAT_BytesPerSector     dw 512                  ; Number of bytes per sector
iFAT_SectorsPerCluster  db 1                    ; Number of sectors per cluster
iFAT_ReservedSectors    dw 1                    ; Number of reserved sectors
iFAT_NumberOfFats       db 2                    ; Number of FAT tables
iFAT_MaxRootEntries     dw 224                  ; Maximum number of root directories
iFAT_SmallSectors       dw 2880                 ; Total sector count (For FAT16 and older)
iFAT_SectorsPerFat      dw 9                    ; Sectors per FAT
iFAT_SectorsPerTrack    dw 18                   ; Sectors per track
iFAT_NumberOfHeads      dw 2                    ; Number of heads
iFAT_HiddenSectors      dd 0                    ; Number of hidden sectors
iFAT_DriveNo            db 0                    ; Physical drive number



;===============================================
; Sets the registers necessary for disk I/O.
;   In:
;     ax - The logical sector number.
;   Out:
;     ax, ch, cl, dh, cl
;===============================================
fat_setRegisters:
    push    bx
    push    ax
    mov     bx,ax                               ; Preserve the logical sector number  

    mov     dx,0                                ; First sector
    div     word [gs:iFAT_SectorsPerTrack]      ; edx = eax MOD SectorsPerTrack / eax = eax DIV SectorsPerTrack
    add     dl,1                                ; Physical sectors start at 1 not 0
    mov     cl,dl                               ; int 13h uses cl for sectors

    mov     ax,bx
    mov     dx,0
    div     word [gs:iFAT_SectorsPerTrack]
    mov     dx,0
    div     word [gs:iFAT_NumberOfHeads]
    mov     dh,dl
    mov     ch,al

    pop     ax
    pop     bx
    mov     dl,byte [gs:iFAT_DriveNo]
    ret



;===============================================
; Calculates the FAT table indices for the
; specified logical sector number.
;   In:
;     ax - The logical sector number.
;   Out:
;     ax - Zero based table index.
;     cx - Zero based byte offset.
;     dx - Bit shift value.
;===============================================
fat_calcTableIndices:
    push    bp
    mov     bp,sp

.validate:
    cmp     ax,2
    jb      .error                              ; x >= 2
    cmp     ax,word [gs:iFAT_SmallSectors]      ; x < iFAT_SmallSectors
    jb      .calculate

.error:
    mov     sp,bp
    pop     bp
    mov     ax,0FFFFh   ; -1 indicates an error
    ret

.calculate:
    ; offset in bytes and bits
    xor     dx,dx       ; dx:ax = ax * cx
    mov     cx,3        ; dx = (ax % cx)
    mul     cx          ; ax = (ax / cx)
    mov     cx,2        ; ax = byte offset in cluster
    div     cx          ; dx = modulo (bit-shifting)

    push    ax
    push    dx

    ; offset in sectors
    xor     dx,dx
    mov     cx,word [gs:iFAT_BytesPerSector]
    div     cx
    
    ; Final values
    ;mov     ax,ax      ; FAT sector
    pop     dx          ; Modulo/bit offset
    pop     cx          ; Byte offset

    ; Absolute to relative byte offset
    push    bx
    mov     bx,word [gs:iFAT_BytesPerSector]
    sub     bx,1
    and     cx,bx       ; (offset & (512-1))
    pop     bx

; ax - FAT sector; The zero-based index of the sector within the FAT table.
; cx - Byte offset; The byte offset within the FAT sector.
; dx - Bit offset; Bit offset for the respective entry.
.return:
    mov     sp,bp
    pop     bp
    ret



;===============================================
; Get the value of cluster X in the FAT.
;   In:
;     ax - The logical cluster number.
;   Out:
;     ax - The value of the FAT entry matching
;          the logical cluster number.
;===============================================
fat_getClusterValue:
    push    bp
    mov     bp,sp
    sub     sp,8                                ; local variables
    sub     sp,word [gs:iFAT_BytesPerSector]    ; FAT_0
    sub     sp,word [gs:iFAT_BytesPerSector]    ; FAT_1
    pusha
    push    es

    ; Store the original logical cluster number
    mov     word [bp-2],ax

.getIndices:                                    ; Get the indices
    call    fat_calcTableIndices
    cmp     ax,0FFFFh
    jne     .loadFAT

.errIndices:                                    ; Could not get indices
    pop     es
    popa
    mov     ax,0FF7h                            ; Bad cluster
    mov     sp,bp
    pop     bp
    ret

.loadFAT:                                       ; Load the two sectors
    mov     word [bp-4],ax                      ; FAT sector
    mov     word [bp-6],cx                      ; Byte offset
    mov     word [bp-8],dx                      ; Bit shift

    ; Set the extra segment to the stack
    mov     bx,ss
    mov     es,bx

    ; Physical sector offset
    mov     cx,ax
    xor     ax,ax
    call    fat_getLogicalFat
    add     ax,cx

    ; Set the registers to load
    call    fat_setRegisters

    ; Our buffer will be on the stack and consist of two sectors
    lea     bx,[bp-8]
    sub     bx,word [gs:iFAT_BytesPerSector]
    sub     bx,word [gs:iFAT_BytesPerSector]

    mov     ah,2        ; READ
    mov     al,2        ; number of sectors to read

    stc
    int     13h
    jc      .errRead    ; Read errors
    cmp     al,2
    je      .getValue   ; Incorrect number of sectors

.errRead:                                       ; Could not read desired sectors
    pop     es
    popa
    mov     ax,0FF7h                            ; Bad cluster
    mov     sp,bp
    pop     bp
    ret

.getValue:
    lea     bx,[bp-8]
    sub     bx,word [gs:iFAT_BytesPerSector]
    sub     bx,word [gs:iFAT_BytesPerSector]
    mov     ax,word [bp-6]                      ; buffer[offset]
    add     bx,ax

    mov     cx,word [es:bx]                     ; value

    mov     ax,word [bp-8]                      ; bit shift
    test    ax,ax
    je      .even

.odd:
    shr     cx,4

.even:
    and     cx,0FFFh
    mov     word [bp-2],cx

; Cleans the stack and restores the registers
.return:
    pop     es
    popa
    mov     ax,word [bp-2]
    mov     sp,bp
    pop     bp
    ret



;===============================================
; Set the value of cluster X in the FAT.
;   In:
;     ax - The logical cluster number.
;     cx - The value to put into the cluster.
;   Out:
;     ax - Zero if succesful; otherwise, a non-zero value.
;===============================================
fat_setClusterValue:
    push    bp
    mov     bp,sp
    sub     sp,12                               ; local variables
    sub     sp,word [gs:iFAT_BytesPerSector]    ; FAT_0
    sub     sp,word [gs:iFAT_BytesPerSector]    ; FAT_1
    pusha
    push    es

    ; Store the original parameters
    mov     word [bp-2],ax
    mov     word [bp-10],cx

.getIndices:
    call    fat_calcTableIndices
    cmp     ax,0FFFFh
    jne     .loadFAT

.errIndices:
    pop     es
    popa
    mov     ax,0FFFFh
    mov     sp,bp
    pop     bp
    ret

.loadFAT:                                       ; Load the two sectors
    mov     word [bp-4],ax                      ; FAT sector
    mov     word [bp-6],cx                      ; Byte offset
    mov     word [bp-8],dx                      ; Bit shift

    ; Set the extra segment to the stack
    mov     bx,ss
    mov     es,bx

    ; Physical sector offset
    mov     cx,ax
    xor     ax,ax
    call    fat_getLogicalFat
    add     ax,cx
    mov     word [bp-12],ax                     ; Physical sector

    ; Set the registers to load
    call    fat_setRegisters

    ; Our buffer will be on the stack and consist of two sectors
    lea     bx,[bp-12]
    sub     bx,word [gs:iFAT_BytesPerSector]
    sub     bx,word [gs:iFAT_BytesPerSector]

    mov     ah,2        ; READ
    mov     al,2        ; number of sectors to read

    stc
    int     13h
    jc      .errRead    ; Read errors
    cmp     al,2
    je      .getValue   ; Incorrect number of sectors

.errRead:                                       ; Could not read desired sectors
    pop     es
    popa
    mov     ax,0FFFFh
    mov     sp,bp
    pop     bp
    ret

.getValue:
    lea     bx,[bp-12]
    sub     bx,word [gs:iFAT_BytesPerSector]
    sub     bx,word [gs:iFAT_BytesPerSector]
    mov     ax,word [bp-6]                      ; buffer[offset]
    add     bx,ax

    ; The value as currently stored in the table
    mov     cx,word [es:bx]                     ; value

    ; Test whether we need to shift or not
    mov     ax,word [bp-8]                      ; bit shift
    test    ax,ax
    je      .even

.odd:                                           ; UPPER 12-bits
    mov     ax,word [bp-10]
    shl     ax,4
    and     ax,0FFF0h   
    and     cx,0000Fh
    or      ax,cx
    jmp     .write

.even:                                          ; LOWER 12-bits
    mov     ax,word [bp-10]
    and     ax,00FFFh
    and     cx,0F000h
    or      ax,cx
    jmp     .write

.write:
    ; Replace the original with the new value
    mov     word [es:bx],ax

    ; Write the loaded sectors back to the medium
    mov     ax,word [bp-12]
    call    fat_setRegisters

    ; We need to write the entire sectors
    lea     bx,[bp-12]
    sub     bx,word [gs:iFAT_BytesPerSector]
    sub     bx,word [gs:iFAT_BytesPerSector]

    mov     ah,3
    mov     al,2

    stc
    int     13h
    jc      .errWrite
    cmp     al,2
    je      .write2

.errWrite:
    pop     es
    popa
    mov     ax,0FFFFh
    mov     sp,bp
    pop     bp
    ret

; FAT12 has two tables, so write it to both tables
.write2:
    movzx   cx,byte [gs:iFAT_NumberOfFats]
    sub     cx,1        ; We already wrote 1 sector

.write_loop:
    push    cx

    mov     ax,word [bp-12]
    add     ax,word [gs:iFAT_SectorsPerFat]
    mov     word [bp-12],ax
    call    fat_setRegisters

    ; We need to write the entire sectors
    lea     bx,[bp-12]
    sub     bx,word [gs:iFAT_BytesPerSector]
    sub     bx,word [gs:iFAT_BytesPerSector]
    
    mov     ah,3
    mov     al,2

    stc
    int     13h
    jc      .errWrite2
    cmp     al,2
    jne     .errWrite2

    pop     cx
    sub     cx,1
    jne     .write_loop

; Cleans the stack and restores the registers
.return:
    pop     es
    popa
    xor     ax,ax
    mov     sp,bp
    pop     bp
    ret

.errWrite2:
    pop     cx
    jmp     .errWrite



;===============================================
; Reads a cluster to the destination.
;   In:
;     ax - The logical cluster number.
;     es:di - The destination to copy the data to.
;   Out:
;     ax - Zero if successful; otherwise, a non-zero value.
;===============================================
fat_readCluster:
    push    bp
    mov     bp,sp
    pusha

    ; Convert the logical number into the physical
    call    fat_logicalToPhysical
    call    fat_setRegisters

    mov     bx,di
    mov     ah,2
    mov     al,1

    stc
    int     13h
    jc      .error
    cmp     al,1
    jne     .error

.return:
    popa
    xor     ax,ax
    mov     sp,bp
    pop     bp
    ret

.error:
    popa
    mov     ax,0FFFFh
    mov     sp,bp
    pop     bp
    ret



;===============================================
; Writes a cluster to the storage medium.
;   In:
;     ax - The logical cluster number.
;     es:si - The data to copy.
;   Out:
;     ax - Zero if successful; otherwise, a non-zero value.
;===============================================
fat_writeCluster:
    push    bp
    mov     bp,sp
    pusha

    ; Convert the logical number into the physical
    call    fat_logicalToPhysical
    call    fat_setRegisters

    mov     bx,si
    mov     ah,3
    mov     al,1

    stc
    int     13h
    jc      .error
    cmp     al,1
    jne     .error

.return:
    popa
    xor     ax,ax
    mov     sp,bp
    pop     bp
    ret

.error:
    popa
    mov     ax,0FFFFh
    mov     sp,bp
    pop     bp
    ret



;===============================================
; Calculates the root indices for the specified
; index number.
;   In:
;     ax - The index number.
;   Out:
;     ax - Zero based root sector index.
;     cx - Zero based byte offset.
;===============================================
fat_calcRootIndices:
    push    bp
    mov     bp,sp

.validate:
    cmp     ax,0
    jb      .error
    cmp     ax,word [gs:iFAT_MaxRootEntries]
    jb      .calculate

.error:
    mov     ax,0FFFFh
    mov     sp,bp
    pop     bp
    ret

.calculate:
    shl     ax,5                                ; 32-bytes per entry
    mov     cx,word [gs:iFAT_BytesPerSector]    ; 512-bytes per sector

    push    dx
    xor     dx,dx       ; dx:ax = ax * cx
    div     cx          ; dx = (ax % cx)
    mov     cx,dx       ; ax = (ax / cx)
    pop     dx

.return:
    mov     sp,bp
    pop     bp
    ret



;===============================================
; Gets a copy of the root entry.
;   In:
;     ax - root index
;     es:di - Destination to store the entry.
;   Out:
;     ax - Zero if succesful; otherwise, a non-zero value.
;===============================================
fat_rootGetEntry:
    push    bp
    mov     bp,sp
    sub     sp,6
    sub     sp,word [gs:iFAT_BytesPerSector]
    pusha
    push    es
    push    di

.validate:
    cmp     ax,0
    jb      .error
    cmp     ax,word [gs:iFAT_MaxRootEntries]
    jb      .loadSector

.error:
    pop     di
    pop     es
    popa
    mov     ax,0FFFFh
    mov     sp,bp
    pop     bp
    ret

.loadSector:
    mov     word [bp-2],ax                      ; Store the index
    call    fat_calcRootIndices                 ; Get the indices
    cmp     ax,0FFFFh
    je      .error

    mov     word [bp-4],ax                      ; Sector offset
    mov     word [bp-6],cx                      ; Byte offset

    mov     dx,ax
    call    fat_getLogicalRoot
    add     ax,dx
    call    fat_setRegisters

    mov     bx,ss
    mov     es,bx
    lea     bx,[bp-6]
    sub     bx,word [gs:iFAT_BytesPerSector]

    mov     ah,2
    mov     al,1

    stc
    int     13h
    jc      .error
    cmp     al,1
    jne     .error

.copy:
    pop     di
    pop     es

    push    ds
    push    si

    mov     bx,ss
    mov     ds,bx
    
    lea     si,[bp-6]
    sub     si,word [gs:iFAT_BytesPerSector]
    add     si,word [bp-6]                      ; offset

    mov     cx,32       ; [ds:si]
    rep movsb           ; [es:di]

    pop     si
    pop     ds

.return:
    popa
    xor     ax,ax
    mov     sp,bp
    pop     bp
    ret



;===============================================
; Sets a root entry to the given values.
;   In:
;     ax - root index
;     es:si - The entry to store onto the medium.
;   Out:
;     ax - Zero if succesful; otherwise, a non-zero value.
;===============================================
fat_rootSetEntry:
    push    bp
    mov     bp,sp
    sub     sp,6
    sub     sp,word [gs:iFAT_BytesPerSector]
    pusha
    push    es
    push    si

.validate:
    cmp     ax,0
    jb      .error0
    cmp     ax,word [gs:iFAT_MaxRootEntries]
    jb      .loadSector

.error0:
    call    print_hex
    call    print_newline
    
    pop     si
    pop     es
    popa
    mov     ax,0FFFFh
    mov     sp,bp
    pop     bp
    ret

.error1:
    pop     si
    pop     es
    popa
    mov     ax,0FFFEh
    mov     sp,bp
    pop     bp
    ret

.error2:
    pop     si
    pop     es
    popa
    mov     ax,0FFFDh
    mov     sp,bp
    pop     bp
    ret

.loadSector:
    mov     word [bp-2],ax                      ; Store the index
    call    fat_calcRootIndices                 ; Get the indices
    cmp     ax,0FFFFh
    je      .error1

    mov     word [bp-4],ax                      ; Sector offset
    mov     word [bp-6],cx                      ; Byte offset

    mov     dx,ax
    call    fat_getLogicalRoot
    add     ax,dx
    call    fat_setRegisters

    mov     bx,ss
    mov     es,bx
    lea     bx,[bp-6]
    sub     bx,word [gs:iFAT_BytesPerSector]

    mov     ah,2
    mov     al,1

    stc
    int     13h
    jc      .error2
    cmp     al,1
    jne     .error2

.copy:                  ; Copy input to the stack buffer
    pop     si
    pop     es          ; Restore the source address

    push    ds          ; Keep a copy of them all
    push    es
    push    si
    push    di

    mov     bx,es       ; Source [ds:si]
    mov     ds,bx

    mov     bx,ss       ; Destination [es:di]
    mov     es,bx
    
    lea     di,[bp-6]
    sub     di,word [gs:iFAT_BytesPerSector]
    add     di,word [bp-6]                      ; offset

    mov     cx,32       ; [ds:si]
    rep movsb           ; [es:di]

    pop     di
    pop     si
    pop     es
    pop     ds

    push    es          ; Store the original on the stack once more
    push    si

.write:                                         ; Write the changes onto the disk
    mov     dx,word [bp-4]                      ; sector offset
    call    fat_getLogicalRoot
    add     ax,dx
    call    fat_setRegisters

    mov     bx,ss
    mov     es,bx
    lea     bx,[bp-6]
    sub     bx,word [gs:iFAT_BytesPerSector]

    mov     ah,3
    mov     al,1

    stc
    int     13h
    jc      .error2
    cmp     al,1
    jne     .error2

.return:
    pop     si
    pop     es
    popa
    xor     ax,ax
    mov     sp,bp
    pop     bp
    ret



;===============================================
; Writes a file to the medium.
;   In:
;     es:bx - File name
;   Out:
;     ax - Zero if successful.
;===============================================
;fat_doesFileExist:
;    push    bp
;    mov     bp,sp

;.return:
;    mov     sp,bp
;    pop     bp
;    ret



;===============================================
; Reallocates clusters. This includes growning
; as well as shrinking in number of clusters.
;   In:
;     ax - The number of necessary clusters.
;     cx - The starting cluster.
;   Out:
;     ax - Zero if successful.
;===============================================
;fat_relocClusters:
;    push    bp
;    mov     bp,sp
;    sub     sp,6
;
;    ; Store a copy of the value
;    mov     word [bp-2],ax  ; reqClusters - The requested number of clusters.
;    mov     word [bp-4],cx  ; rootCluster - The starting cluster.
;    mov     word [bp-6],0   ; numClusters - The number of used clusters.
;
;    ; The starting cluster will be our root
;    mov     ax,cx
;
;
;.loop:
;    call    fat_getClusterValue
;    push    ax
;
;    ; Increment the number of used clusters
;    mov     cx,word [bp-6]
;    add     cx,1
;    mov     word [bp-6],cx
;
;    ; Now start comparing the cluster information
;    pop     ax
;
;    ; Take action based on the cluster index
;    cmp     cx,word [bp-2]
;    je      .lof        ; Set the last of file marker
;    ja      .above      ; Clear the cluser value
;
;; This is a cluster used for our current file
;.below:
;    cmp     ax,0        ; The cluster does not have a next
;    je      .allocate   ; cluster following it.
;    cmp     ax,0FF0h    ; The cluster is valid and has an
;    jb      .loop       ; other cluster following it.
;
;.allocate:  ; TODO: We need to allocate another cluster and link it...
;
;
;    jmp     .loop
;
;; This is a cluster previously used by our file
;.above:
;    push    ax
;    mov     cx,0
;    call    fat_setClusterValue
;    pop     ax
;    jmp     .loop
;
;; The current cluster is the last cluster of the file.
;.lof:
;    push    ax          ; Save it, we may need it for further processing
;    mov     cx,0FFFh
;    call    fat_setClusterValue
;    pop     ax
;    jmp     .loop
;
;.return:
;    mov     sp,bp
;    pop     bp
;    ret



;===============================================
; Writes a file to the medium.
;   In:
;     ds:si - File name
;     es:di - File data
;     cx    - File size
;   Out:
;     ax - Zero if successful.
;===============================================
;fat_writeFile:
;    push    bp
;    mov     bp,sp
;    sub     sp,46
;    push    bx
;
;    mov     word [bp-2],cx
;
;; Check if file with that name already exists.
;.findExisting:
;    push    es
;    mov     bx,ss
;    mov     es,bx
;    lea     di,[bp-44]                          ; [es:di] = FAT entry
;    call    fat_searchEntry                     ; Searches the root directory for a file with
;    pop     es                                  ; the given name.
;    
;    test    ax,ax
;    je      .fileExists
;
;; File does not exist, get the id of the first empty entry
;.fileNotFound:
;    call    fat_findEmptyFileEntry              ; Searches for an empty spot in the root directory
;    test    ax,0FFFFh                           ; No empty space found
;    je      .error0                             ; Otherwise, ax is now the index
;    mov     word [bp-46],ax                     ;   of the first empty spot.
;
;    call    fat_findEmptyCluster
;    cmp     ax,07FFFh
;    jge     .error0
;    mov     word [bp-4],ax
;
;; Fills the entry with basic file information
;.fillFileEntry:
;    ; 0-7   BYTE filename
;    ; 8-10  BYTE extension
;    ; 11    BYTE attributes
;    ; 12-13 WORD reserved
;    ; 14-15 WORD creationTime
;    ; 16-17 WORD creationDate
;    ; 18-19 WORD lastAccessDate
;    ; 20-21 WORD zeros
;    ; 22-23 WORD lastWriteTime
;    ; 24-25 WORD lastWriteDate
;    ; 26-27 WORD firstLogicalCluster
;    ; 28-31 DWORD fileSize
;
;    ; 1) Clear the entry
;    push    es
;    mov     bx,ss
;    mov     es,bx
;    lea     di,[bp-44]                          ; [es:di] = FAT entry
;    mov     cx,32
;.clear_loop:
;    mov     byte [es:di],0
;    add     di,1
;    sub     cx,1
;    jne     .clear_loop
;    pop     es
;
;    ; 2) Copy the name
;    push    es
;    push    si
;    mov     bx,ss
;    mov     es,bx
;    lea     di,[bp-44]
;    mov     cx,8        ; Do not copy extension!
;.copy_name:
;    mov     al,byte [ds:si]
;    mov     byte [es:di],al
;    add     si,1
;    add     di,1
;    sub     cx,1
;    jne     .copy_name
;    pop     si
;    pop     es
;
;    ; 3) Set type to ARCHIVE (aka file)
;    push    es
;    mov     bx,ss
;    mov     es,bx
;    lea     di,[bp-44]
;    add     di,11
;    mov     byte [es:di],020h
;    pop     es
;
;    ; 4) Set the file size
;    push    es
;    mov     bx,ss
;    mov     es,bx
;    lea     di,[bp-44]
;    add     di,28
;    ;mov     cx,word [bp-2]
;    ;mov     word [es:di],cx
;    mov     word [es:di],512        ; single cluster
;    mov     word [es:di+2],0        ; upper 16-bits
;    pop     es
;
;    ; 5) Set the first cluster
;    push    es
;    mov     bx,ss
;    mov     es,bx
;    lea     di,[bp-44]
;    add     di,26
;    mov     ax,word [bp-4]
;    mov     word [es:di],ax
;    mov     cx,0FFFh
;    call    fat_setClusterValue
;    pop     es
;
;    ; 6) Write the entry
;    push    es
;    mov     bx,ss
;    mov     es,bx
;    mov     ax,word [bp-46]     ; index
;    lea     si,[bp-44]
;    call    fat_rootSetEntry     ; Write the entry to the disk
;    pop     es
;
;
;; If it does, get the size and check if it changed, if changed
;; allocate or free FAT clusters accordingly.
;.fileExists:
;    ; When it comes here the information about the entry
;    ; is stored in the location [es:di]
;
;.clusers0:
;    ; Determine the number of clusters needed
;    xor     dx,dx
;    mov     ax,word [bp-2]                      ; size of the file to write
;    mov     cx,word [gs:iFAT_BytesPerSector]    ; ax = number of clusters needed
;    div     cx                                  ; dx = modulo
;    
;    ; TODO: FIXME: jump
;    test    dx,dx                               ; If dx is not zero it means we
;    je      .clusers1                           ; need to allocate another
;    add     ax,1                                ; cluster for storing data.
;
;.clusers1:
;    push    ax                                  ; Save the new number of clusters
;
;    ; Determine the number of clusters currently used
;    xor     dx,dx
;    mov     ax,word [bp-14]                     ; -44 + 30 = -14
;    mov     cx,word [gs:iFAT_BytesPerSector]
;    div     cx
;
;    test    dx,dx
;    je      .clusters_calc
;    add     ax,1
;
;.clusters_calc:
;    mov     cx,ax       ; Current
;    pop     ax          ; Needed
;
;    cmp     ax,cx       ; If the same number of clusters are used,
;    je      .writeData  ; just start writing the data.
;
;; ax now holds the number of clusters needed,
;; we need to check each root entry till we match
;; the new number. If the new number is lower than
;; the old number we must overwrite the last cluster
;; value to become the last of file. For the sake
;; of easiness we do not free other clusters.
;.allocClusters:
;
;
;
;
;
;.writeData:
;; 3) Write the data to the allocated clusters.
;; TODO:
;
;.error0:
;.return:
;    pop     bx
;    mov     sp,bp
;    pop     bp
;    ret



; Searches for the first empty cluster in the
; FAT table.
; TODO: Cluster boundary checks, it currently
;       keeps checking till the first empty space
;       or an error.
fat_findEmptyCluster:
    push    bp
    mov     bp,sp
    sub     sp,8
    sub     sp,word [gs:iFAT_BytesPerSector]
    sub     sp,word [gs:iFAT_BytesPerSector]
    push    es
    push    bx
    push    di
    push    cx
    push    dx

    mov     word [bp-8],0                       ; current sector

    mov     bx,ss                               ; Set the target segment
    mov     es,bx

.loadNext:
    lea     bx,[bp]
    sub     bx,8
    sub     bx,word [gs:iFAT_BytesPerSector]
    sub     bx,word [gs:iFAT_BytesPerSector]
    mov     di,bx                               ; es:di

    mov     ax,word [bp-8]                      ; sector_0
    add     word [bp-8],1                       ; sector++

    call    fat_loadFatSector                   ; 1st sector
    test    ax,ax
    jne     .error0 

    add     di,word [gs:iFAT_BytesPerSector]
    mov     ax,word [bp-8]                      ; sector_1

    call    fat_loadFatSector                   ; 2nd sector
    test    ax,ax
    jne     .error1

    mov     word [bp-6],0                       ; WORD logicalCluster

.compareNext:
    mov     ax,word [bp-6]                      ; logicalCluster
    xor     dx,dx           ; dx:ax = ax * cx
    mov     cx,3            ; dx = (ax % cx)
    mul     cx              ; ax = (ax / cx)
    mov     cx,2            ; ax = byte offset in current cluster
    div     cx              ; dx = modulo (for bit-shifting)

    push    bx
    add     bx,ax
    mov     cx,word [ss:bx]
    pop     bx

    test    ax,ax
    je      .even

.odd:
    shr     cx,4
    jmp     .compare

.even:
    and     cx,0FFFh
    jmp     .compare

.compare:
    test    cx,cx
    je      .found
    add     word [bp-6],1
    cmp     word [bp-6],0156h                   ; Max per sector
    ja      .loadNext
    jmp     .compareNext

.found:
    mov     ax,word [bp-6]
    jmp     .return

.error0:
    mov     ax,0FFFFh
    jmp     .return

.error1:
    mov     ax,0FFFEh
    jmp     .return

.return:
    pop     dx
    pop     cx
    pop     di
    pop     bx
    pop     es
    mov     sp,bp
    pop     bp
    ret


;===============================================
; Searches for and returns the index of the
; first empty file entry in the root directory.
;   In:
;     ? (ds:si - The name of the file.)
;   Out:
;     ax - Zero if failed; otherwise, the index.
;===============================================
fat_findEmptyFileEntry:
    push    bp
    mov     bp,sp
    sub     sp,8
    sub     sp,word [gs:iFAT_BytesPerSector]
    pusha
    push    es                                  ; Save the destination
    push    di
    mov     bx,ss
    mov     es,bx

    mov     ax,word [gs:iFAT_BytesPerSector]
    shr     ax,5
    mov     word [bp-6],ax                      ; WORD entriesPerSector

    call    fat_getSizeRoot
    mov     word [bp-2],ax                      ; WORD rootSize
    mov     word [bp-4],0                       ; WORD rootIndex

.loadSector:
    mov     word [bp-8],0                       ; WORD entriesIndex

    lea     ax,[bp]
    sub     ax,word [gs:iFAT_BytesPerSector]
    sub     ax,8
    mov     di,ax

    mov     ax,word [bp-4]
    call    fat_loadRootSector
    test    ax,ax
    jne     .error

    lea     dx,[bp]
    sub     dx,word [gs:iFAT_BytesPerSector]
    sub     dx,8                                ; dx now points to the first entry of the loaded sector

.search:
    mov     di,dx
    mov     al,byte [es:di]
    cmp     al,0E5h     ; State: erased
    je      .calculate
    cmp     al,0        ; State: free
    je      .calculate

    add     dx,32                               ; 32-bytes per entry

    mov     ax,word [bp-8]
    add     ax,1
    cmp     ax,word [bp-6]
    mov     word [bp-8],ax                      ; entriesIndex < entriesPerSector
    jb      .search

    mov     ax,word [bp-4]
    add     ax,1
    cmp     ax,word [bp-2]                      ; rootIndex < rootSize
    mov     word [bp-4],ax
    jb      .loadSector

.error:
    pop     di
    pop     es
    popa
    mov     ax,0FFFFh
    mov     sp,bp
    pop     bp
    ret

.calculate:
    xor     dx,dx
    mov     ax,word [bp-4]      ; bp-4 = rootIndex
    mov     cx,word [bp-6]      ; bp-6 = entriesPerSector
    mul     cx
    mov     cx,word [bp-8]      ; bp-8 = entriesIndex
    add     ax,cx
    mov     word [bp-2],ax
    ; entriesPerSector * rootIndex
    ; + entriesIndex

.return:
    pop     di
    pop     es
    popa
    mov     ax,word [bp-2]
    mov     sp,bp
    pop     bp
    ret


;===============================================
; Loads a file from the FAT and stores it in the
; destination. It assumes the destination is
; large enough to hold the file.
;   In:
;     ds:si - The name of the file.
;     es:di - The destination to store the data.
;   Out:
;     ax - Zero if successful.
;===============================================
fat_loadFile:
    push    bp
    mov     bp,sp
    sub     sp,44                               ; FAT name + extension + FAT entry
    sub     sp,MAX_PATH                         ; Input path
    pusha
    ;push    ds
    push    es
    push    di

    mov     bx,ss                               ; Copy to the stack
    mov     es,bx

.copy_start:
    ;lea     di,[bp-MAX_PATH-44]                 ; Copies the string from the source onto the stack
    ;mov     cx,MAX_PATH
    ;call    strncpy

    ;mov     bx,ss
    ;mov     ds,bx

.get_entry:
    ;lea     si,[bp-MAX_PATH-44]                 ; TODO: ERROR: DOES NOT SEEM TO RETURN!
    lea     di,[bp-44]
    call    fat_searchEntry
    test    ax,ax
    jne     .error0

    ; NOTE: good
    ;mov     ax,[bp-18]
    ;call    print_hex

    pop     di          ; We need our original destination
    pop     es
    ;pop     ds

    mov     bx,di               ; Buffer pointer
    mov     ax,word [bp-18]     ; Logical sector

.load_loop:
    call    fat_logicalToPhysical
    call    fat_setRegisters

    mov     ah,2
    mov     al,1
    
    stc
    int     13h
    jc      .error1
    cmp     al,1
    jne     .error2
    
    add     bx,word [gs:iFAT_BytesPerSector]

    mov     ax,word [bp-18]
    call    fat_findNextCluster
    mov     word [bp-18],ax

    cmp     ax,0FF0h
    jb      .load_loop

    mov     word [bp-2],0
    ;mov     ax,word [bp-16]
    ;mov     word [bp-2],ax
    jmp     .return

.error0:
    pop     di
    pop     es
    ;pop     ds
    mov     word [bp-2],0FFFDh  ; ax
    jmp     .return

.error1:
    mov     word [bp-2],0FFFFh  ; FERR_LOAD_FILE_0
    jmp     .return

.error2:
    mov     word [bp-2],0FFFEh  ; FERR_LOAD_FILE_1
    jmp     .return

.return:
    popa
    mov     ax,word [bp-2]
    mov     sp,bp
    pop     bp
    ret



;===============================================
; Gets the size of the file.
;   In:
;     ds:si - The name of the entry.
;   Out:
;     dx:ax - The size of the file, -1 on error.
;===============================================
fat_getFileSize:
    push    bp
    mov     bp,sp
    sub     sp,32
    pusha
    push    es
        
.load_entry:
    mov     bx,ss
    mov     es,bx
    lea     di,[bp-32]
    call    fat_searchEntry
    test    ax,ax
    jne     .error

.return:
    pop     es
    popa
    mov     dx,word [bp-2]
    mov     ax,word [bp-4]
    mov     sp,bp
    pop     bp
    ret

.error:
    mov     word [bp-2],0FFFFh
    mov     word [bp-4],0FFFFh
    jmp     .return


;===============================================
; Finds the next cluster value.
;   In:
;     ax - The current logical cluster.
;   Out:
;     ax - The value of the next logical cluster.
;===============================================
fat_findNextCluster:
    push    bp
    mov     bp,sp
    sub     sp,8
    sub     sp,word [gs:iFAT_BytesPerSector]
    sub     sp,word [gs:iFAT_BytesPerSector]

    ;cmp     ax,word [iFAT_SmallSectors]
    ;jb      .validArg
    ;mov     sp,bp
    ;pop     bp
    ;ret

.validArg:
    ; Save the destination and set the
    ; destination pointer to be our stack instead
    pusha
    push    es
    push    di
    mov     bx,ss
    mov     es,bx

    ; Immediately store the passed argument
    mov     word [bp-2],ax

;
; The input can be a variety of values. Based on
; the input we can determine if we have to load
; and which logical sector contains the data.
;
.checkInput:
    mov     ax,word [bp-2]                      ; 0FF0h - Reserved
    cmp     ax,0FF0h                            ; 0FF7h - Bad
    jae     .return                             ; 0FF8h - Last


    ; Determine the byte the start reading from,
    ; this determines the logical sector to load
    xor     dx,dx
    mov     cx,3
    mul     cx                                  ; dx:ax = ax * cx
    mov     cx,2                                ; dx = (ax % cx)
    div     cx                                  ; ax = (ax / cx)

    ; Store the values
    mov     word [bp-4],ax                      ; byte offset
    mov     word [bp-6],dx                      ; modulo


    ; ax determines the logical sector
    xor     dx,dx
    mov     cx,word [gs:iFAT_BytesPerSector]
    div     cx
    mov     word [bp-8],ax                      ; logical sector

    ; Destination to read to
    lea     bx,[bp]
    sub     bx,8
    sub     bx,word [gs:iFAT_BytesPerSector]
    sub     bx,word [gs:iFAT_BytesPerSector]
    mov     di,bx

    call    fat_loadFatSector                   ; 1st sector
    test    ax,ax
    jne     .return

    mov     ax,word [bp-8]
    add     ax,1
    add     di,word [gs:iFAT_BytesPerSector]

    call    fat_loadFatSector                   ; 2nd sector
    test    ax,ax
    jne     .return

    ; Determine the offset within our buffer
    mov     ax,word [bp-8]                      ; logical sector
    shl     ax,9                                ; *512
    mov     dx,word [bp-4]                      ; byte offset
    sub     dx,ax                               ; offset within our buffer

    ; Take the value of the two bytes on our stack
    ;sub     bx,word [iFAT_BytesPerSector]
    add     bx,dx                               ; Points to our buffer
    mov     cx,word [ss:bx]                     ; This is the value we seek

    mov     ax,word [bp-6]
    test    ax,ax
    je      .even

.odd:
    shr     cx,4
    jmp     .result

.even:
    and     cx,0FFFh
    jmp     .result

.result:
    mov     word [ebp-2],cx
    pop     di
    pop     es
    popa
    mov     ax,word [ebp-2]
    mov     sp,bp
    pop     bp
    ret  

.return:
    pop     di
    pop     es
    popa
    mov     sp,bp
    pop     bp
    ret



;===============================================
; Searches for the FAT entry by its id.
;   In:
;     ax - The id of the entry to find.
;     es:di - The destination to copy the entry
;             information to. (sizeof == 32)
;   Out:
;     ax - Zero if found; otherwise, non-zero.
;===============================================
fat_getEntryById:
    push    bp
    mov     bp,sp
    sub     sp,8
    sub     sp,word [gs:iFAT_BytesPerSector]
    pusha
    push    si
    push    es                                  ; Original destination
    push    di

    cmp     ax,0
    jl      .error
    cmp     ax,word [gs:iFAT_MaxRootEntries]
    jge     .error

    mov     bx,ss
    mov     es,bx

    mov     cx,ax

    mov     ax,word [gs:iFAT_BytesPerSector]
    shr     ax,5
    mov     word [bp-6],ax                      ; WORD entriesPerSector

    xchg    ax,cx
    xor     dx,dx
    idiv    cx

    mov     word [bp-2],dx                      ; WORD offset
    mov     word [bp-4],ax                      ; WORD logicalSector

.loadSector:
    lea     cx,[bp]
    sub     cx,word [gs:iFAT_BytesPerSector]
    sub     cx,8
    mov     di,cx

    call    fat_loadRootSector
    test    ax,ax
    jne     .error

.copy:
    mov     ax,word [bp-2]
    shl     ax,5
    mov     si,ax
    add     si,di       ; Points to the requested entry

    pop     di
    pop     es

    push    ds
    push    bx
    mov     bx,ss
    mov     ds,bx
    mov     cx,32       ; [es:di] = dest
    rep movsb           ; [ds:si] = src
    pop     bx
    pop     ds

    pop     si
    popa
    xor     ax,ax
    mov     sp,bp
    pop     bp
    ret

.error:
    pop     di
    pop     es
    pop     si
    popa
    mov     ax,FERR_ENTRY_BY_ID
    mov     sp,bp
    pop     bp
    ret



;===============================================
; Searches for and copies the found entry.
;   In:
;     ds:si - The name of the entry.
;     es:di - The destination to copy the entry
;             information to. (sizeof == 32)
;   Out:
;     ax - Zero if found; otherwise, non-zero.
;===============================================
fat_searchEntry:
    push    bp
    mov     bp,sp
    sub     sp,8
    sub     sp,word [gs:iFAT_BytesPerSector]
    pusha
    push    es                                  ; Save the destination
    push    di
    mov     bx,ss
    mov     es,bx

    mov     ax,word [gs:iFAT_BytesPerSector]
    shr     ax,5
    mov     word [bp-6],ax                      ; WORD entriesPerSector
    
    call    fat_getSizeRoot
    mov     word [bp-2],ax                      ; WORD rootSize
    mov     word [bp-4],0                       ; WORD rootIndex

.loadSector:
    mov     word [bp-8],0                       ; WORD entriesIndex

    lea     ax,[bp]
    sub     ax,word [gs:iFAT_BytesPerSector]
    sub     ax,8
    mov     di,ax                               ; void * sector // Location to store the read sector

    mov     ax,word [bp-4]
    call    fat_loadRootSector
    test    ax,ax
    jne     .error

    lea     dx,[bp]
    sub     dx,word [gs:iFAT_BytesPerSector]
    sub     dx,8                                ; dx now points to the first entry of the loaded sector

.search:
    mov     cx,11                               ; name + extension
    mov     di,dx
    call    fat_strcmp
    test    ax,ax
    je      .copy

    add     dx,32                               ; 32-bytes per entry

    mov     ax,word [bp-8]
    add     ax,1
    cmp     ax,word [bp-6]
    mov     word [bp-8],ax                      ; entriesIndex < entriesPerSector
    jb      .search

    mov     ax,word [bp-4]
    add     ax,1
    cmp     ax,word [bp-2]                      ; rootIndex < rootSize
    mov     word [bp-4],ax
    jb      .loadSector

.error:
    mov     word [bp-2],ax
    pop     di
    pop     es
    popa
    mov     ax,word [bp-2]
    ;mov     ax,1                                ; return 1
    mov     sp,bp
    pop     bp
    ret

.copy:
    mov     ax,di

    pop     di                                  ; Restore the original destination
    pop     es

    push    si
    mov     si,ax                               ; start of entry
    mov     cx,32                               ; 32-bytes
    
    push    ds
    push    bx
    mov     bx,ss
    mov     ds,bx
    rep movsb
    pop     bx
    pop     ds

    pop     si

.return:
    
    popa
    xor     ax,ax                               ; return 0
    mov     sp,bp
    pop     bp
    ret



;===============================================
; Loads a FAT root or table sector
;   In:
;     ax - The offset within the root sector. [0, 14)
;     es:di - The destination to load the sector to.
;   Out:
;     ax - Zero if successful.
;===============================================
fat_loadRootSector:
    pusha
    mov     cx,ax
    call    fat_getLogicalRoot
    jmp     load_generic

fat_loadFatSector:
    pusha
    mov     cx,ax
    xor     ax,ax
    call    fat_getLogicalFat
    jmp     load_generic

load_generic:
    add     ax,cx
    call    fat_setRegisters

    ;mov     es,si                               NOTE: Should already be set properly!
    mov     bx,di

    mov     ah,2                                ; I/O Read
    mov     al,1                                ; Only 1 sector
    
    stc
    int     13h                                 ; I/O Read
    jc      .error0                             ; Hardware error
    cmp     al,1                                ; Insufficient number of sectors read
    jne     .error1
    ;jmp     .return

.return:
    popa
    xor     ax,ax
    ret

.error0:
    popa
    mov     ax,FERR_LOAD_GENERIC_0
    ret

.error1:
    popa
    mov     ax,FERR_LOAD_GENERIC_1
    ret


;===============================================
; Converts the input string into a valid FAT
; string, consisting of only letters and digits.
;   In:
;     ds:si - char * src
;     es:di - char * dest
;   Out:
;     N/A
;===============================================
fat_sanitize:
    push    si
    push    di

.loop:
    mov     al,byte [ds:si]
    cmp     al,0                                ; string-terminator
    je      .return
    
.stage1:                                        ; lowercase to uppercase
    cmp     al,61h
    jb      .stage2
    cmp     al,7Ah
    ja      .stage2
    sub     al,20h
    jmp     .valid

.stage2:                                        ; A-Z
    cmp     al,41h
    jb      .stage3
    cmp     al,5Ah
    jbe     .valid

.stage3:                                        ; 0-9
    cmp     al,30h
    jb      .continue
    cmp     al,39h
    ja      .continue
    ;jmp     .valid

.valid:
    mov     byte [es:di],al
    inc     di

.continue:
    inc     si
    jmp     .loop

.return:
    mov     byte [es:di],0                      ; Always appaend a string-terminator
    pop     di
    pop     si
    ret


;===============================================
; Compares two FAT name compliant strings.
;   In:
;     ds:si - char * str0
;     es:di - char * str1
;     cx - short num
;   Out:
;     ax - Zero if a match; otherwise, one.
;===============================================
fat_strcmp:
    push    di
    push    si
    push    cx

.loop:
    dec     cx

    mov     al,byte [ds:si]
    cmp     al,byte [es:di]     ; *s0 != *s1    ; <NOT EQUAL>
    jne     .mismatch

    cmp     al,0                ; *s0 == '\0'   ; <EQUAL> string terminator
    je      .match

    cmp     al,20h              ; *s0 == ' '    ; <EQUAL> end-of-string
    je      .match

    inc     di
    inc     si
    cmp     cx,0
    jne     .loop
    ;jmp    .match                              ; <EQUAL> no special characters

.match:
    xor     ax,ax
    jmp     .return

.mismatch:
    mov     ax,1
    
.return:
    pop     cx
    pop     si
    pop     di
    ret


;===============================================
; Returns a pointer to the right most occurence
; of the matching character within the string.
;   In:
;     ds:si - char * str
;     al - char character
;   Out:
;     ax - NULL if no match; otherwise, char*
;===============================================
fat_strrchr:
    push    si
    push    di
    xor     di,di

.loop:
    cmp     al,byte [ds:si]
    je      .match
    cmp     byte [ds:si],0
    je      .return
    add     si,1
    jmp     .loop

.match:
    mov     di,si
    jmp     .loop

.return:
    mov     ax,di
    pop     di
    pop     si
    ret




;===============================================
; UTILITY
;   All the utility functions preserve all
;   registers except for the return register ax.
;===============================================
fat_getSizeFats:                                ; short getSizeFats( void )
    push    dx
    movzx   ax,byte [gs:iFAT_NumberOfFats]
    imul    ax,word [gs:iFAT_SectorsPerFat]
    pop     dx
    ret


fat_getSizeRoot:                                ; short getSizeRoot( void )
    push    dx
    mov     ax,word [gs:iFAT_MaxRootEntries]
    shl     ax,5                                ; 32-bytes per entry
    xor     dx,dx
    div     word [gs:iFAT_BytesPerSector]
    pop     dx
    ret


fat_getLogicalFat:                              ; short getLogicalFat( short idxTable )
    imul    ax,word [gs:iFAT_SectorsPerFat]
    add     ax,word [gs:iFAT_ReservedSectors]
    ret


fat_getLogicalRoot:                             ; short getLogicalRoot( void )
    call    fat_getSizeFats
    add     ax,word [gs:iFAT_ReservedSectors]
    ret


fat_getLogicalData:                             ; short getLogicalData( void )
    push    bx
    mov     bx,word [gs:iFAT_ReservedSectors]
    call    fat_getSizeFats
    add     bx,ax
    call    fat_getSizeRoot         
    add     ax,bx
    pop     bx
    ret


fat_getStartFat:                                ; short fat_getStartFat( short idxTable )
    push    dx
    call    fat_getLogicalFat
    imul    ax,word [gs:iFAT_BytesPerSector]
    ;imul    ax,word [iFAT_SectorsPerCluster]
    pop     dx
    ret


fat_getStartRoot:                               ; short fat_getStartRoot( void )
    push    dx
    call    fat_getLogicalRoot
    imul    ax,word [gs:iFAT_BytesPerSector]
    ;imul    ax,word [iFAT_SectorsPerCluster]
    pop     dx
    ret


fat_getStartData:                               ; short fat_getStartData( void )
    push    dx
    call    fat_getLogicalData
    imul    ax,word [gs:iFAT_BytesPerSector]
    ;imul    ax,word [iFAT_SectorsPerCluster]
    pop     dx
    ret


fat_logicalToPhysical:
    push    cx
    mov     cx,ax
    call    fat_getLogicalData
    sub     ax,2
    add     ax,cx
    pop     cx
    ret


;===========
; FAT error messages
;===========
ferr_load0  db 'No matching name', 0
ferr_load1  db 'Load error', 0
ferr_load2  db 'Sector error', 0

ferr_fsize  db 'File size', 0

ferr_fread0 db 'I/O Error', 0
ferr_fread1 db 'I/O Sector', 0


;% ;include "src/kernel/fat_root.inc"
%include "src/kernel/fat_recluster.inc"

