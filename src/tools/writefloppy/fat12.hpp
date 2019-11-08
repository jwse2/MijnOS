#ifndef FAT12_HPP
#define FAT12_HPP

// Helpers
#define FROM_BYTE(x) \
    ((char)(x))

#define FROM_WORD(x) \
    ((char)((((uint16_t)(x)) & 0xFF00) >> 8)), \
    ((char)((((uint16_t)(x)) & 0x00FF) >> 0))

#define FROM_DWORD(x) \
    ((char)((((uint32_t)(x)) & 0xFF000000) >> 24)), \
    ((char)((((uint32_t)(x)) & 0x00FF0000) >> 16)), \
    ((char)((((uint32_t)(x)) & 0x0000FF00) >> 8)), \
    ((char)((((uint32_t)(x)) & 0x000000FF) >> 0))

#define FROM_QWORD(x) \
    ((char)((((uint64_t)(x)) & 0xFF00000000000000) >> 56)), \
    ((char)((((uint64_t)(x)) & 0x00FF000000000000) >> 48)), \
    ((char)((((uint64_t)(x)) & 0x0000FF0000000000) >> 40)), \
    ((char)((((uint64_t)(x)) & 0x000000FF00000000) >> 32)), \
    ((char)((((uint64_t)(x)) & 0x00000000FF000000) >> 24)), \
    ((char)((((uint64_t)(x)) & 0x0000000000FF0000) >> 16)), \
    ((char)((((uint64_t)(x)) & 0x000000000000FF00) >> 8)), \
    ((char)((((uint64_t)(x)) & 0x00000000000000FF) >> 0))

#define STRING8(a,b,c,d,e,f,g,h) \
    (((char)(a))), \
    (((char)(b))), \
    (((char)(c))), \
    (((char)(d))), \
    (((char)(e))), \
    (((char)(f))), \
    (((char)(g))), \
    (((char)(h)))

#define STRING12(a,b,c,d,e,f,g,h,i,j,k,l) \
    (((char)(a))), \
    (((char)(b))), \
    (((char)(c))), \
    (((char)(d))), \
    (((char)(e))), \
    (((char)(f))), \
    (((char)(g))), \
    (((char)(h))), \
    (((char)(i))), \
    (((char)(j))), \
    (((char)(k))), \
    (((char)(l)))


typedef struct
{
    short time;
    short date;
} DateTime_t;

/**
 * A traditional FAT entry.
 */
typedef struct
{
    char        filename[8];
    char        extension[3];
    uint8_t     attributes;
    char        reserved[2];
    DateTime_t  creation;
    short       lastAccessDate;
    char        ignored[2];
    DateTime_t  lastWrite;
    short       firstLogicalSector;
    int         fileSize;
} FATEntry_t;

/**
 * A Long-File-Name (LFN) entry in FAT.
 */
typedef struct
{
    char        index;
    wchar_t     str0[5];
    uint8_t     attributes;
    char        reserved;
    uint8_t     checksum;
    wchar_t     str1[6];
    short       firstLogicalSector;
    wchar_t     str2[2];
} LFNEntry_t;

// FAT entry types for next sector
#define FAT_TYPE_UNUSED             ((unsigned char)(0x000))
#define FAT_TYPE_RESERVED           ((unsigned char)(0xFF0))
#define FAT_TYPE_BAD_CLUSTER        ((unsigned char)(0xFF7))
#define FAT_TYPE_LAST_OF_FILE       ((unsigned char)(0xFFF))
#define FAT_TYPE_VAR(var)           ((unsigned char)(var))

// File Attribute Flags
#define FAT_ATTRIB_READ_ONLY        ((unsigned char)(0x01))
#define FAT_ATTRIB_HIDDEN           ((unsigned char)(0x02))
#define FAT_ATTRIB_SYSTEM           ((unsigned char)(0x04))
#define FAT_ATTRIB_VOLUME_LABEL     ((unsigned char)(0x08))
#define FAT_ATTRIB_SUBDIRECTORY     ((unsigned char)(0x10))
#define FAT_ATTRIB_ARCHIVE          ((unsigned char)(0x20))
#define FAT_ATTRIB_LONGNAME         ((unsigned char)(0x0F))


#endif //FAT12_HPP
