/*++

Copyright (c) 2026  O&V Soft. All Rights Reserved.

Module Name:

    obcreate.c

Abstract:

    Object creation routines.

Author(s):

    Vlad Lymar <ggcc98765432110@gmail.com>

Environment:

    Kernel-mode only.

--*/

#include "../inc/ob.h"

//
// Maximum number of statically allocated kernel objects per system boot
//

_OBJECT_SLOT ObpObjectTable[MAX_OBJECTS];

/*++

Routine Description:

    Allocates and initializes a kernel object from the static pool.
    Attaches the OBJECT_HEADER, updates the corresponding OBJECT_TYPE 
    statistics, and returns a pointer directly to the payload area.

Arguments:

    ObjectType - Pointer to the registered object type (e.g., FileType).

    ObjectSize - Size of the caller's object payload in bytes.

    Object     - Receives the pointer to the created object payload body.

Return Value:

    NTSTATUS - STATUS_SUCCESS on allocation, or appropriate error code.

--*/
NTSTATUS
ObCreateObject(
    POBJECT_TYPE ObjectType,
    ULONG        ObjectSize,
    PVOID       *Object
    )
{
    ULONG               i;
    POBJECT_SLOT Slot = NULL;
    POBJECT_HEADER      Header;

    if (ObjectType == NULL || Object == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    if (ObjectSize > MAXIMUM_OBJECT_PAYLOAD_SIZE) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Locate an unallocated slot in the static object table
    //
    for (i = 0; i < MAX_OBJECTS; i++) {
        if (!ObpObjectTable[i].InUse) {
            Slot = &ObpObjectTable[i];
            Slot->InUse = TRUE;
            break;
        }
    }

    if (Slot == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Populate the OBJECT_HEADER preceding the payload
    //
    Header = &Slot->Header;
    Header->PointerCount = 1;
    Header->HandleCount  = 0;
    Header->Type         = ObjectType;
    Header->Attributes   = 0;

    //
    // Update object type statistics
    //
    ObjectType->TotalNumberOfObjects++;
    if (ObjectType->TotalNumberOfObjects > ObjectType->HighWaterNumberOfObjects) {
        ObjectType->HighWaterNumberOfObjects = ObjectType->TotalNumberOfObjects;
    }

    //
    // Zero payload area for safety
    //
    RtlZeroMemory(Slot->Payload, ObjectSize);

    //
    // Return pointer to the payload area past the header
    //
    *Object = OBJECT_HEADER_TO_OBJECT(Header);

    return STATUS_SUCCESS;
}
