[BITS 16]
    jmp near bootstrap

%include "src/const.inc"

; BPB
OEMId                   db 'MijnOS_0'           ; OEM identifier
BytesPerSector          dw 512                  ; Number of bytes per sector
SectorsPerCluster       db 1                    ; Number of sectors per cluster
ReservedSectors         dw 1                    ; Number of reserved sectors
NumberOfFats            db 2                    ; Number of FAT tables
MaxRootEntries          dw 224                  ; Maximum number of root directories
SmallSectors            dw 2880                 ; Total sector count (For FAT16 and older)
MediaDescriptor         db 0F0h                 ; 3.5" 1.44MB Floppy
SectorsPerFat           dw 9                    ; Sectors per FAT
SectorsPerTrack         dw 18                   ; Sectors per track
NumberOfHeads           dw 2                    ; Number of heads
HiddenSectors           dd 0                    ; Number of hidden sectors
LargeSectors            dd 0                    ; Total sector count (For FAT32 and newer)

; Extended BPB
DriveNo                 db 0                    ; Physical drive number
Reserved                db 0                    ; Reserved
BootSignature           db 029h                 ; Boot signature, indicates the presence of the following three fields
VolumeId                dd 22352E33h            ; Volume id
VolumeLabel             db 'NO NAME    '        ; Volume label
FileSystemType          db 'FAT12   '           ; File system type



;===========
; BOOTSTRAP (448-bytes)
;===========
bootstrap:
    mov     ax,SEG_BOOTLOADER
    mov     ds,ax                               ; Set data segment to where we're loaded
    mov     es,ax
    mov     ax,SEG_BOOTLOADER_STACK             ; Skip over the bootloader
    mov     ss,ax
    mov     sp,200h                             ; Set up a 512 bytes stack after the bootloader

    call    load_fat                            ; Loads the FAT table into memory
    call    load_root                           ; Loads the root directory into memory, and immediately searches for the kernel
    ;call    load_kernel                         ; Loads the kernel into memory

    jmp     $                                   ; If the kernel falls through, the system will hang

reboot:
    mov     si,str_error
    call    print_string
    mov     ax,word [dbg_error]
    call    print_hex
    ;mov     ax,0
    ;int     19h
    jmp     $



;===============================================
; Sets the registers necessary for loading from disk.
;   In: ax
;   Out: ax, ch, cl, dh, cl
;===============================================
setLoadRegisters:
    push    bx
    push    ax
    mov     bx,ax                               ; Preserve the logical sector number

    mov     dx,0                                ; First sector
    div     word [SectorsPerTrack]              ; edx = eax MOD SectorsPerTrack / eax = eax DIV SectorsPerTrack
    add     dl,1                                ; Physical sectors start with 1
    mov     cl,dl                               ; int 13h uses cl for sectors

    mov     ax,bx
    mov     dx,0
    div     word [SectorsPerTrack]
    mov     dx,0
    div     word [NumberOfHeads]
    mov     dh,dl
    mov     ch,al

    pop     ax
    pop     bx
    mov     dl,byte [DriveNo]
    ret



;===============================================
; Loads the FAT table into memory.
;===============================================
load_fat:
    mov     ax,1                                ; Sector 1 is the 1st FAT table
    call    setLoadRegisters

    ; [ES:BX]
    mov     si,SEG_FAT_TABLE                    ; Set the pointer to the FAT table buffer
    mov     es,si
    xor     bx,bx

    mov     ah,2                                ; I/O Read
    mov     al,9                                ; FAT table consists of 9-sectors

    stc
    int     13h                                 ; Read the sectors into the buffer
    mov     word [dbg_error],1
    jc      reboot                              ; An error occured
    cmp     al,9
    mov     word [dbg_error],2
    jne     reboot                              ; Not enough sectors were read
    ret



;===============================================
; Loads the root directory into memory.
;===============================================
load_root:
    mov     ax,19                               ; Sector 19 is the start of the root directory
    call    setLoadRegisters

    ; [ES:BX]
    mov     si,SEG_ROOT_DIRECTORY               ; Set the pointer to the buffer
    mov     es,si
    xor     bx,bx

    mov     ah,2                                ; I/O Read
    mov     al,14                               ; The root consists of 14-sectors

    stc
    int     13h                                 ; Read the sectors into memory
    mov     word [dbg_error],3
    jc      reboot                              ; An error occured
    cmp     al,14
    mov     word [dbg_error],4
    jne     reboot                              ; Not enough sectors were read
    jmp     search_kernel



