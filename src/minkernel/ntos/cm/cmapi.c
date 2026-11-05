/*++

Copyright (c) 2026  O&V Soft. All Rights Reserved.

Module Name:

    cmapi.c

Abstract:

    This module implements core API primitives for managing registry keys and values.

Author(s):

    Vlad Lymar <ggcc98765432110@gmail.com>

Environment:

    Kernel-mode only.

--*/

#include "cm.h"


/*++

Routine Description:

    CmCreateKey creates a new child registry key under the specified parent node.

Arguments:

    Parent - Specifies the parent key node.
    Name - Specifies the name of the new key (must be a persistent string literal).

Return Value:

    Returns a pointer to the newly created CM_KEY_NODE structure, or NULL on failure.

--*/
PCM_KEY_NODE
CmCreateKey(
    PCM_KEY_NODE Parent,
    PCSTR Name
)
{
    if (!Name) return NULL;

    PCM_KEY_NODE NewKey = (PCM_KEY_NODE)MmAllocatePool(NonPagedPool, sizeof(CM_KEY_NODE), CM_POOL_TAG);
    if (!NewKey) return NULL;

    NewKey->Name = Name;
    NewKey->Parent = Parent;
    NewKey->Child = NULL;
    NewKey->Sibling = NULL;
    NewKey->Values = NULL;

    if (Parent) {
        if (!Parent->Child) {
            Parent->Child = NewKey;
        } else {
            PCM_KEY_NODE Current = Parent->Child;
            while (Current->Sibling) {
                Current = Current->Sibling;
            }
            Current->Sibling = NewKey;
        }
    }

    return NewKey;
}


/*++

Routine Description:

    CmSetValueKey creates or updates a value entry under the specified key node.

Arguments:

    Key - Specifies the target registry key node.
    Name - Specifies the name of the value entry (must be a persistent string literal).
    Type - Specifies the registry data type (e.g., REG_SZ, REG_DWORD).
    Data - Pointer to the raw value data.
    DataSize - Size of the data in bytes.

Return Value:

    None.

--*/
VOID
CmSetValueKey(
    PCM_KEY_NODE Key,
    PCSTR Name,
    ULONG Type,
    PVOID Data,
    ULONG DataSize
)
{
    if (!Key || !Name) return;

    PCM_VALUE_ENTRY NewVal = (PCM_VALUE_ENTRY)MmAllocatePool(NonPagedPool, sizeof(CM_VALUE_ENTRY), CM_POOL_TAG);
    if (!NewVal) return;

    NewVal->Name = Name;
    NewVal->Type = Type;
    NewVal->DataSize = DataSize;

    if (Data && DataSize > 0) {
        NewVal->Data = MmAllocatePool(NonPagedPool, DataSize, CM_POOL_TAG);
        if (NewVal->Data) {
            PSTR Dest = (PSTR)NewVal->Data;
            PCSTR Src = (PCSTR)Data;
            for (ULONG i = 0; i < DataSize; i++) {
                Dest[i] = Src[i];
            }
        }
    } else {
        NewVal->Data = NULL;
    }

    NewVal->Next = Key->Values;
    Key->Values = NewVal;
}


/*++

Routine Description:

    CmQueryValueKey searches for a value entry by name within the specified key.

Arguments:

    Key - Specifies the target registry key node.
    Name - Specifies the name of the value entry to locate.

Return Value:

    Returns a pointer to the CM_VALUE_ENTRY structure if found, or NULL otherwise.

--*/
PCM_VALUE_ENTRY
CmQueryValueKey(
    PCM_KEY_NODE Key,
    PCSTR Name
)
{
    if (!Key || !Name) return NULL;

    PCM_VALUE_ENTRY Current = Key->Values;
    while (Current) {
        PCSTR a = Current->Name;
        PCSTR b = Name;
        int match = 1;
        while (*a && *b) {
            if (*a != *b) { match = 0; break; }
            a++; b++;
        }
        if (match && *a == 0 && *b == 0) {
            return Current;
        }
        Current = Current->Next;
    }
    return NULL;
}
