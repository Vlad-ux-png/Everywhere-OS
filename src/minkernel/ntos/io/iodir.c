/*++

Copyright (c) 2026  O&V Soft. All Rights Reserved.

Module Name:

    iodir.c

Abstract:

    Directory listing system service (NtQueryDirectory).

Author(s):

    Vlad Lymar <ggcc98765432110@gmail.com>

Environment:

    Kernel-mode only.

--*/

#include "../inc/ntdef.h"
#include "../inc/ob.h"
#include "evryfsp.h"

/*++

Routine Description:

    Queries the root directory of EVRYFS, retrieving a list of all files 
    along with their sizes into the provided output buffer.

Arguments:

    FileObject   - Supplies the directory or volume file object (optional in flat fs).
    Buffer       - Receives an array of EVRYFS_DIR_ENTRY_INFO structures.
    Length       - Supplies the size of the buffer in bytes.
    EntriesCount - Receives the number of entries written.

Return Value:

    NTSTATUS - STATUS_SUCCESS or appropriate error code.

--*/
NTSTATUS
NtQueryDirectory(
    PVOID  FileObject,
    PVOID  Buffer,
    ULONG  Length,
    PULONG EntriesCount
)
{
    char     Names[EVRYFS_MAX_FILES][EVRYFS_NAME_LEN];
    uint32_t Sizes[EVRYFS_MAX_FILES];
    int      Count = 0;
    ULONG    RequiredLength;
    PEVRYFS_DIR_ENTRY_INFO OutEntries;

    (VOID)FileObject;

    if (Buffer == NULL || EntriesCount == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Call EVRYFS core list routine to gather names and sizes
    //
    EvryFsList(Names, Sizes, &Count);

    //
    // Calculate required buffer space
    //
    RequiredLength = (ULONG)(Count * sizeof(EVRYFS_DIR_ENTRY_INFO));
    if (Length < RequiredLength) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    OutEntries = (PEVRYFS_DIR_ENTRY_INFO)Buffer;

    //
    // Copy data into the caller's output buffer
    //
    for (int i = 0; i < Count; i++) {
        EvryFsStrCpy(OutEntries[i].FileName, Names[i], EVRYFS_NAME_LEN);
        OutEntries[i].FileSize = Sizes[i];
    }

    *EntriesCount = (ULONG)Count;

    return STATUS_SUCCESS;
}
