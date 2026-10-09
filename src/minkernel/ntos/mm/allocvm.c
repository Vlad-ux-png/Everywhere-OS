/*++

Copyright (c) 2026  Everywhere Computing, Inc. All Rights Reserved.

Module Name:

    allocvm.c

Abstract:

    Virtual memory reservation and commitment.

    MmAllocateVirtualMemory handles both MEM_RESERVE (allocates a range
    of virtual address space by inserting a VAD) and MEM_COMMIT (maps
    demand-zero PTEs into a previously reserved range).

Author:

    Noah Juopperi <nipfswd@gmail.com>

Environment:

    Kernel-mode only.

--*/

#include "../inc/mm.h"
#include "mi.h"

/* -----------------------------------------------------------------------
 * MmAllocateVirtualMemory
 * ----------------------------------------------------------------------- */

/*++

Routine Description:

    Reserves and/or commits a region of virtual address space.

    If *BaseAddress is NULL and AllocationType includes MEM_RESERVE,
    the routine finds a free virtual address range of at least *RegionSize
    bytes, creates a VAD for it, and returns its base in *BaseAddress.

    If AllocationType includes MEM_COMMIT (alone or with MEM_RESERVE),
    demand-zero PTEs are written for each page in the committed range.

Arguments:

    BaseAddress    - On entry, desired base address (or NULL for any).
                     On exit, actual allocated base.

    ZeroBits       - Number of high-order zero bits required in the address
                     (0 = no constraint).  Currently unused.

    RegionSize     - On entry, requested size in bytes.
                     On exit, actual committed size (rounded to page boundary).

    AllocationType - MEM_RESERVE, MEM_COMMIT, or both.

    Protect        - Page protection for committed pages (PAGE_* flags).

Return Value:

    STATUS_SUCCESS                 - Region allocated/committed.
    STATUS_INVALID_PARAMETER       - Bad flags or size.
    STATUS_NO_MEMORY               - No virtual address space available.
    STATUS_CONFLICTING_ADDRESSES   - BaseAddress conflicts with existing VAD.

--*/
NTSTATUS
MmAllocateVirtualMemory(
    PVOID    *BaseAddress,
    ULONG_PTR ZeroBits,
    SIZE_T   *RegionSize,
    ULONG     AllocationType,
    ULONG     Protect
    )
{
    ULONG_PTR  Base;
    SIZE_T     Size;
    PMMVAD     Vad;
    NTSTATUS   Status;
    MMPTE      DemandZeroPte;
    PMMPTE     StartPte;
    PMMPTE     EndPte;
    PMMPTE     Pte;

    (VOID)ZeroBits;

    if (RegionSize == NULL || *RegionSize == 0) {
        return STATUS_INVALID_PARAMETER;
    }

    if (!(AllocationType & (MEM_RESERVE | MEM_COMMIT))) {
        return STATUS_INVALID_PARAMETER;
    }

    Size = ROUND_TO_PAGES(*RegionSize);

    //
    // Determine the base address.
    //
    if (*BaseAddress == NULL || (AllocationType & MEM_RESERVE)) {

        if (*BaseAddress != NULL) {
            Base = (ULONG_PTR)*BaseAddress & PAGE_MASK;
        } else {
            //
            // Find a free range from the system VAD tree root.
            //
            Status = MiFindEmptyAddressRangeInTree(
                         Size,
                         (ULONG_PTR)PAGE_SIZE,
                         MmSystemCacheWs.VadRoot,
                         &Base);

            if (!NT_SUCCESS(Status)) {
                return Status;
            }
        }

        //
        // Create and insert the VAD.
        //
        Vad = (PMMVAD)MmAllocatePool(NonPagedPool, sizeof(MMVAD), 0x6461564D);  // 'daVM'
        if (Vad == NULL) {
            return STATUS_NO_MEMORY;
        }

        Vad->StartingVpn    = MI_VA_TO_VPN(Base);
        Vad->EndingVpn      = MI_VA_TO_VPN(Base + Size - 1);
        Vad->LeftChild      = NULL;
        Vad->RightChild     = NULL;
        Vad->Parent         = NULL;
        Vad->Balance        = 0;
        Vad->u.LongFlags    = 0;
        Vad->u.VadFlags.PrivateMemory = 1;
        Vad->u.VadFlags.Protection    = (ULONG)Protect & 0x1F;

        Status = MiInsertVad(Vad, &MmSystemCacheWs.VadRoot);
        if (!NT_SUCCESS(Status)) {
            MmFreePool(Vad, 0x6461564D);  // 'daVM'
            return Status;
        }

        *BaseAddress = (PVOID)Base;

    } else {
        Base = (ULONG_PTR)*BaseAddress & PAGE_MASK;
    }

    *RegionSize = Size;

    //
    // If MEM_COMMIT is requested, write demand-zero PTEs.
    //
    if (AllocationType & MEM_COMMIT) {

        DemandZeroPte.Long              = 0;
        DemandZeroPte.Soft.Protection   = (ULONG)Protect & 0x1F;

        StartPte = MI_GET_PTE_ADDRESS(Base);
        EndPte   = MI_GET_PTE_ADDRESS(Base + Size - PAGE_SIZE);

        for (Pte = StartPte; Pte <= EndPte; Pte++) {
            if (MI_PTE_IS_ZERO(*Pte)) {
                Pte->Long = DemandZeroPte.Long;
            }
        }
    }

    return STATUS_SUCCESS;
}
