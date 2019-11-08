; MASM x86
.MODEL FLAT,C

.CONST

.CODE
public repcmp

repcmp:
    push    ebp
    mov     ebp,esp
    sub     esp,4
    push    edi
    push    esi

    mov     esi,dword ptr [ebp+0Ch]
    mov     edi,dword ptr [ebp+10h]
    mov     ecx,dword ptr [ebp+8]

    xor     eax,eax
    repe cmpsb
    setne   al

return:
    pop     esi
    pop     edi
    mov     esp,ebp
    pop     ebp
    ret

END
