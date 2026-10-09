/*++

Copyright (c) 2026  Everywhere Computing, Inc. All Rights Reserved.

Module Name:

    freevm.c

Abstract:

    Virtual memory decommitment and release.

    MmFreeVirtualMemory handles MEM_DECOMMIT (invalidates committed PTEs,
    frees the physical pages) and MEM_RELEASE (decommits and removes the
    VAD from the tree).

Author:

    Noah Juopperi <nipfswd@gmail.com>

Environment:

    Kernel-mode only.

--*/

#include "../inc/mm.h"
#include "mi.h"

/* -----------------------------------------------------------------------
 * MmFreeVirtualMemory
 * ----------------------------------------------------------------------- */

/*++

Routine Description:

    Decommits and/or releases a region of virtual address space.

    MEM_DECOMMIT  - Invalidates all committed PTEs in the range and
                    returns the physical pages to the free list.
    MEM_RELEASE   - Decommits all pages in the entire VAD and removes
                    the VAD from the address space.  *RegionSize must be 0.

Arguments:

    BaseAddress - On entry, an address within the region.  On exit,
                  the actual base of the region freed.

    RegionSize  - On entry, size of region to decommit (must be 0 for
                  MEM_RELEASE).  On exit, actual size freed.

    FreeType    - MEM_DECOMMIT or MEM_RELEASE.

Return Value:

    STATUS_SUCCESS           - Memory freed.
    STATUS_INVALID_PARAMETER - Bad combination of arguments.
    STATUS_INVALID_ADDRESS   - No VAD found for BaseAddress.

--*/
NTSTATUS
MmFreeVirtualMemory(
    PVOID  *BaseAddress,
    SIZE_T *RegionSize,
    ULONG   FreeType
    )
{
    ULONG_PTR  Base;
    ULONG_PTR  EndVa;
    SIZE_T     Size;
    PMMVAD     Vad;
    PMMPTE     Pte;
    PMMPTE     StartPte;
    PMMPTE     EndPte;
    PFN_NUMBER PageFrameIndex;
    PMMPFN     Pfn1;
    ULONG      OldIrql;

    if (BaseAddress == NULL || *BaseAddress == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    if (!(FreeType & (MEM_DECOMMIT | MEM_RELEASE))) {
        return STATUS_INVALID_PARAMETER;
    }

    if ((FreeType & MEM_RELEASE) && RegionSize != NULL && *RegionSize != 0) {
        return STATUS_INVALID_PARAMETER;
    }

    Base = (ULONG_PTR)*BaseAddress & PAGE_MASK;

    //
    // Locate the VAD that contains Base.
    //
    Vad = MiLocateAddress((PVOID)Base, MmSystemCacheWs.VadRoot);
    if (Vad == NULL) {
        return STATUS_INVALID_ADDRESS;
    }

    if (FreeType & MEM_RELEASE) {
        //
        // Decommit the entire VAD range.
        //
        Base   = (uint32_t)MI_VPN_TO_VA_ENDING(Vad->StartingVpn);
        EndVa  = (uint32_t)MI_VPN_TO_VA_ENDING(Vad->EndingVpn);
        Size   = EndVa - Base + 1;
        *BaseAddress = (PVOID)Base;
        if (RegionSize != NULL) {
            *RegionSize = Size;
        }
    } else {
        if (RegionSize == NULL || *RegionSize == 0) {
            return STATUS_INVALID_PARAMETER;
        }
        Size   = ROUND_TO_PAGES(*RegionSize);
        EndVa  = Base + Size - 1;
        *RegionSize = Size;
    }

    //
    // Walk the PTEs and free each committed page.
    //
    StartPte = MI_GET_PTE_ADDRESS(Base);
    EndPte   = MI_GET_PTE_ADDRESS(Base + Size - PAGE_SIZE);

    for (Pte = StartPte; Pte <= EndPte; Pte++) {

        if (!MI_PTE_IS_VALID(*Pte)) {
            //
            // Clear demand-zero or transition software PTEs.
            //
            Pte->Long = 0;
            continue;
        }

        PageFrameIndex = (PFN_NUMBER)Pte->Hard.PageFrameNumber;
        Pfn1           = MI_PFN_ELEMENT(PageFrameIndex);

        LOCK_PFN(OldIrql);
        MiDecrementShareCount(Pfn1, PageFrameIndex);
        UNLOCK_PFN(OldIrql);

        Pte->Long = 0;
    }

    //
    // For MEM_RELEASE, remove the VAD from the tree.
    //
    if (FreeType & MEM_RELEASE) {
        MiRemoveVad(Vad, &MmSystemCacheWs.VadRoot);
    }

    return STATUS_SUCCESS;
}
