/*++

Copyright (c) 2026  O&V Soft. All Rights Reserved.

Module Name:

    create.c

Abstract:

    Native file creation system service (NtCreateFile).

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

    Creates a new file (or overwrites an existing one) in EVRYFS 
    and returns an open kernel file object ready for use.

Arguments:

    FileName      - Supplies the name of the file to create.
    InitialData   - Optional initial data buffer to write upon creation.
    Length        - Size of the initial data in bytes.
    OutFileObject - Receives the pointer to the newly created file object.

Return Value:

    NTSTATUS - STATUS_SUCCESS or appropriate error code.

--*/
NTSTATUS
NtCreateFile(
    PCHAR  FileName,
    PCHAR  InitialData,
    ULONG  Length,
    PPVOID OutFileObject
)
{
    NTSTATUS           Status;
    POBJECT_TYPE       FileType;
    PVOID              FileObject = NULL;
    PEVRY_FILE_CONTEXT FileContext;
    LONG               WriteResult;

    if (FileName == NULL || OutFileObject == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Create or initialize the file on disk using EVRYFS backend.
    //
    PCHAR DataPtr = InitialData ? InitialData : (PCHAR)"";
    WriteResult = EvryFsWriteFile(FileName, DataPtr, (LONG)Length);
    if (WriteResult < 0) {
        return STATUS_UNSUCCESSFUL; 
    }

    //
    // Reference the registered "File" object type
    //
    FileType = ObReferenceObjectTypeByName(L"File");
    if (FileType == NULL) {
        return STATUS_OBJECT_TYPE_MISMATCH;
    }

    //
    // Allocate an object instance from the static object table
    //
    Status = ObCreateObject(FileType, sizeof(EVRYFS_FILE_CONTEXT), &FileObject);
    if (!NT_SUCCESS(Status)) {
        return Status; 
    }

    //
    // Initialize the file context payload
    //
    FileContext = (PEVRY_FILE_CONTEXT)FileObject;

    EvryFsStrCpy(FileContext->FileName, FileName, EVRYFS_NAME_LEN);
    FileContext->CurrentOffset = 0;
    FileContext->FileSize      = Length;
    FileContext->IsWritable    = TRUE;

    //
    // Return the file object pointer to the caller
    //
    *OutFileObject = FileObject;
    return STATUS_SUCCESS;
}
