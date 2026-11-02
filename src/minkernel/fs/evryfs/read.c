/*++

Copyright (c) 2026  Everywhere Computing, Inc. All Rights Reserved.

Module Name:

    read.c

Abstract:

    EVRYFS file read path.

Author:

    Noah Juopperi <nipfswd@gmail.com>
    Vlad Lymar <ggcc98765432110@gmail.com>

Environment:

    Kernel-mode only

--*/

#include "evryfsp.h"

int EvryFsReadFile(const char* name, uint8_t* buf, int maxlen)
{
    if (!g_FsPresent) return -1;

    int idx = EvryFsFindEntry(name);
    if (idx < 0) return -1;

    EVRYFS_DIRENT* d = EvryFsDirEntry(idx);

    int total = (int)d->size;
    if (total > maxlen) total = maxlen;
    int sectors = (total + 511) / 512;
    uint8_t sector_buf[512];
    int     copied  = 0;

    for (int sec = 0; sec < sectors && copied < total; sec++) {
        if (AtaReadSector(d->start_lba + (uint32_t)sec, sector_buf) < 0)
            return -1;
        int chunk = total - copied;
        if (chunk > 512) chunk = 512;
        for (int b = 0; b < chunk; b++) buf[copied + b] = sector_buf[b];
        copied += chunk;
    }
    return copied;
}
