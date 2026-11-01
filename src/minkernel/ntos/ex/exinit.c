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
#include "../inc/ob.h"
#include "kdcomp.h"

extern LARGE_INTEGER ExpLuid;

RTL_GENERIC_TABLE ExpSystemTable;

BOOLEAN
ExInitSystem(
    VOID
    )
{
    ExpLuid.QuadPart = 1000;

    RtlInitializeGenericTable(
        &ExpSystemTable,
        (PVOID)RtlGenericTableCompare,         
        (PVOID)RtlGenericTableAllocate,
        (PVOID)RtlGenericTableFree,   
        NULL                       
    );

    ObInitSystem();

    ExpInitializeWorkItems();
    
    return TRUE;
}
