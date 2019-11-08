#include <cstddef>
#include <cstdio>
#include <cstdint>
#include "io.hpp"


/**
 * Determines the size of a file.
 * @param stream A pointer to the file stream.
 * @return A negative value indicates an error occured; otherwise, the size of the file.
 */
long int fsize(FILE *stream)
{
    long int size;

    if (fseek(stream, 0, SEEK_END))
    {
        return -1;
    }

    size = ftell(stream);
    if (size == -1L)
    {
        return -2;
    }

    if (fseek(stream, 0, SEEK_SET))
    {
        return -3;
    }

    return size;
}

/**
 * Determines the machine's endianness.
 * @return true if little-endian; otherwise, big-endian is assumed.
 */
bool isLittleEndian(void)
{
    union
    {
        unsigned char buffer[4];
        unsigned int value;
    };
    
    value = 0x11223344;

    // We only accomodate for Little and Big endianness
    return (buffer[0] == 0x44);
}

/**
 * Reads a BYTE from the stream into the referenced variable.
 * @param stream The file stream to read from.
 * @param out A reference to to location to store the value.
 * @return Zero if successful; otherwise, a non-zero value.
 */
int readBYTE(FILE *stream, void *out)
{
    int value = getc(stream);

    if (value == EOF)
    {
        return -1;
    }

    *(static_cast<int8_t*>(out)) = static_cast<int8_t>(value);

    return 0;
}

/**
 * Writes a BYTE to the stream.
 * @param stream The file stream to write too.
 * @param in The variable to write.
 * @return Zero if successful; otherwise, a non-zero value.
 */
int writeBYTE(FILE *stream, void *in)
{
    int result = fputc(*(static_cast<int8_t*>(in)), stream);

    if (result == EOF)
    {
        return -1;
    }

    return 0;
}

/**
 * Reads a WORD from the stream into the referenced variable.
 * @param stream The file stream to read from.
 * @param out A reference to to location to store the value.
 * @return Zero if successful; otherwise, a non-zero value.
 */
int readWORD(FILE *stream, void *out)
{
    union
    {
        int8_t  buffer[2];
        int16_t value;
    };

    if (2 != fread(buffer, 1, 2, stream))
    {
        return -1;
    }

    if (!isLittleEndian())
    {
        *(static_cast<int16_t*>(out)) = static_cast<int16_t>(
            (static_cast<uint16_t>(buffer[0]) << 8) |
            (static_cast<uint16_t>(buffer[1]) << 0)
        );
    }
    else
    {
        *(static_cast<int16_t*>(out)) = value;
    }

    return 0;
}

/**
 * Writes a WORD to the stream.
 * @param stream The file stream to write too.
 * @param in The variable to write.
 * @return Zero if successful; otherwise, a non-zero value.
 */
int writeWORD(FILE *stream, void *in)
{
    union
    {
        int8_t  buffer[2];
        int16_t value;
    } out, tmp;

    if (!isLittleEndian())
    {
        tmp.value = *(static_cast<int16_t*>(in));

        out.value = static_cast<int16_t>(
            (static_cast<uint16_t>(tmp.buffer[0]) << 8) |
            (static_cast<uint16_t>(tmp.buffer[1]) << 0)
        );
    }
    else
    {
        out.value = *(static_cast<int16_t*>(in));
    }

    return (2 != fwrite(out.buffer, 1, 2, stream)) ? -1 : 0;
}

/**
 * Reads a DWORD from the stream into the referenced variable.
 * @param stream The file stream to read from.
 * @param out A reference to to location to store the value.
 * @return Zero if successful; otherwise, a non-zero value.
 */
int readDWORD(FILE *stream, void *out)
{
    union
    {
        int8_t  buffer[4];
        int32_t value;
    };

    if (4 != fread(buffer, 1, 4, stream))
    {
        return -1;
    }

    if (!isLittleEndian())
    {
        *(static_cast<int32_t*>(out)) = static_cast<int32_t>(
            (static_cast<uint32_t>(buffer[0]) << 24) |
            (static_cast<uint32_t>(buffer[1]) << 16) |
            (static_cast<uint32_t>(buffer[2]) << 8) |
            (static_cast<uint32_t>(buffer[3]) << 0)
        );
    }
    else
    {
        *(static_cast<int32_t*>(out)) = value;
    }

    return 0;
}

/**
 * Writes a DWORD to the stream.
 * @param stream The file stream to write too.
 * @param in The variable to write.
 * @return Zero if successful; otherwise, a non-zero value.
 */
int writeDWORD(FILE *stream, void *in)
{
    union
    {
        int8_t  buffer[4];
        int32_t value;
    } out, tmp;

    if (!isLittleEndian())
    {
        tmp.value = *(static_cast<int32_t*>(in));

        out.value = static_cast<int32_t>(
            (static_cast<uint32_t>(tmp.buffer[0]) << 24) |
            (static_cast<uint32_t>(tmp.buffer[1]) << 16) |
            (static_cast<uint32_t>(tmp.buffer[2]) << 8) |
            (static_cast<uint32_t>(tmp.buffer[3]) << 0)
        );
    }
    else
    {
        out.value = *(static_cast<int32_t*>(in));
    }

    return (4 != fwrite(out.buffer, 1, 4, stream)) ? -1 : 0;
}

/**
 * Reads a QWORD from the stream into the referenced variable.
 * @param stream The file stream to read from.
 * @param out A reference to to location to store the value.
 * @return Zero if successful; otherwise, a non-zero value.
 */
int readQWORD(FILE *stream, void *out)
{
    union
    {
        int8_t  buffer[8];
        int64_t value;
    };

    if (8 != fread(buffer, 1, 8, stream))
    {
        return -1;
    }

    if (!isLittleEndian())
    {
        *(static_cast<int64_t*>(out)) = static_cast<int64_t>(
            (static_cast<uint64_t>(buffer[0]) << 56) |
            (static_cast<uint64_t>(buffer[1]) << 48) |
            (static_cast<uint64_t>(buffer[2]) << 40) |
            (static_cast<uint64_t>(buffer[3]) << 32) |
            (static_cast<uint64_t>(buffer[4]) << 24) |
            (static_cast<uint64_t>(buffer[5]) << 16) |
            (static_cast<uint64_t>(buffer[6]) << 8) |
            (static_cast<uint64_t>(buffer[7]) << 0)
        );
    }
    else
    {
        *(static_cast<int64_t*>(out)) = value;
    }

    return 0;
}

/**
 * Writes a QWORD to the stream.
 * @param stream The file stream to write too.
 * @param in The variable to write.
 * @return Zero if successful; otherwise, a non-zero value.
 */
int writeQWORD(FILE *stream, void *in)
{
    union
    {
        int8_t  buffer[8];
        int64_t value;
    } out, tmp;

    if (!isLittleEndian())
    {
        tmp.value = *(static_cast<int64_t*>(in));

        out.value = static_cast<int64_t>(
            (static_cast<uint64_t>(tmp.buffer[0]) << 24) |
            (static_cast<uint64_t>(tmp.buffer[1]) << 16) |
            (static_cast<uint64_t>(tmp.buffer[2]) << 8) |
            (static_cast<uint64_t>(tmp.buffer[3]) << 0)
        );
    }
    else
    {
        out.value = *(static_cast<int64_t*>(in));
    }

    return (4 != fwrite(out.buffer, 1, 4, stream)) ? -1 : 0;
}
