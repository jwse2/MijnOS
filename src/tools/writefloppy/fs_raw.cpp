#include <cstddef>
#include <cstdio>
#include <cstring>
#include <queue>
#include "fs_raw.hpp"
#include "globals.hpp"
#include "utility.hpp"

/** For a description see the header file. */
int Raw_CopyData(FILE *oFile)
{
    char buffer[BUFFER_SIZE];
    size_t offset, written, read = BUFFER_SIZE;
    size_t size;
    FILE *iFile;

    // Always start with the bootloader first
    iFile = FS_OpenBootloader(&size);
    if (iFile == NULL)
    {
        fprintf(stderr, "I/O ERROR: A.\n");
        return -1;
    }

    // Read and write per buffer till the output file has been filled
    for (offset = 0; offset < FLOPPY_SIZE; offset += BUFFER_SIZE)
    {
        // Open a new file if non is opened
        if (!iFile)
        {
            iFile = FS_OpenNextFile(&size, NULL);
            if (!iFile)
            {
                fprintf(stderr, "I/O ERROR: A.\n");
                return -1;
            }
        }

        // Clear the buffer
        memset(buffer, 0, BUFFER_SIZE);

        if (iFile)
        {
            // Read from the input file
            read = fread(buffer, 1, BUFFER_SIZE, iFile);

            // The filesize may be exactly that of a single buffer, hence we
            // need to keep track of the size as this has to be checked as well.
            size -= read;

            // We read to the end of the file
            if (read != BUFFER_SIZE)
            {
                if (feof(iFile))
                {
                    fclose(iFile);
                    iFile = NULL;
                }
                else
                {
                    if (!ferror(iFile))
                    {
                        fprintf(stderr, "I/O ERROR: An unknown condition occured.\n");
                        fclose(iFile);
                        return 4;
                    }
                }
            }
            else if (size <= 0)
            {
                fclose(iFile);
                iFile = NULL;
            }
        }

        // Write the buffer to the output file
        written = fwrite(buffer, 1, BUFFER_SIZE, oFile);

        // Could not write the file, abort end give an error
        if (written != BUFFER_SIZE)
        {
            fprintf(stderr, "I/O ERROR: Could not write all the bytes, %zu of %i were written.\n", written, BUFFER_SIZE);
            fclose(iFile);
            return 1;
        }
    }

    return 0;
}
