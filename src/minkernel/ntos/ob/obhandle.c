/*++

Copyright (c) 2026  O&V Soft. All Rights Reserved.

Module Name:

    obhandle.c

Abstract:

    Object Manager object creation, reference counting, and handle management.

Author:

    Vlad Lymar <ggcc98765432110@gmail.com>

Environment:

    Kernel-mode only.

--*/

#include "../inc/ob.h"
#include "../inc/mm.h"

VOID
ObReferenceObject(
    IN PVOID Object
)
{
    POBJECT_HEADER Header;

    if (Object != NULL) {
        Header = OBJECT_TO_OBJECT_HEADER(Object);
        Header->PointerCount++;
    }
}
