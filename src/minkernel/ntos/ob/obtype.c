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
#include "../inc/mm.h"

/*++

Routine Description:

    Creates and initializes a system object type descriptor.

Arguments:

    TypeNameString - Supplies the name of the object type.
    PoolType       - Supplies the pool type from which objects of this
                     type should be allocated.

Return Value:

    Pointer to the newly created OBJECT_TYPE structure, or NULL on failure.

--*/
POBJECT_TYPE
ObCreateObjectType(
    PCWSTR       TypeNameString,
    MM_POOL_TYPE PoolType
)
{
    POBJECT_TYPE ObjectType;

    (VOID)TypeNameString;

    //
    // Initialize basic object type tracking fields
    //
    ObjectType->PoolType                 = PoolType;
    ObjectType->TotalNumberOfObjects     = 0;
    ObjectType->TotalNumberOfHandles     = 0;
    ObjectType->HighWaterNumberOfObjects = 0;
    ObjectType->HighWaterNumberOfHandles = 0;

    return ObjectType;
}
