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
static OBJECT_TYPE FileType;

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
