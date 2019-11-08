#include <cstddef>
#include <cstdio>
#include <cassert>
#include "utility.hpp"
#include "globals.hpp"
#include "io.hpp"

FILE* FS_OpenBootloader(size_t *size)
{
    long int sz;
    FILE *iFile;
    errno_t err;

    if (!g_bootloader)
    {
        fprintf(stderr, "I/O Error: Bootloader file not set.\n");
        return NULL;
    }

    // Open the bootloader file
    err = fopen_s(&iFile, g_bootloader, "rb");
    if (err)
    {
        fprintf(stderr, "I/O Error: Could not open bootloader file '%s' for reading.\n", g_bootloader);
        return NULL;
    }

    // Determine the file's size
    sz = fsize(iFile);
    if (sz <= 0)
    {
        fprintf(stderr, "I/O ERROR: Incorrect file size of %li\n", sz);
        fclose(iFile);
        return NULL;
    }
    *size = static_cast<size_t>(sz);

    assert(iFile != NULL);

    return iFile;
}

FILE* FS_OpenNextFile(size_t *size, char **filename)
{
    long int sz;
    FILE *iFile;
    FileArg arg;
    errno_t err;

    // There are no files left
    if (g_queue.empty())
    {
        return NULL;
    }

    // Take the next file from the queue and remove it for the next itteration
    arg = g_queue.front();
    g_queue.pop();

    // Open the file
    err = fopen_s(&iFile, arg.filename, "rb");
    if (err)
    {
        fprintf(stderr, "I/O Error: Could not open '%s' for reading.\n", arg.filename);
        return NULL;
    }

    // Determine the file's size
    sz = fsize(iFile);
    if (sz <= 0)
    {
        fprintf(stderr, "I/O ERROR: Incorrect file size of %li\n", sz);
        return NULL;
    }
    *size = static_cast<size_t>(sz);

    // Only set the pointer if desired
    if (filename)
    {
        *filename = arg.filename;
    }

    return iFile;
}
