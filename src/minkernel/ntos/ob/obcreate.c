/*++

Copyright (c) 2026  O&V Soft. All Rights Reserved.

Module Name:

    obcreate.c

Abstract:

    Dynamic object creation routines using kernel pool allocation.

Author(s):

    Vlad Lymar <ggcc98765432110@gmail.com>

Environment:

    Kernel-mode only.

--*/

#include "../inc/ob.h"
#include "../inc/mm.h"
#include "../inc/rtl.h"

/*++

Routine Description:

    Allocates and initializes a kernel object dynamically from the kernel pool.
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
    POBJECT_HEADER Header;
    ULONG          TotalAllocationSize;
    PUCHAR         AllocatedMemory;

    if (ObjectType == NULL || Object == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    if (ObjectSize > MAXIMUM_OBJECT_PAYLOAD_SIZE) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Calculate total memory needed: OBJECT_HEADER + requested payload size
    //
    TotalAllocationSize = sizeof(OBJECT_HEADER) + ObjectSize;

    //
    // Allocate memory from the RTL heap
    //
    AllocatedMemory = (PUCHAR)RtlAllocateHeap(TotalAllocationSize);
    if (AllocatedMemory == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // The header resides at the very beginning of the allocated block
    //
    Header = (POBJECT_HEADER)AllocatedMemory;
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
    // Get pointer to the payload area immediately following the header
    //
    PVOID Payload = OBJECT_HEADER_TO_OBJECT(Header);

    //
    // Zero payload area for safety
    //
    RtlZeroMemory(Payload, ObjectSize);

    //
    // Return pointer to the payload area
    //
    *Object = Payload;

    return STATUS_SUCCESS;
}
