;===============================================
; INTERRUPT TABLE
;===============================================
ivt_base:
    dq 0    ; Not sure if needed for IVT

ivt_start:
    .int_01:
        dd  00000h
        dd  00000h
    .int_02:
        dd  00000h
        dd  00000h
ivt_end:

ivt_size    dw  ivt_end-ivt_start
ivt_ptr     dw  ivt_base
