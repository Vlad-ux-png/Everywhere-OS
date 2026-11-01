/*++

Copyright (c) 2026  O&V Soft. All Rights Reserved.

Module Name:

    gentable.c

Abstract:

    This module implements generic table routines.

Author:

    Vlad Lymar <ggcc98765432110@gmail.com>

Environment:

    Kernel-mode.

--*/

#include "../inc/ntdef.h"
#include "../inc/rtl.h"

#define GENTABLE_POOL_SIZE (64 * 1024) 

static UCHAR GenTablePool[GENTABLE_POOL_SIZE];
static ULONG GenTablePoolOffset = 0;

VOID
RtlInitializeGenericTable(
    PRTL_GENERIC_TABLE          Table,
    PRTL_GENERIC_COMPARE_ROUTINE CompareRoutine,
    PRTL_GENERIC_ALLOCATE_ROUTINE AllocateRoutine,
    PRTL_GENERIC_FREE_ROUTINE     FreeRoutine,
    PVOID                       TableContext
    )
{
    //
    // Initialize the generic table header fields matching RTL_GENERIC_TABLE layout
    //
    Table->TableRoot = NULL;
    InitializeListHead(&Table->LLHead);
    Table->LastElementFound = NULL;
    Table->LastIndexFound = 0;
    Table->NumberOfElements = 0;
    Table->CompareRoutine = (PVOID)CompareRoutine;
    Table->AllocateRoutine = (PVOID)AllocateRoutine;
    Table->FreeRoutine = (PVOID)FreeRoutine;
    Table->TableContext = TableContext;
    Table->LastElementIndex = 0;
}

ULONG
RtlNumberGenericTableElements(
    PRTL_GENERIC_TABLE Table
    )
{
    return Table->NumberOfElements;
}

BOOLEAN
RtlIsGenericTableEmpty(
    PRTL_GENERIC_TABLE Table
    )
{
    return (Table->NumberOfElements == 0);
}

/* -----------------------------------------------------------------------
 * Default Pool Allocator Bridges 
 * ----------------------------------------------------------------------- */

PVOID
NTAPI
RtlGenericTableAllocate(
    PRTL_GENERIC_TABLE Table,
    CLONG ByteSize
)
{
    PVOID Ptr;
    ULONG AlignedSize;
    PUCHAR BytePtr;
    CLONG i;

    (VOID)Table;

    if (ByteSize == 0) {
        return NULL;
    }

    AlignedSize = (ByteSize + 7) & ~7;

    if (GenTablePoolOffset + AlignedSize > GENTABLE_POOL_SIZE) {
        return NULL;
    }

    Ptr = &GenTablePool[GenTablePoolOffset];
    GenTablePoolOffset += AlignedSize;

    BytePtr = (PUCHAR)Ptr;
    for (i = 0; i < ByteSize; i++) {
        BytePtr[i] = 0;
    }

    return Ptr;
}

VOID
NTAPI
RtlGenericTableFree(
    PRTL_GENERIC_TABLE Table,
    PVOID Buffer
)
{
    (VOID)Table;
    (VOID)Buffer;
}

LONG
NTAPI
RtlGenericTableCompare(
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
