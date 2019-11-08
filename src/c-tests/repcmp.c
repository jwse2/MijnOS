#include <stdio.h>

extern int repcmp(int count, char *str0, char *str1);

int main(int argc, char **argv)
{
    printf("%s / %s (%i)\n", "0", "1", repcmp(2, "0", "1"));
    printf("%s / %s (%i)\n", "2", "2", repcmp(2, "2", "2"));
    printf("%s / %s (%i)\n", "3", "4", repcmp(2, "3", "4"));

    return 0;
}

/*

ml -c -Cx _repcmp.asm
cl -c repcmp.c
link repcmp.obj _repcmp.obj /out:test.exe


*/
