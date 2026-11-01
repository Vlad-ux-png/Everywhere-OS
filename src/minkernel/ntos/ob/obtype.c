/*++

Copyright (c) 2026  O&V Soft. All Rights Reserved.

Module Name:

    obtype.c

Abstract:

    Object type management and creation routines.

Author:

    Vlad Lymar <ggcc98765432110@gmail.com>

Environment:

    Kernel-mode only.

--*/
#include "../inc/ob.h"

static OBJECT_TYPE FileType;

/*++

Routine Description:

    Creates and initializes a system object type descriptor.

Arguments:

    TypeNameString - Supplies the name of the object type.
    PoolType       - Supplies the pool type from which objects of this
                     type should be allocated.

Return Value:

    Status.
    STATUS_SUCCESS - Object Descriptor Initialized Successfully.

--*/

NTSTATUS
ObCreateObjectType(
    PCWSTR       TypeNameString,
    MM_POOL_TYPE PoolType
)
{
    (VOID)TypeNameString;

    FileType.PoolType                 = PoolType;
    FileType.TotalNumberOfObjects     = 0;
    FileType.TotalNumberOfHandles     = 0;
    FileType.HighWaterNumberOfObjects = 0;
    FileType.HighWaterNumberOfHandles = 0;

    return STATUS_SUCCESS;
}

/*++

Routine Description:

    Retrieves the global File object type descriptor.

Arguments:

    None.

Return Value:

    POBJECT_TYPE - Pointer to the File object type.

--*/
POBJECT_TYPE
ObReferenceObjectTypeByName(
    PCWSTR TypeNameString
)
{
    if (TypeNameString != NULL && wcscmp(TypeNameString, L"File") == 0) {
        return &FileType;
    }
    return NULL;
}

/*++

Routine Description:

    Retrieves the global File object type descriptor.

Arguments:

    None.

Return Value:

    POBJECT_TYPE - Pointer to the File object type.

--*/
POBJECT_TYPE
ObGetFileType(
    VOID
)
{
    return &FileType;
}

/*++

Routine Description:

    Retrieves a system object type descriptor by its name, searching
    among the registered object types.

Arguments:

    TypeNameString - Supplies the name of the object type to look up.

Return Value:

    POBJECT_TYPE - Pointer to the object type descriptor, or NULL if not found.

--*/
POBJECT_TYPE
ObReferenceObjectTypeByName(
    PCWSTR TypeNameString
)
{
    if (TypeNameString == NULL) {
        return NULL;
    }

    if (RtlCompareWcharCaseInsensitive(TypeNameString, "File") == 0) {
        return ObGetFileType();
    }

    return NULL;
}
