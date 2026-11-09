/*++

Copyright (c) 2026  O&V Soft. All Rights Reserved.

Module Name:

    cm.h

Abstract:

    This module contains the definitions and structures for the Configuration Manager (cm) subsystem.

Author(s):

    Vlad Lymar <ggcc98765432110@gmail.com>

Environment:

    Kernel-mode only.

--*/

#ifndef _CM_H_
#define _CM_H_

#include "ex.h"

//
// Registry Value Types
//
#define REG_NONE        0
#define REG_SZ          1
#define REG_DWORD       4

typedef struct _CM_KEY_NODE CM_KEY_NODE, *PCM_KEY_NODE;
typedef struct _CM_VALUE_ENTRY CM_VALUE_ENTRY, *PCM_VALUE_ENTRY;

//
// Value Entry Structure
//
struct _CM_VALUE_ENTRY {
    PCSTR Name;
    ULONG Type;
    ULONG DataSize;
    PVOID Data;
    PCM_VALUE_ENTRY Next;
};

//
// Key Node Structure (In-Memory Tree)
//
struct _CM_KEY_NODE {
    PCSTR Name;
    PCM_KEY_NODE Parent;
    PCM_KEY_NODE Child;
    PCM_KEY_NODE Sibling;
    PCM_VALUE_ENTRY Values;
};

extern PCM_KEY_NODE CmRootKey;

VOID
CmInitSystem(
    VOID
);

PCM_KEY_NODE
CmCreateKey(
    PCM_KEY_NODE Parent,
    PCSTR Name
);

VOID
CmSetValueKey(
    PCM_KEY_NODE Key,
    PCSTR Name,
    ULONG Type,
    PVOID Data,
    ULONG DataSize
);

PCM_VALUE_ENTRY
CmQueryValueKey(
    PCM_KEY_NODE Key,
    PCSTR Name
);

#endif /* _CM_H_ */
