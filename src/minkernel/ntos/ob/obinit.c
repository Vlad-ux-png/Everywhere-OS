/*++

Copyright (c) 2026  O&V Soft. All Rights Reserved.

Module Name:

    obinit.c

Abstract:

    Object Manager initialization routines.

Author:

    Vlad Lymar <ggcc98765432110@gmail.com>

Environment:

    Kernel-mode only.

--*/

#include "../inc/ob.h"
#include "../inc/mm.h"
#include "../inc/ke.h"

POBJECT_TYPE ObDirectoryType = NULL;
POBJECT_TYPE ObSymbolicLinkType = NULL;

/*++

Routine Description:

    Initializes the Object Manager subsystem, creating foundational
    object types like Directory and SymbolicLink.

Arguments:

    None.

Return Value:

    None. (Triggers a bugcheck if critical types fail to initialize).

--*/
VOID
ObInitSystem(
    VOID
)
{
    ObDirectoryType = ObCreateObjectType(L"Directory", NonPagedPool);

    ObSymbolicLinkType = ObCreateObjectType(L"SymbolicLink", NonPagedPool);
}
