/*++

Copyright (c) 2026  Everywhere Computing, Inc. All Rights Reserved.

Module Name:

    exinit.c

Abstract:

    Executive subsystem initialization.

Author:

    Noah Juopperi <nipfswd@gmail.com>
    Vlad Lymar <ggcc98765432110@gmail.com>

Environment:

    Kernel-mode only.

--*/

#include "../inc/ex.h"

extern LARGE_INTEGER ExpLuid;

BOOLEAN
ExInitSystem(
    VOID
    )
{
    ExpLuid.QuadPart = 1000;

    ExpInitializeWorkItems();
    
    return TRUE;
}
