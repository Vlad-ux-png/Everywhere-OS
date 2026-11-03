/*++

Copyright (c) 2026  O&V Soft. All Rights Reserved.

Module Name:

    obref.c

Abstract:

    Object reference counting and routines using dynamic pool allocation.

Author(s):

    Vlad Lymar <ggcc98765432110@gmail.com>

Environment:

    Kernel-mode only.

--*/

#include "../inc/ob.h"
#include "../inc/mm.h"
#include "../inc/rtl.h"

//
// Pool tag matching object creation (must be consistent)
//
#define OBJECT_POOL_TAG  0x4A4F626A // 'JObj'

/*++

Routine Description:

    Increments the reference count (PointerCount) of a kernel object.
    This prevents the object from being freed while in use.

Arguments:

    Object - Pointer to the object payload body.

Return Value:

    LONG - The new reference count value.

--*/
LONG
ObReferenceObject(
    PVOID Object
)
{
    POBJECT_HEADER Header;

    if (Object == NULL) {
        return 0;
    }

    //
    // Get the header preceding the object payload using ob.h macro
    //
    Header = OBJECT_TO_OBJECT_HEADER(Object);

    //
    // Increment pointer count. 
    //
    Header->PointerCount++;

    return Header->PointerCount;
}

/*++

Routine Description:

    Decrements the reference count (PointerCount) of a kernel object.
    If the count drops to zero, the object is automatically destroyed 
    and its memory block is freed back to the kernel pool.

Arguments:

    Object - Pointer to the object payload body.

Return Value:

    LONG - The new reference count value (0 if destroyed).

--*/
LONG
ObDereferenceObject(
    PVOID Object
)
{
    POBJECT_HEADER Header;
    LONG           NewCount;

    if (Object == NULL) {
        return 0;
    }

    Header = OBJECT_TO_OBJECT_HEADER(Object);

    //
    // Decrement pointer count
    //
    Header->PointerCount--;
    NewCount = Header->PointerCount;

    //
    // If no more references remain, clean up and free the object memory
    //
    if (NewCount <= 0) {
        //
        // Update type statistics safely
        //
        if (Header->Type != NULL && Header->Type->TotalNumberOfObjects > 0) {
            Header->Type->TotalNumberOfObjects--;
        }

        //
        // Free the entire dynamic block (header + payload) back to the pool
        //
        MmFreePool(Header, OBJECT_POOL_TAG);
    }

    return NewCount;
}
