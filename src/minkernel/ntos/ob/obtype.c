/*++

Copyright (c) 2026  O&V Soft. All Rights Reserved.

Module Name:

    obtype.c

Abstract:

    Dynamic object type management and creation routines.

Author(s):

    Vlad Lymar <ggcc98765432110@gmail.com>

Environment:

    Kernel-mode only.

--*/

#include "../inc/ob.h"
#include "../inc/mm.h"
#include "../inc/rtl.h"

//
// Define a tag for object type allocations
//
#define OB_TYPE_POOL_TAG  0x4F626A54 // 'ObjT'

//
// Internal node structure for the object type list
//
typedef struct _OBJECT_TYPE_NODE {
    OBJECT_TYPE               TypeInfo;
    struct _OBJECT_TYPE_NODE* Next;
} OBJECT_TYPE_NODE, *POBJECT_TYPE_NODE;

//
// Head of the registered object types list
//
static POBJECT_TYPE_NODE ObpTypeListHead = NULL;

/*++

Routine Description:

    Creates and initializes a system object type descriptor dynamically.

Arguments:

    TypeNameString - Supplies the name of the object type.
    PoolType       - Supplies the pool type from which objects of this
                     type should be allocated.

Return Value:

    NTSTATUS - Status of the operation.

--*/
NTSTATUS
ObCreateObjectType(
    PCWSTR       TypeNameString,
    MM_POOL_TYPE PoolType
)
{
    POBJECT_TYPE_NODE NewNode;

    if (TypeNameString == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Allocate memory for the new object type node
    //
    NewNode = (POBJECT_TYPE_NODE)MmAllocatePool(PoolType, sizeof(OBJECT_TYPE_NODE), OB_TYPE_POOL_TAG);
    if (NewNode == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Initialize the object type fields
    //
    RtlInitUnicodeString(&NewNode->TypeInfo.TypeName, TypeNameString);

    NewNode->TypeInfo.PoolType                 = PoolType;
    NewNode->TypeInfo.TotalNumberOfObjects     = 0;
    NewNode->TypeInfo.TotalNumberOfHandles     = 0;
    NewNode->TypeInfo.HighWaterNumberOfObjects = 0;
    NewNode->TypeInfo.HighWaterNumberOfHandles = 0;

    //
    // Insert the new type into the global object types list
    //
    NewNode->Next   = ObpTypeListHead;
    ObpTypeListHead = NewNode;

    return STATUS_SUCCESS;
}

/*++

Routine Description:

    Retrieves a system object type descriptor by its name, searching
    through the dynamically registered object types list.

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
    POBJECT_TYPE_NODE Current;

    if (TypeNameString == NULL) {
        return NULL;
    }

    Current = ObpTypeListHead;
    while (Current != NULL) {
        //
        // Compare case-insensitively using RTL string helpers
        //
        if (RtlCompareWcharCaseInsensitive(TypeNameString, Current->TypeInfo.TypeName.Buffer) == 0) {
            return &Current->TypeInfo;
        }
        Current = Current->Next;
    }

    return NULL;
}
