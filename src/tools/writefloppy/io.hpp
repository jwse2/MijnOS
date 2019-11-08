#ifndef IO_HPP
#define IO_HPP

/** Get the size of the file stream. */
long int fsize(FILE *stream);

/** Get an indicator to whether the machine uses little-endian. */
bool isLittleEndian(void);

/**
 * Read data from a stream in little-endian.
 */
int readBYTE(FILE *stream, void *out);  //  8-bit
int readWORD(FILE *stream, void *out);  // 16-bit
int readDWORD(FILE *stream, void *out); // 32-bit
int readQWORD(FILE *stream, void *out); // 64-bit

/**
 * Write data to a stream in little-endian.
 */
int writeBYTE(FILE *stream, void *in);  //  8-bit
int writeWORD(FILE *stream, void *in);  // 16-bit
int writeDWORD(FILE *stream, void *in); // 32-bit
int writeQWORD(FILE *stream, void *in); // 64-bit

#endif //IO_HPP
