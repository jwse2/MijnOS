[BITS 16]
; [ORG 0x15E00]
jmp main

%include "src\const.inc"

%define BLOCK_COUNT     12                      ; WARNING
%define BLOCK_WIDTH     80                      ;  DO NOT CHANGE
%define BLOCK_HEIGHT    24                      ;  THESE ARE HARDCODED
blck_state  times BLOCK_COUNT db 1              ; State of the blocks

%define PLAYER_WIDTH    28
%define PLAYER_HEIGHT   6
%define PLAYER_Y        (200-10)
%define PLAYER_COLOR    0Fh
plyr_pos    dw (160-(PLAYER_WIDTH/2))

%define BALL_WIDTH      4
%define BALL_HEIGHT     BALL_WIDTH
%define BALL_COLOR      01h
ball_pos_x  dw (160-(BALL_WIDTH/2))
ball_pos_y  dw (PLAYER_Y-(BALL_HEIGHT*2))
ball_ang_x  dw 0
ball_ang_y  dw 0

;===============================================
; Entry point
;===============================================
main:
    ; Switch to graphics mode
    mov     ax,INT_GPU_GRAPHICS
    int     70h

.game_loop:
    call    clear_buffer
    call    draw_blocks
    call    draw_player
    call    draw_ball

    ; DEBUG
    ;call    update_ball
    ;jmp     .game_loop

    ; TEMP - Keypress
    mov     ax,INT_KEYPRESS
    int     70h

    ; Update the balls position
    call    update_ball
    call    check_collision

    ; Quit the game?
    cmp     ax,KEY_ESCAPE
    je      .return

    ; Move left
    cmp     ax,KEY_LC_A
    je      .mv_left
    cmp     ax,KEY_UC_A
    je      .mv_left

    ; Move right
    cmp     ax,KEY_LC_D
    je      .mv_right
    cmp     ax,KEY_UC_D
    je      .mv_right

.continue:
    jmp     .game_loop

.mv_left:
    mov     ax,word [plyr_pos]
    sub     ax,1
    cmp     ax,1
    jb      .game_loop
    mov     word [plyr_pos],ax
    jmp     .game_loop

.mv_right:
    mov     ax,word [plyr_pos]
    add     ax,1
    cmp     ax,320-PLAYER_WIDTH
    jae     .game_loop
    mov     word [plyr_pos],ax
    jmp     .game_loop

.return:
    retf


;===============================================
; Updates the position of the ball
;===============================================
update_ball:
    pusha


    ; Always update the Y-axis first
.updateY:
    mov     ax,word [ball_ang_y]
    test    ax,ax
    jne     .up

.down:
    mov     cx,word [ball_pos_y]
    add     cx,1
    mov     word [ball_pos_y],cx

    ; Check if we need to invert the angle
    add     cx,BALL_HEIGHT
    cmp     cx,PLAYER_Y
    jb      .updateX

    ; Invert the Y-angle
    mov     word [ball_ang_y],1
    jmp     .updateX

.up:
    mov     cx,word [ball_pos_y]
    sub     cx,1
    mov     word [ball_pos_y],cx

    ; Check if we need to invert the angle
    cmp     cx,0
    ja      .updateX

    ; Invert the Y-angle
    mov     word [ball_ang_y],0
    jmp     .updateX


    ; Always update the X-axis second
.updateX:
    mov     ax,word [ball_ang_x]
    test    ax,ax
    je      .right

.left:
    mov     cx,word [ball_pos_x]
    sub     cx,1
    mov     word [ball_pos_x],cx

    ; Check if we need to invert the angle
    cmp     cx,0
    ja      .return

    ; Invert the X-angle
    mov     word [ball_ang_x],0
    jmp     .return

.right:
    mov     cx,word [ball_pos_x]
    add     cx,1
    mov     word [ball_pos_x],cx

    ; Check if we need to invert the angle
    add     cx,BALL_WIDTH
    cmp     cx,320
    jb      .return

    ; Invert the X-angle
    mov     word [ball_ang_x],1
    jmp     .return

.return:
    popa
    ret


