; MASM x86
.MODEL FLAT,C

.CONST

.CODE
public nzcmp

nzcmp:
    push    ebp
    mov     ebp,esp
    sub     esp,4

    xor     eax,eax
    mov     ecx,dword ptr [ebp+8]
    mov     edx,dword ptr [ebp+0Ch]
    cmp     ecx,edx
    setnz   al

return:
    mov     esp,ebp
    pop     ebp
    ret

END
