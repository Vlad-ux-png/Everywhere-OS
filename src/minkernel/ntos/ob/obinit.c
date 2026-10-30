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

POBJECT_TYPE ObDirectoryType;
POBJECT_TYPE ObSymbolicLinkType;

VOID
ObInitSystem(
    VOID
)
{
    ObDirectoryType = NULL;
    ObSymbolicLinkType = NULL;
}
