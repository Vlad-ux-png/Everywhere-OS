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

extern LARGE_INTEGER ExpLuid;

RTL_GENERIC_TABLE ExpSystemTable;

LONG
NTAPI
TableCompare(
    PRTL_GENERIC_TABLE Table,
    PVOID First,
    PVOID Second
)
{
    ULONG key1 = *(PULONG)First;
    ULONG key2 = *(PULONG)Second;

    if (key1 < key2) {
        return -1;
    } else if (key1 > key2) {
        return 1;
    }
    return 0; 
}

BOOLEAN
ExInitSystem(
    VOID
    )
{
    ExpLuid.QuadPart = 1000;

    RtlInitializeGenericTable(
        &ExpSystemTable,
        (PVOID)TableCompare,         
        (PVOID)RtlGenericTableAllocate,
        (PVOID)RtlGenericTableFree,   
        NULL                       
    );

    ObInitSystem();
    
    return TRUE;
}
