/*++

Copyright (c) 2026  O&V Soft. All Rights Reserved.

Module Name:

    iowrite.c

Abstract:

    Native file write system service (NtWriteFile).

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

    Writes data to an open file object.

Arguments:

    FileObject   - Supplies the file object being written to.
    Buffer       - Supplies the data to write into the file.
    Length       - Supplies the number of bytes to write.
    BytesWritten - Receives the actual number of bytes written.

Return Value:

    NTSTATUS - STATUS_SUCCESS or appropriate error code.

--*/
NTSTATUS
NtWriteFile(
    PVOID  FileObject,
    PVOID  Buffer,
    ULONG  Length,
    PULONG BytesWritten
)
{
    PEMY_FILE_CONTEXT FileContext;
    int               WriteResult;

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

    if (!FileContext->IsWritable) {
        return STATUS_ACCESS_DENIED;
    }

    //
    // Write data using EVRYFS write routine
    //
    WriteResult = EvryFsWriteFile(FileContext->FileName, (const uint8_t*)Buffer, (int)Length);
    if (WriteResult < 0) {
        return STATUS_UNSUCCESSFUL;
    }

    FileContext->FileSize      = Length;
    FileContext->CurrentOffset += Length;

    if (BytesWritten != NULL) {
        *BytesWritten = Length;
    }

    return STATUS_SUCCESS;
}
