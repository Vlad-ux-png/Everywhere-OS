/*++

Copyright (c) 2026  Everywhere Computing, Inc. All Rights Reserved.

Module Name:

    mkdisk.c

Abstract:

    Host-side build tool that creates the EVRYFS disk image and packs
    a bunch of shit

    Usage:
        mkdisk <source_ico> <output_disk_img>

    The tool produces a 1 MB raw disk image containing:
        LBA 0  -- EVRYFS superblock (magic + version + next_free_lba)
        LBA 1  -- Root directory (one EVRYFS_DIRENT for folder.ico)
        LBA 2+ -- folder.ico file data (sequential 512-byte sectors)

Author:

    Noah Juopperi <nipfswd@gmail.com>
    Vlad Lymar <ggcc98765432110@gmail.com>

Environment:

    Host (build machine) -- standard C, no kernel headers required.

--*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

//## CONSTANTS STANDARD, (must match base/fs/evryfs/evryfs.h)!!! ##

#define EVRYFS_MAGIC        0x45565259U
#define EVRYFS_VERSION      1
#define EVRYFS_SUPER_LBA    0
#define EVRYFS_DIR_LBA      1
#define EVRYFS_DATA_START   2
#define EVRYFS_MAX_FILES    12
#define EVRYFS_NAME_LEN     29

/* 1 MB raw disk image */
#define DISK_SIZE           (1024 * 1024)
#define SECTOR_SIZE         512
// Blank spaces are boring so I put a useless comment here instead
#define FOLDER_ICO_FS_NAME  "Everywhere\\Res\\F\\folder.ico"

//## ONDISK STRUCTURE ##\\

#pragma pack(push, 1)

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t next_free_lba;
    uint8_t  reserved[500];
} EVRYFS_SUPER;

typedef struct {
    char     name[EVRYFS_NAME_LEN];
    uint32_t flags;
    uint32_t start_lba;
    uint32_t size;
} EVRYFS_DIRENT;

#pragma pack(pop)

//## EP ( da "ENTRY POINT") ##

int main(int argc, char* argv[])
{
    if (argc < 3) {
        fprintf(stderr, "Use it as mkdisk <source_ico> <output_disk_img>\n");
        return 1;
    }

    const char* ico_path  = argv[1];
    const char* disk_path = argv[2];

    // Read source files
    FILE* ico_f = fopen(ico_path, "rb");
    if (!ico_f) {
        fprintf(stderr, "mkdisk: cannot open '%s': ", ico_path);
        perror("");
        return 1;
    }

    fseek(ico_f, 0, SEEK_END);
    long ico_len = ftell(ico_f);
    fseek(ico_f, 0, SEEK_SET);

    if (ico_len <= 0 || ico_len > DISK_SIZE - (EVRYFS_DATA_START * SECTOR_SIZE)) {
        fprintf(stderr, "mkdisk: ICO file too large or empty (%ld bytes)\n", ico_len);
        fclose(ico_f);
        return 1;
    }

    uint8_t* ico_data = (uint8_t*)malloc((size_t)ico_len);
    if (!ico_data) {
        fprintf(stderr, "mkdisk: out of memory\n");
        fclose(ico_f);
        return 1;
    }

    if ((long)fread(ico_data, 1, (size_t)ico_len, ico_f) != ico_len) {
        fprintf(stderr, "mkdisk: read error on '%s'\n", ico_path);
        fclose(ico_f);
        free(ico_data);
        return 1;
    }
    fclose(ico_f);

/////////////////////////////////////////////////////////////////////////FOR-READABILITY
    FILE* disk = fopen(disk_path, "wb");
    if (!disk) {
        fprintf(stderr, "mkdisk: cannot create '%s': ", disk_path);
        perror("");
        free(ico_data);
        return 1;
    }

    {
        uint8_t zero[SECTOR_SIZE];
        memset(zero, 0, SECTOR_SIZE);
        int total_sectors = DISK_SIZE / SECTOR_SIZE;
        for (int i = 0; i < total_sectors; i++) {
            if (fwrite(zero, 1, SECTOR_SIZE, disk) != SECTOR_SIZE) {
                fprintf(stderr, "mkdisk: write error while zeroing disk\n");
                fclose(disk);
                free(ico_data);
                return 1;
            }
        }
    }

    // Write Superblock at LBA0 (hehe)
    {
        uint8_t sec[SECTOR_SIZE];
        memset(sec, 0, SECTOR_SIZE);

        EVRYFS_SUPER* super = (EVRYFS_SUPER*)sec;
        super->magic         = EVRYFS_MAGIC;
        super->version       = EVRYFS_VERSION;

        int ico_sectors = (int)(((long)ico_len + SECTOR_SIZE - 1) / SECTOR_SIZE);
        super->next_free_lba = (uint32_t)(EVRYFS_DATA_START + ico_sectors);

        fseek(disk, (long)EVRYFS_SUPER_LBA * SECTOR_SIZE, SEEK_SET);
        fwrite(sec, 1, SECTOR_SIZE, disk);
    }

    // Write root dir at LBA 1
    {
        uint8_t sec[SECTOR_SIZE];
        memset(sec, 0, SECTOR_SIZE);

        EVRYFS_DIRENT* d = (EVRYFS_DIRENT*)sec;
        strncpy(d->name, FOLDER_ICO_FS_NAME, EVRYFS_NAME_LEN - 1);
        d->name[EVRYFS_NAME_LEN - 1] = '\0';
        d->flags     = 1;                            /* in-use            */
        d->start_lba = (uint32_t)EVRYFS_DATA_START;
        d->size      = (uint32_t)ico_len;

        fseek(disk, (long)EVRYFS_DIR_LBA * SECTOR_SIZE, SEEK_SET);
        fwrite(sec, 1, SECTOR_SIZE, disk);
    }

    // Write it
    {
        uint8_t sec[SECTOR_SIZE];
        int ico_sectors = (int)(((long)ico_len + SECTOR_SIZE - 1) / SECTOR_SIZE);

        fseek(disk, (long)EVRYFS_DATA_START * SECTOR_SIZE, SEEK_SET);

        for (int s = 0; s < ico_sectors; s++) {
            memset(sec, 0, SECTOR_SIZE);
            long offset = (long)s * SECTOR_SIZE;
            long chunk  = ico_len - offset;
            if (chunk > SECTOR_SIZE) chunk = SECTOR_SIZE;
            memcpy(sec, ico_data + offset, (size_t)chunk);
            fwrite(sec, 1, SECTOR_SIZE, disk);
        }
    }

    fclose(disk);
    free(ico_data);

    printf("mkdisk: packed '%s' (%ld bytes) -> '%s' as '%s'\n",
           ico_path, ico_len, disk_path, FOLDER_ICO_FS_NAME);
    return 0;
}
