#ifndef FS_FAT12_HPP
#define FS_FAT12_HPP

#include <cstdio>

/**
 * Copies the data from the input files in the given order into the output file.
 * It uses the FAT12 filesystem in the outputed file.
 * @param oFile The output file stream.
 * @return Zero if successful; otherwise, a non-zero value.
 */
int FAT12_CopyData(FILE *oFile);

#endif //FS_FAT12_HPP
