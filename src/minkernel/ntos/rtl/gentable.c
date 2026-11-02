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
    PRTL_GENERIC_TABLE          Table,
    PRTL_GENERIC_COMPARE_ROUTINE CompareRoutine,
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
 * Default Bridges 
 * ----------------------------------------------------------------------- */

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
