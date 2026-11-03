/*++

Copyright (c) 2026  Everywhere Computing, Inc. All Rights Reserved.

Module Name:

    pool.c

Abstract:

    Non-paged and paged pool allocator backed by RTL Heap.

Author:

    Noah Juopperi <nipfswd@gmail.com>
    Vlad Lymar <ggcc98765432110@gmail.com>

Environment:

    Kernel-mode only

--*/

#include "../inc/ntdef.h"
#include "../inc/mm.h"
#include "../inc/rtl.h"
#include "mi.h"

//
// Track pool statistics locally
//
static ULONG LocalPoolTotalBytes = 0;
static ULONG LocalPoolFreeBytes  = 0;

/*++

Routine Description:

    Initializes the non-paged pool allocator by initializing the underlying 
    RTL heap over the provided memory region.

Arguments:

    PoolBase - First byte of the region to use.
    PoolSize - Length in bytes.

Return Value:

    None.

--*/
VOID
MiInitializeNonPagedPool(
    ULONG_PTR PoolBase,
    ULONG     PoolSize
    )
{
    if (PoolSize == 0 || PoolBase == 0) {
        KeBugCheckEx(MEMORY_MANAGEMENT, 0x31, PoolBase, PoolSize, 0);
    }

    //
    // Initialize the stable RTL heap allocator over the given memory region
    //
    RtlInitializeHeap((PVOID)PoolBase, PoolSize);

    LocalPoolTotalBytes = PoolSize;
    LocalPoolFreeBytes  = PoolSize; // Initial estimate (heap tracks internal usage dynamically)
    
    MmPoolTotalBytes    = PoolSize;
    MmPoolFreeBytes     = PoolSize;
}

/*++

Routine Description:

    Allocates NumberOfBytes bytes from the pool. 
    Delegates directly to the RTL heap allocator.

Arguments:

    PoolType      - Pool to allocate from (NonPagedPool / PagedPool).
    NumberOfBytes - Number of usable bytes requested.
    Tag           - Four-byte caller tag (advisory).

Return Value:

    Pointer to the usable payload area, or NULL if the pool is exhausted.

--*/
PVOID
MmAllocatePool(
    MM_POOL_TYPE PoolType,
    ULONG        NumberOfBytes,
    ULONG        Tag
    )
{
    PVOID AllocatedMem;

    (VOID)PoolType;
    (VOID)Tag;

    if (NumberOfBytes == 0) {
        return NULL;
    }

    //
    // Delegate to the heap allocator
    //
    AllocatedMem = RtlAllocateHeap(NumberOfBytes);
    return AllocatedMem;
}

/*++

Routine Description:

    Returns a block previously obtained from MmAllocatePool back to the heap.

Arguments:

    P   - Pointer returned by a prior MmAllocatePool call.
    Tag - Expected tag (advisory).

Return Value:

    None.

--*/
VOID
MmFreePool(
    PVOID P,
    ULONG Tag
    )
{
    (VOID)Tag;

    if (P == NULL) {
        return;
    }

    //
    // Delegate to the heap deallocator with automatic coalescing
    //
    RtlFreeHeap(P);
}

/*++

Routine Description:

    Returns a snapshot of pool statistics.

Arguments:

    TotalBytes - Receives the total bytes under pool management.
    FreeBytes  - Receives the currently free bytes.

Return Value:

    None.

--*/
VOID
MmQueryPoolStats(
    ULONG *TotalBytes,
    ULONG *FreeBytes
    )
{
    if (TotalBytes != NULL) {
        *TotalBytes = LocalPoolTotalBytes;
    }

    if (FreeBytes != NULL) {
        *FreeBytes = LocalPoolFreeBytes;
    }
}