;===============================================
; Checks for collision between block and ball.
;   In:
;     ax - The block index to check.
;===============================================
check_collision:
    push    bp
    mov     bp,sp
    pusha

    mov     ax,word [ball_pos_y]
    mov     cx,word [ball_pos_x]


.row_0:                                         ; y = 0
    cmp     ax,BLOCK_HEIGHT
    ja      .row_1

.r0c0:                                          ; y = 0 / x = 0
    cmp     cx,BLOCK_WIDTH
    ja      .r0c1
    mov     byte [blck_state+0],0
    jmp     .return

.r0c1:                                          ; y = 0 / x = 1
    cmp     cx,(BLOCK_WIDTH*2)
    ja      .r0c2
    mov     byte [blck_state+1],0
    jmp     .return

.r0c2:                                          ; y = 0 / x = 2
    cmp     cx,(BLOCK_WIDTH*2)
    ja      .r0c3
    mov     byte [blck_state+2],0
    jmp     .return

.r0c3:                                          ; y = 0 / x = 3
    cmp     cx,(BLOCK_WIDTH*3)
    ja      .return
    mov     byte [blck_state+3],0
    jmp     .return



.row_1:
    cmp     ax,(BLOCK_HEIGHT*2)
    ja      .row_2

.r1c0:                                          ; y = 1 / x = 0
    cmp     cx,BLOCK_WIDTH
    ja      .r1c1
    mov     byte [blck_state+4],0
    jmp     .return

.r1c1:                                          ; y = 1 / x = 1
    cmp     cx,(BLOCK_WIDTH*2)
    ja      .r1c2
    mov     byte [blck_state+5],0
    jmp     .return

.r1c2:                                          ; y = 1 / x = 2
    cmp     cx,(BLOCK_WIDTH*2)
    ja      .r1c3
    mov     byte [blck_state+6],0
    jmp     .return

.r1c3:                                          ; y = 1 / x = 3
    cmp     cx,(BLOCK_WIDTH*3)
    ja      .return
    mov     byte [blck_state+7],0
    jmp     .return



.row_2:
    cmp     ax,(BLOCK_HEIGHT*3)
    ja      .return     ; no collision

.r2c0:                                          ; y = 2 / x = 0
    cmp     cx,BLOCK_WIDTH
    ja      .r2c1
    mov     byte [blck_state+8],0
    jmp     .return

.r2c1:                                          ; y = 2 / x = 1
    cmp     cx,(BLOCK_WIDTH*2)
    ja      .r2c2
    mov     byte [blck_state+9],0
    jmp     .return

.r2c2:                                          ; y = 2 / x = 2
    cmp     cx,(BLOCK_WIDTH*2)
    ja      .r2c3
    mov     byte [blck_state+10],0
    jmp     .return

.r2c3:                                          ; y = 2 / x = 3
    cmp     cx,(BLOCK_WIDTH*3)
    ja      .return
    mov     byte [blck_state+11],0
    jmp     .return

    ; Get the state of the block
    ;mov     bx,ax
    ;movzx   ax,byte [blck_state+bx]
    ;test    ax,ax
    ;je      .return     ; Block is already deleted

.return:
    popa
    xor     ax,ax
    mov     sp,bp
    pop     bp
    ret


;===============================================
; Clears the draw buffer
;===============================================
clear_buffer:
    pusha
    push    es

    mov     bx,0A000h   ; Video memory
    mov     es,bx
    xor     cx,cx

.loop_0:                ; Y-axis
    push    cx       
    xor     di,di       ; [es:di]
    mov     al,00h      ; black
    mov     cx,320      ; 320 columns
    rep     stosb
    mov     bx,es       ; increment segment for next line
    add     bx,014h
    mov     es,bx
    pop     cx
    add     cx,1        ; check to see if there is a next line
    cmp     cx,200      ; 200 rows
    jb      .loop_0

.return:
    pop     es
    popa
    ret  


;===============================================
; Draws the player
;===============================================
draw_player:
    push    bp
    mov     bp,sp
    pusha
    push    es

    ; Calculate segment offset
    xor     dx,dx
    mov     ax,014h     ; 320-pixels
    mov     cx,PLAYER_Y
    mul     cx

    ; Segment to write to
    mov     bx,0A000h   ; Video memory
    add     bx,ax       ; Add the segment offset
    mov     es,bx

    xor     cx,cx

