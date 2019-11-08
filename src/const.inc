;===============================================
; SEGMENT LOCATIONS AND SIZES
;===============================================
%define SEG_BOOTLOADER              07C0h       ;   512 bytes
%define SEG_BOOTLOADER_STACK        07E0h       ;   512 bytes
%define SEG_FAT_TABLE               0800h       ;   512 bytes * 9
%define SEG_ROOT_DIRECTORY          0920h       ;   512 bytes * 14
%define SEG_KERNEL                  0AE0h       ; 16.384 bytes
%define SEG_KERNEL_STACK            0EE0h       ; 16.384 bytes
%define SEG_CMD                     12E0h       ;  4.096 bytes
%define SEG_CMD_STACK               13E0h       ;  4.096 bytes
%define SEG_PROGRAM_STACK           14E0h       ;  4.096 bytes
%define SEG_PROGRAM                 15E0h       ; variable, allows for lots of bytes incl. video memory

; NOTES:
;  - Bootloader is always at this address and 512-bytes in size
;  - The stack works in reverse but starts after the bootloader



;===============================================
; SYSTEM WIDE CONSTANTS
;===============================================
%define MAX_PATH    256


; ax values for int 70h
%define INT_RESERVED        0
%define INT_LOAD_FILE       1                   ; loads for execution, aka programs
%define INT_READ_FILE       6                   ; reads for random I/O, aka notepad
%define INT_WRITE_FILE      5                   ; all write operations
%define INT_EXEC_PROGRAM    2                   ; executes a loaded program
%define INT_GPU_GRAPHICS    3                   ; VGA / 16-colors / 320x200 pixels
%define INT_GPU_TEXT        4                   ; Text / 16-colors / 80x25 characters

%define INT_KEYPRESS        8
%define INT_GET_CURSOR_POS  9
%define INT_SET_CURSOR_POS  0Ah

%define INT_DRAW_PIXEL      0Dh
%define INT_DRAW_BUFFER     0Eh

%define INT_CLEAR_SCREEN    10h
%define INT_PRINT_STRING    11h
%define INT_PRINT_HEX       12h
%define INT_PRINT_CHAR      13h
%define INT_PRINT_NEWLINE   14h
%define INT_PRINTN_STRING   15h
%define INT_PRINT_COLORED   16h



; Key presses for get char
%define KEY_BACKSPACE   08h
%define KEY_TAB         09h
%define KEY_ENTER       0Dh
%define KEY_ESCAPE      1Bh

%define KEY_SPACE       20h
%define KEY_PERIOD      2Eh

; 0-9
%define KEY_0           30h
%define KEY_1           31h
%define KEY_2           32h
%define KEY_3           33h
%define KEY_4           34h
%define KEY_5           35h
%define KEY_6           36h
%define KEY_7           37h
%define KEY_8           38h
%define KEY_9           39h

; A-Z
%define KEY_UC_A        41h
%define KEY_UC_B        42h
%define KEY_UC_C        43h
%define KEY_UC_D        44h
%define KEY_UC_E        45h
%define KEY_UC_F        46h
%define KEY_UC_G        47h
%define KEY_UC_H        48h
%define KEY_UC_I        49h
%define KEY_UC_J        4Ah
%define KEY_UC_K        4Bh
%define KEY_UC_L        4Ch
%define KEY_UC_M        4Dh
%define KEY_UC_N        4Eh
%define KEY_UC_O        4Fh
%define KEY_UC_P        50h
%define KEY_UC_Q        51h
%define KEY_UC_R        52h
%define KEY_UC_S        53h
%define KEY_UC_T        54h
%define KEY_UC_U        55h
%define KEY_UC_V        56h
%define KEY_UC_W        57h
%define KEY_UC_X        58h
%define KEY_UC_Y        59h
%define KEY_UC_Z        5Ah

; a-z
%define KEY_LC_A        61h
%define KEY_LC_B        62h
%define KEY_LC_C        63h
%define KEY_LC_D        64h
%define KEY_LC_E        65h
%define KEY_LC_F        66h
%define KEY_LC_G        67h
%define KEY_LC_H        68h
%define KEY_LC_I        69h
%define KEY_LC_J        6Ah
%define KEY_LC_K        6Bh
%define KEY_LC_L        6Ch
%define KEY_LC_M        6Dh
%define KEY_LC_N        6Eh
%define KEY_LC_O        6Fh
%define KEY_LC_P        70h
%define KEY_LC_Q        71h
%define KEY_LC_R        72h
%define KEY_LC_S        73h
%define KEY_LC_T        74h
%define KEY_LC_U        75h
%define KEY_LC_V        76h
%define KEY_LC_W        77h
%define KEY_LC_X        78h
%define KEY_LC_Y        79h
%define KEY_LC_Z        7Ah