;===============================================
; Search through the root directory.
;===============================================
search_kernel:
    xor     bx,bx
    mov     cx,word [MaxRootEntries]

.loop:
    xchg    cx,dx

    mov     si,kernel_name
    mov     di,bx
    mov     cx,11
    rep     cmpsb                               ; [DS:SI] [ES:DI]
    je      .found
    add     bx,32

    xchg    dx,cx
    loop    .loop

.not_found:
    mov     word [dbg_error],5
    jmp     reboot

.found:
    mov     ax,word [es:bx+1Ah]                 ; Logical sector ID
    mov     word [kernel_cluster],ax            ; Store the cluster number
    jmp     load_file_sector



;===============================================
; Loads the specific sector of a file.
;===============================================
load_file_sector:
    mov     ax,word [kernel_cluster]
    add     ax,31                               ; This is FAT12 specific
    call    setLoadRegisters

    mov     si,SEG_KERNEL
    mov     es,si
    mov     bx,word [kernel_pointer]            ; Current offset in the kernel segment to write to

    mov     ah,2
    mov     al,1

    stc
    int     13h
    mov     word [dbg_error],6
    jc      reboot
    cmp     al,1
    mov     word [dbg_error],7
    jne     reboot
    jmp     load_next_cluster


;===============================================
; Tries to load the next cluster of the kernel.
;===============================================
load_next_cluster:
    mov     ax,word [kernel_cluster]            ; Take the logical id of the cluster that has been read
    mov     dx,0
    mov     bx,3
    mul     bx                                  ; dx:ax = ax * bx
    mov     bx,2                                ; dx = (ax % bx)
    div     bx                                  ; ax = (ax / bx)

    mov     si,SEG_FAT_TABLE                    ; The next cluster info resides in the FAT table
    mov     es,si
    mov     si,ax                               ; The offset has been calculated
    mov     ax,word [es:si]                     ; Take the next cluster information

    test    dx,dx
    je      .even

.odd:
    shr     ax,4
    jmp     .continue

.even:
    and     ax,0FFFh

.continue:
    mov     word [kernel_cluster],ax            ; Store the logical id of the next cluster

    cmp     ax,0FF8h                            ; 0FF8h and higher indicate end-of-file
    jae     .boot_kernel

    add     word [kernel_pointer],512           ; Sectors are 512 bytes in size, so read
    jmp     load_file_sector                    ;  the next one behind the current one


.boot_kernel:                                   ; Jumps to the kernel code
    mov     word [dbg_error],8
    mov     si,SEG_KERNEL
    mov     es,si
    xor     bx,bx
    movzx   ax,byte [es:bx]
    cmp     ax,0E9h                             ; If not it has been overwritten or wrongly read
    jne     reboot
    jmp     SEG_KERNEL:0000h



;===============================================
; Prints a random string to the screen.
;===============================================
print_string:
    pusha
    mov     ah,0Eh

.loop:
    lodsb
    cmp     al,0
    je      .done
    int     10h
    jmp     .loop

.done:
    popa
    ret



;===============================================
; Prints a HEX byte onto the screen.
;===============================================
print_hex:
    pusha

    mov     cx,2
    mov     bx,16                               ; Divide the input value by 16
    mov     dx,0
    div     bx

.loop:
    cmp     al,10
    jl      .low
    add     ax,07h                              ; 41h - 30h - 0Ah = 07h

.low:
    add     ax,30h
    mov     ah,0Eh
    int     10h

    mov     ax,dx
    loop    .loop

.done:
    popa
    ret



;===========
; VARIABLES
;===========
kernel_cluster          dw -1                   ; Current targeted sector to load for the kernel
kernel_pointer          dw 0                    ; Current offset within the segment
kernel_name             db 'KERNEL  BIN'        ; Name of the kernel file as saved on the FAT12 volume

; DEBUG
str_error               db 'Error ',0
str_jump                db 'JMP',0
dbg_error               dw 0FFFFh


;===========
; BOOT SIG (2-bytes)
;===========
times 510-($-$$) db 0
sign dw 0AA55h
