#ifndef UTILITY_HPP
#define UTILITY_HPP

#include <cstddef>
#include <cstdio>

FILE* FS_OpenBootloader(size_t *size);
FILE* FS_OpenNextFile(size_t *size, char **filename);

#endif //UTILITY_HPP
