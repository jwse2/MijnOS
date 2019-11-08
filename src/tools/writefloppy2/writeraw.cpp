#include <cstdio>
#include <queue>

#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>

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

char g_data[DEVICE_SIZE];           /* Contains all the date of the image. */
char g_workingDirectory[MAX_PATH];  /* The path to the working directory */
char g_pathOut[MAX_PATH];           /* The filename of the output file */
std::queue<char*> g_queue;          /* All the input file names */
size_t g_totalWritten;              /* Number of bytes written in total */


/**
 * Sets the default values for the dynamic settings.
 */
void setDefaults(void)
{
    memset(g_data, 0, DEVICE_SIZE);
    memset(g_workingDirectory, 0, MAX_PATH);
    memset(g_pathOut, 0, MAX_PATH);
    g_totalWritten = 0;
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


            if (arg[0] == 'w')
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
 */

    printf("\nUSAGE: %s <options> file <additional files>\n", lpExeName);
    printf("\nEXAMPLE: %s -w .\\bin -o floppy.flp bootloader.bin kernel.bin\n", lpExeName);

#define OPTION_EXT(arg0, arg1, text) \
    printf("  %-2s %-8s %s\n", arg0, arg1, text)

    // Options
    printf("\nOPTIONS\n");
    OPTION_EXT("-w", "<path>", "Changes the working directory.");
    OPTION_EXT("-o", "<path>", "Changes the output file.");
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

    return 0;
}


int copyFileData(FILE *lpFileOut, FILE *lpFileIn, const char *filename)
{
    char buffer[BYTES_PER_SECTOR];
    size_t read, written;
    bool notified = false;

    do
    {
        memset(buffer, 0, BYTES_PER_SECTOR);
        read = fread(buffer, 1, BYTES_PER_SECTOR, lpFileIn);
        CASSERT(ferror(lpFileIn) == 0, "An error occured while reading from '%s'", filename);

        if (read > 0)
        {
            if ((g_totalWritten + BYTES_PER_SECTOR) > DEVICE_SIZE)
            {
                CERROR("Tried to write too many bytes to the output.");
                return -1;
            }

            if (!notified)
            {
                notified = true;
                CINFO("Writing '%s' to 0x%08zX", filename, g_totalWritten);
            }

            written = fwrite(buffer, 1, BYTES_PER_SECTOR, lpFileOut);
            CASSERT(ferror(lpFileOut) == 0, "An error occured while writing.");

            if (written != BYTES_PER_SECTOR)
            {
                CERROR("Could not write buffer to '%s'.", filename);
                return -2;
            }

            g_totalWritten += BYTES_PER_SECTOR;
        }
    }
    while (read == BYTES_PER_SECTOR);

    return 0;
}


int appendOutput(FILE *lpFileOut, char *filename)
{
    char buffer[BYTES_PER_SECTOR];
    size_t written;

    memset(buffer, 0, BYTES_PER_SECTOR);

    while (g_totalWritten < DEVICE_SIZE)
    {
        written = fwrite(buffer, 1, BYTES_PER_SECTOR, lpFileOut);
        CASSERT(ferror(lpFileOut) == 0, "An error occured while writing.");

        if (written != BYTES_PER_SECTOR)
        {
            CERROR("Could not write buffer to '%s'.", filename);
            return -1;
        }

        g_totalWritten += BYTES_PER_SECTOR;
    }

    return 0;
}


int main(int argc, char **argv)
{
    FILE *lpFileOut, *lpFileIn;
    errno_t err;
    int result;

    // Initialize all the values and settings based on defaults and user input
    if (initialize(argc, argv))
    {
        //CERROR("Failed to initialize application.");
        return 0;
    }

    CASSERT(g_pathOut[0] != '\0', "Output file not set.");

    err = fopen_s(&lpFileOut, g_pathOut, "wb");
    if (!err)
    {
        while (!g_queue.empty())
        {
            char *filename = g_queue.front();

            err = fopen_s(&lpFileIn, filename, "rb");
            if (err)
            {
                CERROR("Could not open input file '%s'.", filename);
                break;
            }

            result = copyFileData(lpFileOut, lpFileIn, filename);
            if (result)
            {
                fclose(lpFileIn);
                CERROR("Could not write file '%s'. (%i)", filename, result);
                break;
            }

            result = appendOutput(lpFileOut, filename);
            fclose(lpFileOut);

            if (result)
            {
                CERROR("Could not append file '%s'. (%i)", filename, result);
                break;
            }

            g_queue.pop();
        }

        fclose(lpFileOut);
    }
    else
    {
        CERROR("Could not open output file '%s'.", g_pathOut);
    }

    return 0;
}

