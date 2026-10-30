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

PVOID
RtlInsertElementGenericTable(
    PRTL_GENERIC_TABLE Table,
    PVOID              Buffer,
    ULONG              BufferSize,
    PBOOLEAN           NewElement
    )
{
    PRTL_TABLE_NODE Current;
    PRTL_TABLE_NODE Parent = NULL;
    PRTL_TABLE_NODE NewNode;
    LONG            CompareResult = 0;
    PRTL_GENERIC_COMPARE_ROUTINE CompareRoutine;
    PRTL_GENERIC_ALLOCATE_ROUTINE AllocateRoutine;
    PRTL_GENERIC_FREE_ROUTINE FreeRoutine;

    if (NewElement != NULL) {
        *NewElement = FALSE;
    }

    AllocateRoutine = (PRTL_GENERIC_ALLOCATE_ROUTINE)Table->AllocateRoutine;
    FreeRoutine = (PRTL_GENERIC_FREE_ROUTINE)Table->FreeRoutine;
    CompareRoutine = (PRTL_GENERIC_COMPARE_ROUTINE)Table->CompareRoutine;

    //
    // Allocate memory for the new node including the user buffer size
    //
    NewNode = (PRTL_TABLE_NODE)AllocateRoutine(Table, sizeof(RTL_TABLE_NODE) - 1 + BufferSize);
    if (NewNode == NULL) {
        return NULL;
    }

    RtlCopyMemory(NewNode->Buffer, Buffer, BufferSize);
    NewNode->Left = NULL;
    NewNode->Right = NULL;
    NewNode->Parent = NULL;
    NewNode->BalanceFactor = 0;

    //
    // If the table is empty, insert as root
    //
    if (Table->TableRoot == NULL) {
        Table->TableRoot = NewNode;
        Table->NumberOfElements++;
        Table->LastElementFound = NewNode->Buffer;
        if (NewElement != NULL) {
            *NewElement = TRUE;
        }
        return NewNode->Buffer;
    }

    //
    // Traverse the tree to find the insertion point (Standard BST insertion)
    //
    Current = (PRTL_TABLE_NODE)Table->TableRoot;
    while (Current != NULL) {
        Parent = Current;
        CompareResult = CompareRoutine(Table, NewNode->Buffer, Current->Buffer);

        if (CompareResult < 0) {
            Current = Current->Left;
        } else if (CompareResult > 0) {
            Current = Current->Right;
        } else {
            //
            // Element already exists in the table. Free the allocated node and return existing.
            //
            FreeRoutine(Table, NewNode);
            if (NewElement != NULL) {
                *NewElement = FALSE;
            }
            Table->LastElementFound = Current->Buffer;
            return Current->Buffer;
        }
    }

    //
    // Attach the new node to its parent
    //
    NewNode->Parent = Parent;
    if (CompareResult < 0) {
        Parent->Left = NewNode;
    } else {
        Parent->Right = NewNode;
    }

    Table->NumberOfElements++;
    Table->LastElementFound = NewNode->Buffer;
    if (NewElement != NULL) {
        *NewElement = TRUE;
    }

    return NewNode->Buffer;
}

BOOLEAN
RtlDeleteElementGenericTable(
    PRTL_GENERIC_TABLE Table,
    PVOID              Buffer
    )
{
    PRTL_TABLE_NODE Current;
    LONG            CompareResult;
    PRTL_GENERIC_COMPARE_ROUTINE CompareRoutine;
    PRTL_GENERIC_FREE_ROUTINE FreeRoutine;

    if (Table->TableRoot == NULL || Table->NumberOfElements == 0) {
        return FALSE;
    }

    CompareRoutine = (PRTL_GENERIC_COMPARE_ROUTINE)Table->CompareRoutine;
    FreeRoutine = (PRTL_GENERIC_FREE_ROUTINE)Table->FreeRoutine;

    //
    // Search for the element to delete
    //
    Current = (PRTL_TABLE_NODE)Table->TableRoot;
    while (Current != NULL) {
        CompareResult = CompareRoutine(Table, Buffer, Current->Buffer);

        if (CompareResult < 0) {
            Current = Current->Left;
        } else if (CompareResult > 0) {
            Current = Current->Right;
        } else {
            //
            // Node found. Perform removal
            //
            if (Table->LastElementFound == Current->Buffer) {
                Table->LastElementFound = NULL;
            }

            FreeRoutine(Table, Current);
            Table->NumberOfElements--;
            return TRUE;
        }
    }

    return FALSE;
}

PVOID
RtlLookupElementGenericTable(
    PRTL_GENERIC_TABLE Table,
    PVOID              Buffer
    )
{
    PRTL_TABLE_NODE Current;
    LONG            CompareResult;
    PRTL_GENERIC_COMPARE_ROUTINE CompareRoutine;

    if (Table->TableRoot == NULL) {
        return NULL;
    }

    CompareRoutine = (PRTL_GENERIC_COMPARE_ROUTINE)Table->CompareRoutine;
    Current = (PRTL_TABLE_NODE)Table->TableRoot;

    while (Current != NULL) {
        CompareResult = CompareRoutine(Table, Buffer, Current->Buffer);

        if (CompareResult < 0) {
            Current = Current->Left;
        } else if (CompareResult > 0) {
            Current = Current->Right;
        } else {
            Table->LastElementFound = Current->Buffer;
            return Current->Buffer;
        }
    }

    return NULL;
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
    (VOID)Table;
    return NULL;
}

VOID
NTAPI
RtlGenericTableFree(
    PRTL_GENERIC_TABLE Table,
    PVOID Buffer
)
{
    (VOID)Table;
}
