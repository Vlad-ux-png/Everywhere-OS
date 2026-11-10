/*++

Copyright (c) 2026  Everywhere Computing, Inc. All Rights Reserved.

Module Name:

    evryfs.h

Abstract:

    EVRYFS on-disk structures and public API. Filesystem lives under
    base/fs/evryfs, separate from the kernel executive.

    On-disk layout (512-byte LBA sectors):
        LBA 0  -- Superblock (EVRYFS_SUPER, 512 bytes)
        LBA 1  -- Root directory (12 x EVRYFS_DIRENT, 480 bytes)
        LBA 2+ -- File data (allocated sequentially)

    The driver itself is split into one module per concern:
        ata.c      -- ATA PIO sector read/write
        super.c    -- superblock cache, mount / auto-format
        dirsup.c   -- directory entry accessor, lookup, listing
        allocsup.c -- data-LBA allocation
        read.c     -- EvryFsReadFile
        write.c    -- EvryFsWriteFile
        strsup.c   -- freestanding string helpers
    Only this header is public; cross-module declarations live in
    evryfsp.h and are private to the driver.

Author:

    Noah Juopperi <nipfswd@gmail.com>
    Vlad Lymar <ggcc98765432110@gmail.com>

Environment:

    Kernel-mode only

--*/

#ifndef _EVRYFS_H_
#define _EVRYFS_H_

#include <stdint.h>

/* ---- Constants --------------------------------------------------------- */

#define EVRYFS_MAGIC        0x45565259U     /* 'EVRY' little-endian         */
#define EVRYFS_VERSION      1

#define EVRYFS_SUPER_LBA    0
#define EVRYFS_DIR_LBA      1
#define EVRYFS_DATA_START   2               /* first data LBA                */

#define EVRYFS_MAX_FILES    12              /* entries in one directory sector*/
#define EVRYFS_NAME_LEN     28              /* max filename length incl. NUL  */

/* ---- On-disk structures ------------------------------------------------ */

/*
 * EVRYFS_SUPER -- superblock, exactly 512 bytes.
 */
typedef struct {
    uint32_t magic;             /* EVRYFS_MAGIC                              */
    uint32_t version;           /* EVRYFS_VERSION                            */
    uint32_t next_free_lba;     /* next allocatable data LBA                 */
    uint8_t  reserved[500];
} __attribute__((packed)) EVRYFS_SUPER;

/*
 * EVRYFS_DIRENT -- directory entry, exactly 40 bytes.
 * 12 entries fit in one 512-byte sector (12 * 40 = 480 <= 512).
 */
typedef struct {
    char     name[EVRYFS_NAME_LEN]; /* null-terminated filename              */
    uint32_t flags;                 /* bit 0: 1 = in use, 0 = free           */
    uint32_t start_lba;             /* first data LBA                        */
    uint32_t size;                  /* file size in bytes                    */
} __attribute__((packed)) EVRYFS_DIRENT;

typedef struct _EVRYFS_FILE_CONTEXT {
    char     FileName[EVRYFS_NAME_LEN];
    uint32_t CurrentOffset;
    uint32_t FileSize;
    uint8_t  IsWritable;
} EVRYFS_FILE_CONTEXT, *PEVRY_FILE_CONTEXT;

typedef struct _EVRYFS_DIR_ENTRY_INFO {
    char     FileName[EVRYFS_NAME_LEN];
    uint32_t FileSize;
} EVRYFS_DIR_ENTRY_INFO, *PEVRYFS_DIR_ENTRY_INFO;

/* ---- ATA helpers (ata.c) ----------------------------------------------- */

int AtaReadSector (uint32_t lba,       uint8_t* buf);
int AtaWriteSector(uint32_t lba, const uint8_t* buf);

/* ---- Public filesystem API ----------------------------------------------- */

/*
 * EvryFsInit -- detect disk, read or format superblock + directory.
 * Returns 0 on success, -1 if no disk is present.
 */
int  EvryFsInit(void);

/*
 * EvryFsWriteFile -- create or overwrite a file with the given data.
 * Returns 0 on success, -1 on error (no disk, directory full).
 */
int  EvryFsWriteFile(const char* name, const uint8_t* data, int len);

/*
 * EvryFsReadFile -- read a file into buf (up to maxlen bytes).
 * Returns bytes read, or -1 if the file is not found / no disk.
 */
int  EvryFsReadFile(const char* name, uint8_t* buf, int maxlen);

/*
 * EvryFsList -- fill names[][] and sizes[] with all in-use directory entries.
 * *count receives the number of entries written.
 */
void EvryFsList(char names[][EVRYFS_NAME_LEN], uint32_t sizes[], int* count);

#endif /* _EVRYFS_H_ */
