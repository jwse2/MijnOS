#ifndef FS_RAW_HPP
#define FS_RAW_HPP

#include <cstdio>

/**
 * Copies the data from the input files in the given order into the output file.
 * @param oFile The output file stream.
 * @return Zero if successful; otherwise, a non-zero value.
 */
int Raw_CopyData(FILE *oFile);

#endif //FS_RAW_HPP
