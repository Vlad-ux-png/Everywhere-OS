/*++

Copyright (c) 2026  O&V Soft. All Rights Reserved.

Module Name:

    ioread.c

Abstract:

    Native file read system service (NtReadFile).

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

    Reads data from an open file object.

Arguments:

    FileObject - Supplies the file object being read from.
    Buffer     - Receives the data read from the file.
    Length     - Supplies the number of bytes to read.
    BytesRead  - Receives the actual number of bytes read.

Return Value:

    NTSTATUS - STATUS_SUCCESS or appropriate error code.

--*/
NTSTATUS
NtReadFile(
    PVOID  FileObject,
    PVOID  Buffer,
    ULONG  Length,
    PULONG BytesRead
)
{
    PEMY_FILE_CONTEXT FileContext;
    int               ReadResult;

    if (FileObject == NULL || Buffer == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Validate object header
    //
    POBJECT_HEADER Header = OBJECT_TO_OBJECT_HEADER(FileObject);
    if (Header == NULL || Header->Type == NULL) {
        return STATUS_OBJECT_TYPE_MISMATCH;
    }

    FileContext = (PEMY_FILE_CONTEXT)FileObject;

    //
    // Read data using EVRYFS read routine
    //
    ReadResult = EvryFsReadFile(FileContext->FileName, (uint8_t*)Buffer, (int)Length);
    if (ReadResult < 0) {
        return STATUS_UNSUCCESSFUL;
    }

    FileContext->CurrentOffset += (ULONG)ReadResult;

    if (BytesRead != NULL) {
        *BytesRead = (ULONG)ReadResult;
    }

    return STATUS_SUCCESS;
}
