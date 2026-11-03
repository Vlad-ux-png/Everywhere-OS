/*++

Copyright (c) 2026  O&V Soft. All Rights Reserved.

Module Name:

    iofile.c

Abstract:

    File object management services: NtOpenFile and NtCloseFile.

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

    Opens an existing file from EVRYFS and creates a kernel file object.

Arguments:

    FileName      - Supplies the name of the file to open.
    OutFileObject - Receives the pointer to the newly created file object.

Return Value:

    NTSTATUS - STATUS_SUCCESS or appropriate error code.

--*/
NTSTATUS
NtOpenFile(
    const char* FileName,
    PVOID*      OutFileObject
)
{
    NTSTATUS          Status;
    POBJECT_TYPE      FileType;
    PVOID             FileObject = NULL;
    PEMY_FILE_CONTEXT FileContext;
    int               EntryIdx;

    if (FileName == NULL || OutFileObject == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Check if the file exists in EVRYFS
    //
    EntryIdx = EvryFsFindEntry(FileName);
    if (EntryIdx < 0) {
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    //
    // Reference the "File" object type registered in the system
    //
    FileType = ObReferenceObjectTypeByName(L"File");
    if (FileType == NULL) {
        return STATUS_OBJECT_TYPE_MISMATCH;
    }

    //
    // Create an object instance using the static object manager
    //
    Status = ObCreateObject(FileType, sizeof(EVRYFS_FILE_CONTEXT), &FileObject);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    //
    // Initialize file context payload from directory entry
    //
    EVRYFS_DIRENT* dirent = EvryFsDirEntry(EntryIdx);
    FileContext = (PEMY_FILE_CONTEXT)FileObject;

    EvryFsStrCpy(FileContext->FileName, dirent->name, EVRYFS_NAME_LEN);
    FileContext->CurrentOffset = 0;
    FileContext->FileSize      = dirent->size;
    FileContext->IsWritable    = TRUE;

    *OutFileObject = FileObject;
    return STATUS_SUCCESS;
}

/*++

Routine Description:

    Closes an open file object by decrementing its reference count.
    When the reference count reaches zero, the object is automatically destroyed.

Arguments:

    FileObject - Supplies the file object to close.

Return Value:

    NTSTATUS - STATUS_SUCCESS or STATUS_INVALID_PARAMETER.

--*/
NTSTATUS
NtCloseFile(
    PVOID FileObject
)
{
    if (FileObject == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Decrement the object reference count
    //
    ObDereferenceObject(FileObject);

    return STATUS_SUCCESS;
}
