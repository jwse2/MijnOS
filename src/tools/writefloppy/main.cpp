#include <cstddef>
#include <cstdio>
#include <cstring>
#include <queue>
#include "io.hpp"
#include "globals.hpp"
#include "fs_raw.hpp"
#include "fs_fat12.hpp"

#define ARGUMENT_NO_VALUE(arg)      "Argument "arg" has no value specified.\n"


/** Global variables */
std::queue<FileArg> g_queue;    /* All the input files as given */
char g_outputFile[MAX_PATH];    /* The output file path */
char *g_bootloader;             /* The bootloader file */
int g_filesystem;               /* The filesystem to use */


/** Process the arguments passed to the application. */
int procArguments(int argc, char **argv)
{
    int i;

    for (i = 0; i < argc; i++)
    {
        // 'Quickly seperate arguments from file names
        if (argv[i][0] == '-')
        {
            // Set the bootloader file
            if (!strcmp(argv[i], "-b"))
            {
                if (++i >= argc)
                {
                    printf(ARGUMENT_NO_VALUE("-b"));
                    continue;
                }

                // Change the bootloader name to the specified filename
                g_bootloader = argv[i];
            }

            // Change the working directory
            if (!strcmp(argv[i], "-d"))
            {
                if (++i >= argc)
                {
                    printf(ARGUMENT_NO_VALUE("-d"));
                    continue;
                }

                // Change the working directory
                if (!SetCurrentDirectoryA(argv[i]))
                {
                    return 1;
                }
            }

            // Change the output file
            else if (!strcmp(argv[i], "-o"))
            {
                if (++i >= argc)
                {
                    printf(ARGUMENT_NO_VALUE("-o"));
                    continue;
                }

                // Copy the filename into the array; but truncate if too long
                strncpy_s(g_outputFile, MAX_PATH, argv[i], _TRUNCATE);
            }

            // Set the filesystem to raw
            else if (!strcmp(argv[i], "-raw"))
            {
                g_filesystem = FS_RAW;
            }

            // Set the filesystem to FAT12
            else if (!strcmp(argv[i], "-fat12"))
            {
                g_filesystem = FS_FAT12;
            }
        }

        // Otherwise it is an input file
        else
        {
            char *cur = argv[i];
            char *sep = strchr(cur, ':');

            // There is no seperator
            if (sep == nullptr)
            {
                g_queue.push(FileArg(ATTRIB_NONE, cur));
            }

            // Seperate the attributes from the filename
            else
            {
                int attributes = ATTRIB_NONE;

                // Loop over all the characters
                for (int j = 0; j < (sep-cur); j++)
                {
                    if (cur[j] == 'H')
                    {
                        attributes |= ATTRIB_HIDDEN;
                    }
                    else if (cur[j] == 'R')
                    {
                        attributes |= ATTRIB_READONLY;
                    }
                    else if (cur[j] == 'S')
                    {
                        attributes |= ATTRIB_SYSTEM;
                    }
                    else
                    {
                        printf("WARNING: Unknown file attribute '%c'\n", cur[j]);
                    }
                }

                g_queue.push(FileArg(attributes, sep+1));
            }
        }
    }

    return (i == argc) ? 0 : 1;
}

/** Set the globals to their default values. */
int setDefaults(void)
{
    g_bootloader = nullptr;
    g_filesystem = FS_FAT12;
    return strcpy_s(g_outputFile, MAX_PATH, "default.flp");
}

/** Print a message on how to use the application. */
void printUsage(const char *exeName)
{
    // Display standard usage
    printf("\nUSAGE: %s <filesystem> <options> inputFiles\n", exeName);

    // Display an usage example
    printf("\nEXAMPLE: -fat12 %s -d .\\bin -o device.flp -b bootloader.bin RS:kernel.bin p_arkanoid.bin\n", exeName);

#define OPTION(arg, text) \
    printf("  %-8s %s\n", arg, text)

    // Filesystem
    printf("\nFILESYSTEM (DEFAULT: -fat12)\n");
    OPTION("-raw", "Use no filesystem");
    OPTION("-fat12", "Use the FAT12 filesystem");

    // Options list
    printf("\nOPTIONS\n");
    OPTION("-b", "Set the file to use as the bootloader (default: first input file)");
    OPTION("-d", "Change the working directory");
    OPTION("-o", "Set the output file name (default: default.flp)");

    // Attributes
    printf("\nATTRIBUTES (DEFAULT: none are set)\n");
    OPTION("H", "Hidden");
    OPTION("R", "Read-Only");
    OPTION("S", "System");

#undef OPTION
}

/** Initialize all the program variables to the correct values. */
int initialize(int argc, char **argv)
{
    if (argc < 3)
    {
        printUsage(argv[0]);
        return -1;
    }

    // Set the defaults
    if (setDefaults())
    {
        printf("Could set default values.\n");
        return -2;
    }

    // Process the arguments but skip the executable name
    if (procArguments(argc - 1, argv + 1))
    {
        printf("An error occured while processing the passed arguments.\n");
        return -3;
    }

    // If the bootloader was not specifically set take the first file
    if (g_bootloader == nullptr)
    {
        if (g_queue.empty())
        {
            printf("No bootloader file specified; neither is there a file available to use as a bootloader.\n");
            return -4;
        }

        // Set the bootloader and remove it from the queue
        FileArg arg = g_queue.front();
        g_bootloader = arg.filename;
        g_queue.pop();
    }

    return 0;
}

/** Application entry point. */
int main(int argc, char **argv)
{
    if (initialize(argc, argv))
    {
        return 0;
    }

    FILE *oFile = NULL; // output
    errno_t err; // Windows error

    // Open the output file in write/binary
    err = fopen_s(&oFile, g_outputFile, "wb");
    if (err)
    {
        return 0;
    }

    // Copy the data using the specified filesystem format;
    // and give a message on both success and failure.
    if (g_filesystem == FS_RAW)
    {
        if (!Raw_CopyData(oFile))
        {
            printf("Data has been written successfully!\n");
        }
        else
        {
            printf("ERROR: Raw write failed.\n");
        }
    }
    else if (g_filesystem == FS_FAT12)
    {
        if (!FAT12_CopyData(oFile))
        {
            printf("Data has been written successfully!\n");
        }
        else
        {
            printf("ERROR: FAT12 write failed.\n");
        }
    }
    else
    {
        printf("ERROR: Filesystem not supported.\n");
    }

    // Close the file streams
    fclose(oFile);
    return 0;
}
