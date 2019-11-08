
#ifndef MAX_PATH
#define MAX_PATH    260 /* Windows default is 260 */
#endif

/**
 * SETTINGS
 *   Device settings for FAT12, these impact read/write operations.
 */
#define BYTES_PER_SECTOR        512
#define SECTORS_PER_CLUSTER     1
#define RESERVED_CLUSTERS       1
#define NUMBER_OF_FAT           2
#define MAX_ROOT_DIRECTORIES    224
#define TOTAL_SECTORS_FAT16     2880
#define DEVICE_TYPE             0xF0
#define SECTORS_PER_FAT         9
#define SECTORS_PER_TRACK       18
#define NUMBER_OF_HEADS         2
#define NUMBER_OF_HIDDEN_SEC    0
#define TOTAL_SECTORS_FAT32     0 /* We are using FAT12, thus should remain 0 */
#define PHYSICAL_DRIVE_NUM      0
#define RESERVED_VALUE          0
#define BOOT_SIGNATURE          0x29
#define VOLUME_ID               0xE457A504
#define VOLUME_LABEL            'NO NAME    '
#define FILE_SYSTEM             'FAT12   '


/**
 * UTILITY
 *   These are values based on the settings above.
 */
#define DEVICE_SIZE         (BYTES_PER_SECTOR * TOTAL_SECTORS_FAT16)
#define BYTES_PER_CLUSTER   (BYTES_PER_SECTOR * SECTORS_PER_CLUSTER)
#define NUMBER_OF_CLUSTERS  (BYTES_PER_SECTOR / SECTORS_PER_CLUSTER)
#define FAT_SIZE            (BYTES_PER_SECTOR * SECTORS_PER_FAT)
#define NUM_FAT_ENTRIES     (FAT_SIZE / 3)
#define NUMBER_OF_TRACKS    (TOTAL_SECTORS_FAT16 / SECTORS_PER_TRACK)
#define DATA_SIZE           (DEVICE_SIZE - BYTES_PER_CLUSTER - (FAT_SIZE * NUMBER_OF_FAT))
#define NUM_ROOT_CLUSTERS   ((MAX_ROOT_DIRECTORIES * 32) / BYTES_PER_CLUSTER)
#define ENTRIES_PER_DIR     (BYTES_PER_CLUSTER / 32)
