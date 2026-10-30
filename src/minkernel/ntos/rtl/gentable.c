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

VOID
RtlInitializeGenericTable(
    PRTL_GENERIC_TABLE Table,
    PVOID CompareRoutine,
    PVOID AllocateRoutine,
    PVOID FreeRoutine,
    PVOID TableContext
)
{
    PLIST_ENTRY listHead = &Table->LLHead;

    Table->TableRoot = NULL;
    listHead->Flink = listHead;
    listHead->Blink = listHead;
    Table->LastElementFound = listHead;
    Table->LastIndexFound = 0;
    Table->NumberOfElements = 0;
    Table->CompareRoutine = CompareRoutine;
    Table->AllocateRoutine = AllocateRoutine;
    Table->FreeRoutine = FreeRoutine;
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
    return (Table->TableRoot == NULL) ? TRUE : FALSE;
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
    (VOID)Table;
    return MmAllocatePool(NonPagedPool, ByteSize, 0x54624C67); // 'TbLg'
}

VOID
NTAPI
RtlGenericTableFree(
    PRTL_GENERIC_TABLE Table,
    PVOID Buffer
)
{
    (VOID)Table;
    MmFreePool(Buffer, 0x54624C67); // 'TbLg'
}