.loop:
    push    cx       
    mov     di,word [ds:plyr_pos]
    mov     al,PLAYER_COLOR
    mov     cx,PLAYER_WIDTH
    rep     stosb
    mov     bx,es       ; increment segment for next line
    add     bx,014h
    mov     es,bx
    pop     cx
    add     cx,1        ; check to see if there is a next line
    cmp     cx,PLAYER_HEIGHT
    jb      .loop

.return:
    pop     es
    popa
    mov     sp,bp
    pop     bp
    ret


;===============================================
; Draws the ball
;===============================================
draw_ball:
    push    bp
    mov     bp,sp
    pusha
    push    es

    ; Calculate segment offset
    xor     dx,dx
    mov     ax,014h     ; 320-pixels
    mov     cx,word [ds:ball_pos_y]
    mul     cx

    ; Segment to write to
    mov     bx,0A000h   ; Video memory
    add     bx,ax       ; Add the segment offset
    mov     es,bx

    xor     cx,cx

.loop:
    push    cx       
    mov     di,word [ds:ball_pos_x]
    mov     al,BALL_COLOR
    mov     cx,BALL_WIDTH
    rep     stosb
    mov     bx,es       ; increment segment for next line
    add     bx,014h
    mov     es,bx
    pop     cx
    add     cx,1        ; check to see if there is a next line
    cmp     cx,BALL_HEIGHT
    jb      .loop

.return:
    pop     es
    popa
    mov     sp,bp
    pop     bp
    ret


;===============================================
; Draws the blocks
;===============================================
draw_blocks:
    push    bp
    mov     bp,sp
    pusha

    mov     cx,BLOCK_COUNT
.loop:
    mov     bx,cx
    sub     bx,1
    movzx   ax,byte [blck_state+bx]
    test    ax,ax
    je      .continue

.draw:
    mov     ax,bx
    call    draw_single

.continue:
    sub     cx,1
    jne     .loop


.return:
    popa
    mov     sp,bp
    pop     bp
    ret




;===============================================
; Draws a single block.
;   In:
;     ax = index
;===============================================
draw_single:
    push    bp
    mov     bp,sp
    sub     sp,4        ; 0 = row, 1 = column
    sub     sp,4        ; 2 = y-offset, 3 = x-offset
    pusha

    ; Calculate xy-position of the block
    mov     cx,ax
    shr     cx,2                ; cx = row
    and     ax,3                ; ax = column
    mov     word [bp-2],cx      ; row
    mov     word [bp-4],ax      ; column

    ; Calculate the x-offset of the block
    xor     dx,dx
    mov     ax,word [bp-4]      ; column
    mov     cx,BLOCK_WIDTH
    mul     cx
    mov     word [bp-8],ax      ; x-offset

    ; Calculate the y-offset of the block
    xor     dx,dx
    mov     ax,word [bp-2]      ; row
    mov     cx,BLOCK_HEIGHT
    mul     cx
    mov     word [bp-6],ax      ; y-offset

; Draw the blocks
.draw:
    mov     dx,word [bp-6] ; y-pos
    add     dx,BLOCK_HEIGHT-2

.loop_0:
    mov     cx,word [bp-8] ; x-pos
    add     cx,BLOCK_WIDTH-2

.loop_1:
    mov     ax,INT_DRAW_PIXEL
    mov     bx,0Fh      ; Full white
    int     70h

    ; x-axis
    sub     cx,1
    cmp     cx,word [bp-8]
    ja      .loop_1

    ; y-axis
    sub     dx,1
    cmp     dx,word [bp-6]
    ja      .loop_0

.return:
    popa
    mov     sp,bp
    pop     bp
    ret



;===============================================
; GPU/VGA draw testing function.
;===============================================
draw_test:

    ; Fill the screen with a white color with
    ; the exception of the edges
    mov     dx,198      ; y
.loop_0:
    mov     cx,318      ; x
.loop_1:
    mov     ax,INT_DRAW_PIXEL
    mov     bx,0Fh      ; Full white
    int     70h

    sub     cx,1
    jne     .loop_1     ; x_loop

    sub     dx,1
    jne     .loop_0     ; y_loop

    ret
