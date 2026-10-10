/*++

Copyright (c) 2026  Everywhere Computing, Inc. All Rights Reserved.

Module Name:

    vadtree.c

Abstract:

    High-level virtual address descriptor (VAD) operations.

    VADs describe regions of a process virtual address space.  They are
    stored in a per-process AVL tree keyed by starting VPN.

    MiInsertVad    - Validates and inserts a new VAD.
    MiRemoveVad    - Removes a VAD from the tree and frees it.
    MiLocateAddress - Returns the VAD that maps a given virtual address.

Author:

    Noah Juopperi <nipfswd@gmail.com>
    Vlad Lymar <ggcc98765432110@gmail.com>

Environment:

    Kernel-mode only.  Caller holds the process working-set lock.

--*/

#include "../inc/mm.h"
#include "mi.h"

/* -----------------------------------------------------------------------
 * MiInsertVad
 * ----------------------------------------------------------------------- */

/*++

Routine Description:

    Inserts Vad into the VAD tree rooted at *VadRoot after verifying that
    no existing VAD overlaps the region [Vad->StartingVpn, Vad->EndingVpn].

Arguments:

    Vad     - Fully initialised MMVAD to insert.

    VadRoot - Pointer to the root pointer of the VAD tree.

Return Value:

    STATUS_SUCCESS        - Vad was inserted.
    STATUS_CONFLICTING_ADDRESSES - An existing VAD overlaps the range.

--*/
NTSTATUS
MiInsertVad(
    PMMVAD  Vad,
    PMMVAD *VadRoot
    )
{
    PMMVAD Conflict;

    Conflict = MiCheckForConflictingNode(Vad->StartingVpn,
                                         Vad->EndingVpn,
                                         *VadRoot);
    if (Conflict != NULL) {
        return STATUS_CONFLICTING_ADDRESSES;
    }

    MiInsertNode(Vad, VadRoot);
    return STATUS_SUCCESS;
}

/* -----------------------------------------------------------------------
 * MiRemoveVad
 * ----------------------------------------------------------------------- */

/*++

Routine Description:

    Removes Vad from the VAD tree rooted at *VadRoot and frees the MMVAD
    allocation from the non-paged pool.

Arguments:

    Vad     - The VAD to remove.

    VadRoot - Pointer to the root pointer of the VAD tree.

Return Value:

    None.

--*/
VOID
MiRemoveVad(
    PMMVAD  Vad,
    PMMVAD *VadRoot
    )
{
    MiRemoveNode(Vad, VadRoot);
    MmFreePool(Vad, 0x6461564D);  // 'daVM'
}

/* -----------------------------------------------------------------------
 * MiLocateAddress
 * ----------------------------------------------------------------------- */

/*++

Routine Description:

    Returns the VAD that describes the virtual address VirtualAddress,
    or NULL if the address is not mapped in the tree.

Arguments:

    VirtualAddress - Virtual address to look up.

    Root           - Root of the VAD tree to search.

Return Value:

    Pointer to the enclosing MMVAD, or NULL.

--*/
PMMVAD
MiLocateAddress(
    PVOID  VirtualAddress,
    PMMVAD Root
    )
{
    return MiLocateAddressInTree(MI_VA_TO_VPN((ULONG_PTR)VirtualAddress),
                                 Root);
}
