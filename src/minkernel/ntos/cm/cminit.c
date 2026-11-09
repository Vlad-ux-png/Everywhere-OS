/*++

Copyright (c) 2026  O&V Soft. All Rights Reserved.

Module Name:

    cminit.c

Abstract:

    This module implements the initialization routines for the Configuration Manager (cm) subsystem.

Author(s):

    Vlad Lymar <ggcc98765432110@gmail.com>

Environment:

    Kernel-mode only.

--*/

#include "cm.h"

//
// Global registry root node 
//
PCM_KEY_NODE CmRootKey = NULL;


/*++

Routine Description:

    CmInitSystem initializes the Configuration Manager subsystem during kernel startup,
    building the initial in-memory registry hive structure (\REGISTRY\MACHINE and \REGISTRY\USER).

Arguments:

    None.

Return Value:

    None.

--*/
VOID
CmInitSystem(
    VOID
)
{
    CmRootKey = (PCM_KEY_NODE)RtlAllocateHeap(sizeof(CM_KEY_NODE));
    if (CmRootKey) {
        CmRootKey->Name = "REGISTRY";
        CmRootKey->Parent = NULL;
        CmRootKey->Child = NULL;
        CmRootKey->Sibling = NULL;
        CmRootKey->Values = NULL;

        //
        // Create standard root hives
        //
        PCM_KEY_NODE Machine = CmCreateKey(CmRootKey, "MACHINE");
        PCM_KEY_NODE User = CmCreateKey(CmRootKey, "USER");
    }
}
