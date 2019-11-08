/**
 * WRITEFLOPPY
 *   Write floppy is a program written for the minor MijnOS. It allows one to
 *   write files, including the boot sector, to a virtual floppy image. This is
 *   all done with the FAT12 filesystem format.
 *
 * SPECIFICATIONS
 *   The write floppy program assumes the following key specifications:
 *     - 3.5" floppy disk
 *     - 1.44 MB (1440 KB) of disk space
 *     - 2880 total sectors
 *
 * REMARKS
 *   The application assumes a little-endian machine is used.
 */
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <queue>

#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>

typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned int dword;
typedef unsigned long long int qword;

const bool VERBOSE = false;

#define CINFO(fmt, ...)     fprintf(stdout, fmt"\n", ##__VA_ARGS__)
#define CWARN(fmt, ...)     fprintf(stdout, "WARNING: "fmt"\n", ##__VA_ARGS__)
#define CERROR(fmt, ...)    fprintf(stderr, "ERROR: "fmt"\n", ##__VA_ARGS__)
#define CVERBOSE(fmt, ...)  if (VERBOSE) { fprintf(stdout, "[VERBOSE] "fmt"\n", ##__VA_ARGS__); }
#define CASSERT(ex, fmt, ...) \
    if (!(ex)) { \
        fprintf(stderr, "ASSERTION FAILURE\n\t%s @ %i\n\t"#ex"\n\t"fmt"\n", __FILE__, __LINE__, ##__VA_ARGS__); \
        exit(EXIT_FAILURE); \
    }

#include "settings.inc"


/**
 * DIRECTORY
 *   Defines a single directory object.
 */
typedef struct Entry
{
    char            name[8];            /* The name of the entry */
    char            extension[3];       /* The extension of the entry */
    unsigned char   attributes;         /* The attributes of the entry */
    char            reserved[2];        /* Reserved for future use */
    short           creationTime;       /* Time at which the entry was created */
    short           creationDate;       /* Date at which the entry was created */
    short           lastAccessDate;     /* Date at which the entry was last accessed */
    char            ignore[2];          /* Ignore for FAT12 */
    short           lastWriteTime;      /* Time at which the last write occured */
    short           lastWriteDate;      /* Date at which the last write occured */
    short           firstLogicalSector; /* First logical sector of the entry */
    int             size;               /* The size of the entry */
} entry_t;


// FAT entry types for next sector
#define FAT_TYPE_UNUSED             ((char)(0x000))
#define FAT_TYPE_RESERVED           ((char)(0xFF0))
#define FAT_TYPE_BAD_CLUSTER        ((char)(0xFF7))
#define FAT_TYPE_LAST_OF_FILE       ((char)(0xFFF))
#define FAT_TYPE_VAR(var)           ((char)(var))

// File Attribute Flags
#define FAT_ATTRIB_READ_ONLY        ((char)(0x01))
#define FAT_ATTRIB_HIDDEN           ((char)(0x02))
#define FAT_ATTRIB_SYSTEM           ((char)(0x04))
#define FAT_ATTRIB_VOLUME_LABEL     ((char)(0x08))
#define FAT_ATTRIB_SUBDIRECTORY     ((char)(0x10))
#define FAT_ATTRIB_ARCHIVE          ((char)(0x20))
#define FAT_ATTRIB_LONGNAME         ((char)(0x0F))

// File name flags
#define FAT_FLAG_EMPTY              ((char)(0x00))  /* Indicates the entry is unused */
#define FAT_FLAG_ERASED             ((char)(0xE5))  /* This indicates the entry was erased */
#define FAT_FLAG_SPECIAL            ((char)(0x2E))  /* This denotes the current directory */
#define FAT_FLAG_SWAPPED            ((char)(0x05))  /* The actual character is 0xE5 */


/**
 * DATA
 *   Contains all the date of the image.
 */
char g_data[DEVICE_SIZE];


/**
 * FAT TABLE
 *   Points to the first FAT table within the image data.
 */
char *fat_table;


/**
 * FAT DATA
 *   Points to the first physical sector within the image data.
 */
char *fat_data;


/**
 * DYNAMIC SETTINGS
 *   These values can change depending on the user input.
 */
char g_bootSector[MAX_PATH];        /* The filename of the boot sector file */
char g_workingDirectory[MAX_PATH];  /* The path to the working directory */
char g_pathOut[MAX_PATH];           /* The filename of the output file */
char g_pathIn[MAX_PATH];            /* The filename of the input file */
bool g_strip;                       /* Strip the input volume of unnecessary data */
bool g_format;                      /* Formats the input volume, except the boot sector */
bool g_defragment;                  /* Defragments the input volume */
std::queue<char*> g_queue;          /* All the input file names */


#define FILE_BUFFER_SIZE  0x11000      /* Increase if necessary */
char g_bufferFileData[FILE_BUFFER_SIZE];    /* Allocations exceeed max */


/**
 * FORWARD DECLARATIONS
 *   Operations are forward declared and reside at the end of the file. This
 *   allows for a clearer seperation between functions.
 */
int OP_strip(void);
int OP_format(void);
int OP_defragment(void);
int OP_addFile(FILE *file, const char *path);
int OP_searchFile(const char * name, void ** dest);
int OP_convertToFileName(const char * in, char name[8], char ext[3]);


/**
 * Sets the default values for the dynamic settings.
 */
void setDefaults(void)
{
    // Clear paths
    memset(g_bootSector, 0, MAX_PATH);
    memset(g_workingDirectory, 0, MAX_PATH);
    memset(g_pathIn, 0, MAX_PATH);

    // The output path always needs a default name
    strncpy_s(g_pathOut, MAX_PATH, "default.flp", _TRUNCATE);

    // Boolean settings
    g_strip = false;
    g_format = false;
    g_defragment = false;

    // Clear the buffer
    memset(g_data, 0, DEVICE_SIZE);

    // FAT references within the data buffer
    fat_table = (g_data + BYTES_PER_CLUSTER);
    fat_data = (g_data + BYTES_PER_CLUSTER + (FAT_SIZE * NUMBER_OF_FAT));
}


/**
 * Processes the arguments passed to the application.
 * @param argc The number of arguments available.
 * @param argv The array containing the arguments
 * @return TRUE when successful; otherwise, FALSE.
 */   
#define VALUE_CHECK(arg)      if (++i >= argc) { CWARN("Argument %s has no value specified.", arg); continue; }
bool procArguments(int argc, char **argv)
{
    int i;
    char *arg;

    for (i = 0; i < argc; i++)
    {
        // Quickly seperate arguments from file names
        if (argv[i][0] == '-' || argv[i][0] == '/')
        {
            arg = (argv[i] + 1);

            if (arg[0] == 'b')
            {
                VALUE_CHECK("-b");
                strncpy_s(g_bootSector, MAX_PATH, argv[i], _TRUNCATE);
                CVERBOSE("Set boot sector file to '%s'", g_bootSector);
            }
            else if (arg[0] == 'w')
            {
                VALUE_CHECK("-w");
                strncpy_s(g_workingDirectory, MAX_PATH, argv[i], _TRUNCATE);
                CVERBOSE("Set working directory to '%s'", g_workingDirectory);
            }
            else if (arg[0] == 'o')
            {
                VALUE_CHECK("-o");
                strncpy_s(g_pathOut, MAX_PATH, argv[i], _TRUNCATE);
                CVERBOSE("Set output file to '%s'", g_pathOut);
            }
            else if (arg[0] == 'i')
            {
                VALUE_CHECK("-i");
                strncpy_s(g_pathIn, MAX_PATH, argv[i], _TRUNCATE);
                CVERBOSE("Set input file to '%s'", g_pathIn);
            }
            else if (arg[0] == 's')
            {
                g_strip = true;
                CVERBOSE("Stipping mode is enabled.");
            }
            else if (arg[0] == 'd')
            {
                g_defragment = true;
                CVERBOSE("Defragmentation mode is enabled.");
            }
            else if (arg[0] == 'f')
            {
                g_format = true;
                CVERBOSE("Formatting mode is enabled.");
            }
            else
            {
                // Always assume errouness operations occur when the input is
                // not what we expected of the user.
                CWARN("Uknown argument '-%s'", arg);
                return false;
            }
        }
        else
        {
            CVERBOSE("Adding '%s' to the queue...", argv[i]);
            g_queue.push(argv[i]);
        }
    }

    return true;
}


void printUsage(char *lpExeName)
{
/**
 * OPTIONS
 *   -w <path>  Changes the working directory.
 *   -o <path>  Set the output file.
 *   -i <path>  Uses an exisiting FAT12 converted image as input file.
 *   -b <path>  Overrides the bootsector with the specified file. Has to be exactly 512-bytes.
 *   -s         Strips the volume of unused data, like long file names.
 *   -d         Defragments the inputted image for the output.
 *   -f         Formats the inputted image but keeps the bootsector.
 *
 * FILE ATTRIBUTES
 *   H  Marks the file as hidden.
 *   R  Marks the file as read-only.
 *   S  Marks the file as a system file.
 */

    printf("\nUSAGE: %s <options> file <additional files>\n", lpExeName);
    printf("\nEXAMPLE: %s -w .\\bin -o floppy.flp -i floppy.bak -b boot.bin -s -d HRS:kernel.bin arkanoid.bin\n", lpExeName);

#define OPTION_EXT(arg0, arg1, text) \
    printf("  %-2s %-8s %s\n", arg0, arg1, text)

#define OPTION(arg0, text) \
    printf("  %-11s %s\n", arg0, text)

    // Options
    printf("\nOPTIONS\n");
    OPTION_EXT("-w", "<path>", "Changes the working directory.");
    OPTION_EXT("-o", "<path>", "Changes the output file.");
    OPTION_EXT("-i", "<path>", "Use an existing FAT12 converted 1.44MB floppy image.");
    OPTION_EXT("-b", "<path>", "Override the bootsector with the specified file. (File must be exactly 512-bytes.)");
    OPTION("-s", "Requires -i. Strips the volume of unused data, like long filenames.");
    OPTION("-d", "Requires -i. Defragments the volume.");
    OPTION("-f", "Requires -i. Formats the volume, but keeps the bootsector as is.");

#define ATTRIBUTE(arg0, text) \
    printf("  %-3s %s\n", arg0, text)

    // Attributes
    printf("\nFILE ATTRIBUTES\n");
    ATTRIBUTE("H", "Hidden");
    ATTRIBUTE("R", "Read-Only");
    ATTRIBUTE("S", "System");
}


/**
 * Initializes the program for further operations.
 * @param argc The number of arguments available.
 * @param argv The array containing the arguments.
 * @return Zero if successful; otherwise, a non-zero value.
 */
int initialize(int argc, char **argv)
{
    CINFO("Initializing...");

    if (argc < 3)
    {
        printUsage(argv[0]);
        return -1;
    }

    // First set all the settings to their defaults
    setDefaults();

    // Process all the user arguments
    if (!procArguments(argc - 1, argv + 1))
    {
        //CERROR("Could not process the passed arguments.");
        return -2;
    }

    // Change the working directory if specified
    if (g_workingDirectory[0] != '\0')
    {
        CVERBOSE("Changing working directory to '%s'", g_workingDirectory);

        if (!SetCurrentDirectoryA(g_workingDirectory))
        {
            CERROR("Could not change the working directory to '%s'. (%i)",
                g_workingDirectory, GetLastError());
            return -3;
        }
    }

    // Mutually exclusive settings
    CASSERT(!g_format || (g_format && !(g_strip || g_defragment)),
        "Conflicting settings detected; format is mutually exclusive with stripping and/or defragmentation.");

    return 0;
}


/**
 * Reads the file into the destination.
 * @param path The path to the file.
 * @param dest The destination to read the file to.
 * @param n The number of bytes to read.
 * @return Zero if successful; otherwise, a non-zero value.
 */
int readFile(const char *path, char *dest, int n)
{
    FILE *file;
    errno_t err;
    size_t read;

    err = fopen_s(&file, path, "rb");
    if (err)
    {
        CERROR("Could not open file '%s' for reading. (%d)", path, err);
        return -1;
    }

    read = fread(dest, 1, DEVICE_SIZE, file);
    CASSERT(ferror(file) == 0, "An error occured whilst reading from '%s'", path);
    fclose(file);

    if (n != -1 && n != static_cast<int>(read))
    {
        return -2;
    }

    return 0;
}


/**
 * Processes the loaded FAT data in correspendence to the settings.
 * @return Zero if successful; otherwise, a non-zero value.
 */
int FAT_proc(void)
{
    int result;

    if (g_format)
    {
        result = OP_format();
        if (result)
        {
            return result;
        }
    }

    if (g_strip)
    {
        result = OP_strip();
        if (result)
        {
            return result * 8;
        }
    }

    if (g_defragment)
    {
        result = OP_defragment();
        if (result)
        {
            return result * 16;
        }
    }

    return 0;
}


/**
 * Verifies that the data in the bootsector matches with that of the application.
 * @return Zero if successful; otherwise, a non-zero value.
 */
int FAT_verify(void)
{
    CVERBOSE("Verifying FAT boot sector...");

    // Skip the OEM
    if (*(reinterpret_cast<word *>(g_data + 11)) != BYTES_PER_SECTOR)        return -1;
    if (*(reinterpret_cast<byte *>(g_data + 13)) != SECTORS_PER_CLUSTER)     return -2;
    if (*(reinterpret_cast<word *>(g_data + 14)) != RESERVED_CLUSTERS)       return -3;
    if (*(reinterpret_cast<byte *>(g_data + 16)) != NUMBER_OF_FAT)           return -4;
    if (*(reinterpret_cast<word *>(g_data + 17)) != MAX_ROOT_DIRECTORIES)    return -5;
    if (*(reinterpret_cast<word *>(g_data + 19)) != TOTAL_SECTORS_FAT16)     return -6;
    if (*(reinterpret_cast<byte *>(g_data + 21)) != DEVICE_TYPE)             return -7;
    if (*(reinterpret_cast<word *>(g_data + 22)) != SECTORS_PER_FAT)         return -8;
    if (*(reinterpret_cast<word *>(g_data + 24)) != SECTORS_PER_TRACK)       return -9;
    if (*(reinterpret_cast<word *>(g_data + 26)) != NUMBER_OF_HEADS)         return -10;
    if (*(reinterpret_cast<dword*>(g_data + 28)) != NUMBER_OF_HIDDEN_SEC)    return -11;
    if (*(reinterpret_cast<dword*>(g_data + 32)) != TOTAL_SECTORS_FAT32)     return -12;
    if (*(reinterpret_cast<byte *>(g_data + 36)) != PHYSICAL_DRIVE_NUM)      return -13;
    if (*(reinterpret_cast<byte *>(g_data + 37)) != RESERVED_VALUE)          return -14;
    if (*(reinterpret_cast<byte *>(g_data + 38)) != BOOT_SIGNATURE)          return -15;
    // Skip volume id
    if (memcmp("NO NAME    ", (g_data + 43), 11))   return -16;
    if (memcmp("FAT12   ", (g_data + 54), 8))       return -17;

    return 0;
}


/**
 * Loads a FAT12 image file.
 * @param file The file to load from.
 * @return Zero if successful; otherwise, a non-zero value.
 */
int FAT_load(FILE *file)
{
    int result;
    size_t read;

    // Read the entire file
    read = fread(g_data, 1, DEVICE_SIZE, file);
    CASSERT(ferror(file) == 0, "An error occured whilst reading data.");

    // Ensure the entire file was read
    if (read != DEVICE_SIZE)
    {
        CERROR("Could not read entire file. File exceeds the maximum size.");
        return -3;
    }

    // FATs generally contain two tables, we can verify the first with the
    // second table for an extra validity check.
    for (int i = 1; i < NUMBER_OF_FAT; i++)
    {
        char *backup = (g_data + BYTES_PER_CLUSTER + (i * FAT_SIZE));

        // Compare both tables
        if (memcmp(fat_table, backup, FAT_SIZE))
        {
            CWARN("FAT table does not match backup table %i.", i);
        }
    }

    // If a new bootsector has to be written,
    // we will overwrite the read boot sector.
    if (g_bootSector[0] != '\0')
    {
        CINFO("Loading bootsector from '%s'.", g_bootSector);

        result = readFile(g_bootSector, g_data, 512);
        if (result)
        {
            CERROR("Could not load boot sector file '%s'. (%i)", g_bootSector, result);
            return -4;
        }
    }

    // Verify the FAT12 boot sector
    result = FAT_verify();
    if (result) {
        return result * 16;
    }

    // Process the user settings
    result = FAT_proc();
    if (result) {
        return result * 48;
    }

    return 0;
}


/**
 * Saves the data as a FAT12 image file.
 * @param file The file to save the data to.
 * @return Zero if successful; otherwise, a non-zero value.
 */
int FAT_save(FILE *file)
{
    size_t written;

    CINFO("Writing image file...");

    written = fwrite(g_data, 1, DEVICE_SIZE, file);
    CASSERT(ferror(file) == 0, "An error occured whilst writing to '%s'", g_pathOut);

    // Ensure the entire file was written
    if (written != DEVICE_SIZE)
    {
        return -1;
    }

    return 0;
}


/**
 * Main program entry point.
 * @param argc The number of arguments passed to the application.
 * @param argv The array containing the passed arguments.
 * @return Zero if successful; otherwise, a non-zero value.
 */
int main(int argc, char **argv)
{
    FILE *lpFile;
    errno_t err;
    int result;

    // Initialize all the values and settings based on defaults and user input
    if (initialize(argc, argv))
    {
        //CERROR("Failed to initialize application.");
        return 0;
    }

    // Should we load an existing file
    if (g_pathIn[0] != '\0')
    {
        err = fopen_s(&lpFile, g_pathIn, "rb");
        if (err)
        {
            CERROR("Could not open input file '%s' for reading. (%d)", g_pathIn, err);
            return 0;
        }

        result = FAT_load(lpFile);
        fclose(lpFile);

        if (result)
        {
            CERROR("Could not load FAT12 image. (%i)", result);
            return 0;
        }
    }

    int file_index = 0;

    // Add all the individual files
    while (!g_queue.empty())
    {
        char *filename = g_queue.front();

        // Need a lot of dummy files
        //int max = (file_index > 0) ? 12 : 1;
        //for (int i = 0; i < max; i++) {

            err = fopen_s(&lpFile, filename, "rb");
            if (err)
            {
                CERROR("Could not open file '%s' for reading. (%d)", filename, err);
                return 0;
            }

            result = OP_addFile(lpFile, filename);
            fclose(lpFile);

            if (result)
            {
                CERROR("Could not add file '%s'. (%i)", filename, result);
                return 0;
            }
        //}
        //file_index++;

        g_queue.pop();
    }

    // Save the data as a FAT12 image.
    {
        err = fopen_s(&lpFile, g_pathOut, "wb");
        if (err)
        {
            CERROR("Could not open output file '%s' for writing. (%d)", g_pathOut, err);
            return 0;
        }

        result = FAT_save(lpFile);
        fclose(lpFile);

        if (result)
        {
            CERROR("Could not save FAT12 image. (%i)", result);
            return 0;
        }
    }

    return 0;
}


/**
 * Gets the entry in the FAT table at the specified index.
 */
int FAT_getEntry(int index)
{
    int value;
    char *target = (fat_table + ((index * 3) / 2));
    char b0 = *(target + 0);
    char b1 = *(target + 1);

    if (index & 1)
    {
        value = 
            ((b0 >> 4) & 0x00F) |
            ((b1 << 4) & 0xFF0);
    }
    else
    {
        value =
            ((b0 << 0) & 0x0FF) |
            ((b1 << 8) & 0xF00);
    }

    return value;
}


/**
 * Sets the entry of the FAT table to the given value.
 */
void FAT_setEntry(int index, int value)
{
    CASSERT((value & ~0xFFF) == 0, "Tried to set invalid value to FAT table.");

    // Backup tables also have to be set
    for (int i = 0; i < NUMBER_OF_FAT; i++)
    {
        char *target = (fat_table + (FAT_SIZE * i) + ((index * 3) / 2));
        char *b0 = (target + 0);
        char *b1 = (target + 1);

        if (index & 1)
        {
            *b0 = ((value << 4) & 0xF0) | (*b0 & 0x0F);
            *b1 = ((value >> 4) & 0xFF);
        }
        else
        {
            *b0 = ((value >> 0) & 0xFF);
            *b1 = ((value >> 8) & 0x0F) | (*b1 & 0xF0);
        }
    }
}


/**
 * Gets a pointer to the data based on the index from the FAT table.
 * @param entryIndex The entry index from the FAT table.
 * @return A pointer to the data of the entry.
 */
char * FAT_getDataPtr(int entryIndex)
{
    CASSERT(entryIndex >= 0x000 && entryIndex < 0xFF0, "entryIndex = 0x%03X", entryIndex);
    int offset = ((33 + entryIndex - 2) * BYTES_PER_CLUSTER);
    return (g_data + offset);
}


/**
 * Searches for the first available cluster on the volume.
 * @param logicalCluster The logical cluster value.
 * @return Zero if successful; otherwise, a non-zero value.
 */
int FAT_findEmptyCluster(short *logicalCluster)
{
    short maxDataClusters = (DATA_SIZE / BYTES_PER_CLUSTER);

    for (short i = 0; i < maxDataClusters; i++)
    {
        int index = FAT_getEntry(i);
        if (index == 0)
        {
            *logicalCluster = i;
            return 0;
        }
    }

    return -1;
}


/**
 * Creates an entry in the FAT table. (DOES NOT ASSIGN A LOGICAL SECTOR!)
 * @param entryIndex The index of the newly created entry.
 * @param name The name to use for the entry.
 * @param ext The extension to use for the entry.
 * @param attribs The attributes for the entry. (aka ARCHIVE, DIRECTORY, etc.)
 * @param data The data to write.
 * @param size The size of the data.
 * @return Zero if successful; otherwise, a non-zero value.
 */
int FAT_createFile(int *entryIndex, char name[8], char ext[3], char attribs, char *data, int size)
{
    int i;
    char *dir = fat_data;

    // Find the first empty index...
    for (i = 0; i < MAX_ROOT_DIRECTORIES; i++)
    {
        // Only empty and erased can be written to
        if (dir[0] == FAT_FLAG_EMPTY ||
            dir[0] == FAT_FLAG_ERASED)
        {
            *entryIndex = i;
            break;
        }

        dir += 32;
    }

    // There were no empty entries
    if (i == MAX_ROOT_DIRECTORIES)
    {
        return -1;
    }

    // Determine the number of required clusters
    int reqClusters = (size / BYTES_PER_CLUSTER);
    if (size % BYTES_PER_CLUSTER)
    {
        // We need an extra cluster
        reqClusters += 1;
    }

    int rem = size;
    short cluster, firstCluster = -1, lastCluster = -1;

    // Keep looping till we allocated all the clusters
    for (i = 0; i < reqClusters; i++)
    {
        // Find the first empty cluster
        if (FAT_findEmptyCluster(&cluster))
        {
            return -2;
        }

        // Store the first cluster
        if (firstCluster == -1)
        {
            firstCluster = cluster;
        }

        // Link the clusters in the FAT table
        if (lastCluster != -1)
        {
            FAT_setEntry(lastCluster, cluster);
        }
        lastCluster = cluster;

        // Ensure the file always ends
        FAT_setEntry(cluster, 0xFFF);

        // Get the pointer to the cluster data
        char *dataCluster = FAT_getDataPtr(cluster);

        // Make it fit to all the buffers and write
        int write = (rem >= BYTES_PER_CLUSTER) ? BYTES_PER_CLUSTER : rem;
        memcpy(dataCluster, data, write);
        data += write;
        rem -= write;
    }

    // Store the file information
    memcpy(dir, name, 8);
    memcpy(dir + 8, ext, 3);
    memcpy(dir + 11, &attribs, 1);
    memcpy(dir + 26, &firstCluster, 2);
    memcpy(dir + 28, &size, 4);

    return 0;
}


/**
 * Strips the data in a single directory.
 * @param dir A pointer to the directory in memory.
 * @param count The number of entries in the directory.
 * @return Zero if successful; otherwise, a non-zero value.
 */
int OP_stripDir(char *dir, int count)
{
    for (int i = 0; i < count; i++)
    {
        bool erase = false;

        char *cur = (dir + (i * 32));
        if (cur[0] == FAT_FLAG_ERASED)
        {
            // Previously erased and empty files can be stripped
            erase = true;
        }
        else
        {
            // We need to check file attributes
            if (cur[11] == FAT_ATTRIB_LONGNAME)
            {
                erase = true;
            }
        }

        // Erase this entry
        if (erase)
        {
            int offset = (i * 32);
            char *dest = (dir + offset);
            char *src = (dir + offset + 32);

            for (int x = i+1; x < count; x++)
            {
                memcpy(dest, src, 32);
                dest += 32;
                src += 32;
            }

            // Clear the last entry
            dest = (dir + (count-1) * 32);
            memset(dest, 0, 32);

            // Try this copy again
            i--;
        }
    }

    return 0;
}


/**
 * Strips a directory and it's sub-directories.
 * @param dir A pointer to the directory.
 * @param numEntries The number of entries in the directory.
 * @return Zero if successful; otherwise, a non-zero value.
 */
int OP_stripVar(char *dir, int numEntries)
{
    int result;

    CASSERT(numEntries == MAX_ROOT_DIRECTORIES || numEntries == ENTRIES_PER_DIR, "Invalid count...");

    // First strip this directory
    result = OP_stripDir(dir, numEntries);
    if (result)
    {
        return result;
    }

    // Also strip sub-directories
    for (int i = 0; i < numEntries; i++)
    {
        char *fileEntry = (dir + (i * 32));

        // Skip the special directory entries
        if (fileEntry[0] == FAT_FLAG_SPECIAL)
        {
            continue;
        }

        // The file entry is a sub-directory
        if (fileEntry[11] & FAT_ATTRIB_SUBDIRECTORY)
        {
            short sector = *(reinterpret_cast<short*>(fileEntry + 26));
            //int index = FAT_getEntry(sector);
            char *data_p = FAT_getDataPtr(sector);
            result = OP_stripVar(data_p, ENTRIES_PER_DIR);
            if (result)
            {
                return result;
            }
        }
    }

    return 0;
}


/**
 * Strips the volume of unused data.
 * @return Zero if successful; otherwise, a non-zero value.
 */
int OP_strip(void)
{
    CVERBOSE("OP: stripping...");
    return OP_stripVar(fat_data, MAX_ROOT_DIRECTORIES);
}


/**
 * Formats the FAT input.
 * @return Zero if successfull; otherwise, a non-zero value.
 */
int OP_format(void)
{
    CVERBOSE("OP: formatting...");

    // Clear the data, except the volume
    memset(fat_data + 32, 0, DATA_SIZE - 32);
    
    // Clear all the FAT tables in one sweep
    memset(fat_table, 0, (NUMBER_OF_FAT * FAT_SIZE));

    // Set the basic values
    FAT_setEntry(0, 0xFF0);
    FAT_setEntry(1, 0xFFF);

    return 0;
}


/**
 * Defragment the volume.
 * @return Zero if successful; otherwise, a non-zero value.
 */
int OP_defragment(void)
{
    CVERBOSE("OP: defragment...");

    return -1;
}


/**
 * Adds a file to the image.
 * @param file The file to add.
 * @param path The path to the file.
 * @return Zero if successful; otherwise, a non-zero value.
 */
int OP_addFile(FILE *file, const char *path)
{
    const char *sep0, *sep1;
    char name[8], ext[3];

    CVERBOSE("OP: adding '%s'...", path);
   
    // Get the path
    sep0 = strrchr(path, '/');
    sep1 = strrchr(path, '\\');

    // Choose the seperator to use
    if (sep1)
    {
        if (!sep0)
        {
            sep0 = sep1;
        }
        else
        {
            if (sep0 < sep1)
            {
                sep0 = sep1;
            }
        }
    }

    // If there is a seperator we will replace the path
    if (sep0)
    {
        OP_convertToFileName(sep0 + 1, name, ext);
    }
    else
    {
        OP_convertToFileName(path, name, ext);
    }

    CINFO("Writing '%s' to '%.8s'.'%.3s'", path, name, ext);

    // Get the file size
    fseek(file, 0, SEEK_END);
    long int size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Ensure we're still error free
    CASSERT(ferror(file) == 0, "Could not determine the file size of '%s'", path);
    CASSERT(size <= FILE_BUFFER_SIZE, "Increase FILE_BUFFER_SIZE constant");

    size_t read = fread(g_bufferFileData, 1, size, file);
    CASSERT(ferror(file) == 0, "Could not read from '%s'", path);
    if (read != size)
    {
        return -2;
    }

    int entryIndex;
    int result = FAT_createFile(&entryIndex, name, ext, FAT_ATTRIB_ARCHIVE, g_bufferFileData, size);

    return result * 16;
}


/**
 * Get the name of the file from a file entry.
 * @param entry The entry to get the name of.
 * @param name The name of the file.
 * @param ext The extension of the file.
 * @return Zero if successful; otherwise, a non-zero value.
 */
int OP_getEntryFileName(char *entry, char name[8], char ext[3])
{
    memcpy(name, entry, 8);
    memcpy(ext, entry + 8, 3);
    return 0;
}


/**
 * Searches for a file with the matching name in the given directory.
 * @param dir The directory to search in.
 * @param name The name of the file.
 * @param match The destination to the store a pointer to the file's entry.
 * @return Zero if successful; otherwise, a non-zero value.
 */
int OP_searchFileInDir(char *dir, const char *name, char **match)
{
    char lName[8], lExt[3];
    char eName[8], eExt[3];

    // Convert the inputted name into FAT format.
    OP_convertToFileName(name, lName, lExt);

    // Loop over all the entries in the directory
    for (int i = 0; i < ENTRIES_PER_DIR; i++)
    {
        char *entry = (dir + (i * 32));
        OP_getEntryFileName(entry, eName, eExt);

        // We found a match
        if (!memcmp(lName, eName, 8) && !memcmp(lExt, eExt, 3))
        {
            *match = entry;
            return 0;
        }
    }

    return -1;
}


/**
 * Searches for a file with the matching name on the entire volume.
 * @param name The name of the file.
 * @param dest The destination to the store a pointer to the file's entry.
 * @param n The number of files matching the name.
 * @return Zero if succcessful; otherwise, a non-zero value.
 */
int OP_searchFile(const char *name, void **dest, int *n)
{
    return -1;
}


/**
 * Converts the input string into a valid FAT output string.
 * @param dest The destination to put the output string.
 * @param destSize The maximum number of characters to write.
 * @param src The string to convert.
 * @param srcSource The number of available characters.
 * @return Zero if successful; otherwise, a non-zero value.
 */
int OP_toValidString(char *dest, size_t destSize, const char *src, size_t srcSize)
{
    for (size_t i = 0; i < srcSize; i++)
    {
        char cur = src[i];

        // Convert to uppercase
        if (cur >= 'a' && cur <= 'z')
        {
            cur -= ('a' - 'A');
        }

        // Copy range
        if ((cur >= 'A' && cur <= 'Z') ||
            (cur >= '0' && cur <= '9'))
        {
            *dest = cur;
            dest++;
            destSize--;
        }

        // Individual
        else if (
            cur == '\'' ||
            cur == ' ' ||
            cur == '!' ||
            cur == '#' ||
            cur == '$' ||
            cur == '%' ||
            cur == '&' ||
            cur == '(' ||
            cur == ')' ||
            cur == '-' ||
            cur == '@' ||
            cur == '^' ||
            cur == '_' ||
            cur == '`' ||
            cur == '{' ||
            cur == '}' ||
            cur == '~')
        {
            *dest = cur;
            dest++;
            destSize--;
        }

        // Reached the limit
        if (destSize == 0)
        {
            return 0;
        }
    }

    return 0;
}


/**
 * Converts the input string into a valid file name.
 * @param in The input string.
 * @param name The name of the file.
 * @param ext The extension of the file.
 * @return Zero if successful; otherwise, a non-zero value.
 */
int OP_convertToFileName(const char * in, char name[8], char ext[3])
{
    size_t len;
    const char *period;

    // Clear both fields
    memset(name, 0x20, 8);
    memset(ext, 0x20, 3);
    
    // Determine the extension
    period = strrchr(in, '.');
    if (period)
    {
        OP_toValidString(ext, 3, period + 1, strlen(period + 1));
        len = (period - in);
    }
    else
    {
        len = strlen(in);
    }

    // Copy the file name
    OP_toValidString(name, 8, in, len);

    return -1;
}

/**
 * Ensures the name is unique within the directory.
 * @param dir The directory containing the file.
 * @param in The input file name.
 * @param name The name of the file.
 * @param ext The extension of the file.
 * @return Zero if successful; otherwise, a non-zero value.
 */
int OP_toUniqueName(char *dir, char *in, char name[8], char ext[3])
{
    return OP_convertToFileName(in, name, ext);
}
