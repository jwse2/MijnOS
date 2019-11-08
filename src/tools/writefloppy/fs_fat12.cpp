#include <cstdio>
#include "fs_fat12.hpp"
#include "globals.hpp"
#include "io.hpp"


/**
 * The FAT table plays a central role. Therefor it is handy to have it set as
 * global variable instead of using it in the function.
 */
unsigned char fat_Table[512 * 9]; // bytes_per_sector * sectors_per_fat


/** For a description see the header file. */
int FAT12_CopyData(FILE *oFile)
{
    size_t size;
    FILE *iFile;


    return 0;
}

/** Trim the filename to valid FAT12 8.3 filenames. */
void FAT12_TrimFileName(char filename[MAX_PATH])
{
    // Ensure zero-termination, just in case
    filename[MAX_PATH-1] = '\0';

    // Trim from the front to the back. (We do not use VFAT!)
    for (int i = 0; i < MAX_PATH; i++)
    {
        // Abort immediately if this is the end of the string
        if (filename[i] == '\0')
        {
            break;
        }

        // Valid character ranges should be copied
        if (filename[i] >= 'A' && filename[i] <= 'Z')
        {
            continue;
        }
        if (filename[i] >= '0' && filename[i] <= '9')
        {
            continue;
        }

        // Convert to UPPERCASE
        if (filename[i] >= 'a' && filename[i] <= 'z')
        {
            filename[i] -= ('a' - 'A');
            continue;
        }

        // Individual characters
        if (filename[i] == ' ') continue;
        if (filename[i] == '!') continue;
        if (filename[i] == '#') continue;
        if (filename[i] == '$') continue;
        if (filename[i] == '%') continue;
        if (filename[i] == '&') continue;
        if (filename[i] == '\'') continue;
        if (filename[i] == '(') continue;
        if (filename[i] == ')') continue;
        if (filename[i] == '-') continue;
        if (filename[i] == '@') continue;
        if (filename[i] == '^') continue;
        if (filename[i] == '_') continue;
        if (filename[i] == '`') continue;
        if (filename[i] == '{') continue;
        if (filename[i] == '}') continue;
        if (filename[i] == '~') continue;

        // Otherwise we have to copy the next characters to this position
        for (int j = i+1; j < MAX_PATH; j++)
        {
            filename[j-1] = filename[j];
        }

        // At this position is now the character that used to be behind it
        // as such we need to check the same index once more.
        i--;
    }
}
