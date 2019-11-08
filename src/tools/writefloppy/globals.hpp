#ifndef GLOBALS_HPP
#define GLOBALS_HPP

#include <queue>

#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>

#define BUFFER_SIZE     512         // Sectors are supposed to be512-bytes so
                                    // use this for the buffer size.
#define FLOPPY_SIZE     1474560     // The size of a 3.5" 1.44MB floppy disk.

#define ATTRIB_NONE         0   // No attributes have to be set
#define ATTRIB_HIDDEN       1   // The file has to be hidden
#define ATTRIB_READONLY     2   // The file is read-only
#define ATTRIB_SYSTEM       4   // The file is a system file

#define FS_RAW      0   // Use no filesystem; aligns files to 512-bytes
#define FS_FAT12    1   // Use the FAT12 filesystem; values should be taken from
                        // the bootloader file for a correct output format.

struct FileArg
{
    int attributes;     // Contains the file's desired attributes.
    char *filename;     // A pointer to the file's name.

    FileArg(void)
    {
        attributes = ATTRIB_NONE;
        filename = nullptr;
    }

    FileArg(int a, char *p)
    {
        attributes = a;
        filename = p;
    }
};

/** Global variables */
extern std::queue<FileArg> g_queue; // Contains all the input files.
extern char g_outputFile[MAX_PATH]; // Contains the output filename.
extern char *g_bootloader;          // Contains a pointer to the bootloader's
                                    // filename, if not set the first file in
                                    // the queue will be taken.
extern int  g_filesystem;           // The filesystem to use in the output file.

#endif //GLOBALS_HPP
