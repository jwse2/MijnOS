#include <stdio.h>

extern int nzcmp(int a, int b);

int main(int argc, char **argv)
{
    printf("%i < %i (%i)\n", 0, 1, nzcmp(0, 1));
    printf("%i == %i (%i)\n", 2, 2, nzcmp(2, 2));
    printf("%i > %i (%i)\n", 3, 4, nzcmp(3, 4));

    return 0;
}

/*

ml -c -Cx _nzcmp.asm
cl -c nzcmp.c
link nzcmp.obj _nzcmp.obj /out:test.exe


*/
