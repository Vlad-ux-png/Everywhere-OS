/*++

Copyright (c) 2026  Everywhere Computing, Inc. All Rights Reserved.

Module Name:

    pool.c

Abstract:

    Non-paged and paged pool allocator.

    The pool is divided into fixed-size buckets for small allocations
    (1 to POOL_SMALL_LISTS blocks of POOL_BLOCK_SIZE bytes) and a single
    overflow list for larger requests.  Each pool block is prefixed by an
    8-byte POOL_HEADER.  Free blocks store a LIST_ENTRY immediately after
    the header in their payload area; allocated blocks have arbitrary
    caller data there.

    Free list indices are 1-based: index k holds blocks of exactly
    k * POOL_BLOCK_SIZE bytes of payload (plus the 8-byte header).
    Index POOL_SMALL_LISTS is the overflow list for blocks larger than
    POOL_MAX_BLOCK_SIZE.

    MiInitializeNonPagedPool seeds the allocator with one large free block
    spanning the entire non-paged pool region.  MmAllocatePool and
    MmFreePool are the public entry points.

    Locking: every public path disables interrupts for the duration of
    the free-list manipulation (LOCK_POOL / UNLOCK_POOL).

Author:

    Noah Juopperi <nipfswd@gmail.com>

Environment:

    Kernel-mode only

--*/

#include "../inc/mm.h"
#include "mi.h"

/* -----------------------------------------------------------------------
 * Internal constants
 * ----------------------------------------------------------------------- */

//
// PoolType values encoded in POOL_HEADER::PoolType.  Zero means the block
// is free; non-zero identifies the pool it was allocated from.
//
#define POOL_TYPE_NONPAGED  (NonPagedPool + 1)
#define POOL_TYPE_PAGED     (PagedPool    + 1)
#define POOL_TYPE_FREE      0

/* -----------------------------------------------------------------------
 * Internal helpers
 * ----------------------------------------------------------------------- */

//
// MiPoolIndexFromSize
//
// Returns the free-list index (1 .. POOL_SMALL_LISTS) for a total block
// size (header + payload) given in bytes.  The index is clamped to
// POOL_SMALL_LISTS for oversized blocks.
//
static ULONG
MiPoolIndexFromSize(
    ULONG BlockBytes
    )
{
    ULONG index;

    //
    // BlockBytes already includes the 8-byte header.  We want the payload
    // block count.
    //
    index = (BlockBytes + POOL_BLOCK_SIZE - 1) / POOL_BLOCK_SIZE;

    if (index > POOL_SMALL_LISTS) {
        index = POOL_SMALL_LISTS;
    }

    return index;
}

//
// MiRoundBlockSize
//
// Rounds NumberOfBytes (payload only) up to the next POOL_BLOCK_SIZE
// multiple and adds the header, yielding the total on-disk block size.
//
static ULONG
MiRoundBlockSize(
    ULONG NumberOfBytes
    )
{
    ULONG payload;

    payload = (NumberOfBytes + POOL_BLOCK_SIZE - 1) & ~(POOL_BLOCK_SIZE - 1);
    return payload + MM_POOL_HEADER_SIZE;
}

/* -----------------------------------------------------------------------
 * Initialisation
 * ----------------------------------------------------------------------- */

/*++

Routine Description:

    Seeds the non-paged pool allocator.  Zeroes all free-list heads,
    writes a single free POOL_HEADER spanning the entire region, and
    links it into the appropriate overflow free list.

    After this call MmAllocatePool and MmFreePool are operational.

Arguments:

    PoolBase - First byte of the region to use.  Must be 8-byte aligned.

    PoolSize - Length in bytes.  Must be at least MM_POOL_MIN_BLOCK.

Return Value:

    None.

Environment:

    Called once from MmInit, before interrupts are enabled.

--*/
VOID
MiInitializeNonPagedPool(
    ULONG_PTR PoolBase,
    ULONG     PoolSize
    )
{
    PPOOL_HEADER InitialBlock;
    PLIST_ENTRY  Entry;
    ULONG        BlockSize;
    ULONG        Index;
    ULONG        i;

    //
    // Zero all free-list heads.
    //
    for (i = 0; i <= POOL_SMALL_LISTS; i++) {
        InitializeListHead(&MiNonPagedPoolFreeListHead[i]);
    }

    MiNonPagedPoolStart = PoolBase;
    MiNonPagedPoolEnd   = PoolBase + PoolSize;
    MmPoolTotalBytes    = PoolSize;
    MmPoolFreeBytes     = 0;

    //
    // Align the available size down to a POOL_BLOCK_SIZE boundary so the
    // initial block's BlockSize is a round multiple.
    //
    BlockSize = PoolSize & ~(POOL_BLOCK_SIZE - 1);

    if (BlockSize < MM_POOL_MIN_BLOCK) {
        KeBugCheckEx(MEMORY_MANAGEMENT, 0x31, PoolBase, PoolSize, 0);
    }

    //
    // Write the single seed block.  PoolType=0 means free.
    //
    InitialBlock = (PPOOL_HEADER)PoolBase;
    InitialBlock->Ulong1       = 0;
    InitialBlock->BlockSize    = (USHORT)(BlockSize / POOL_BLOCK_SIZE);
    InitialBlock->PreviousSize = 0;
    InitialBlock->PoolType     = POOL_TYPE_FREE;
    InitialBlock->PoolTag      = 0;

    //
    // Put the initial block into the overflow free list (index
    // POOL_SMALL_LISTS) since it covers the entire pool region.
    //
    Entry = (PLIST_ENTRY)((PUCHAR)InitialBlock + MM_POOL_HEADER_SIZE);
    Index = POOL_SMALL_LISTS;
    InsertHeadList(&MiNonPagedPoolFreeListHead[Index], Entry);

    MmPoolFreeBytes = BlockSize;
}

/* -----------------------------------------------------------------------
 * Public allocation entry point
 * ----------------------------------------------------------------------- */

/*++

Routine Description:

    Allocates NumberOfBytes bytes from the pool identified by PoolType.

    The total block size (header + payload rounded to POOL_BLOCK_SIZE) is
    computed, the corresponding free-list bucket is probed first; if it
    is empty, all higher-indexed buckets are scanned in order.  If a block
    is found that is larger than required and can be split (leaving at
    least one POOL_BLOCK_SIZE unit of payload), the block is divided.

    The selected block is removed from its free list, its PoolType field
    is set to the caller's pool type, and a pointer past the header is
    returned.

Arguments:

    PoolType      - Pool to allocate from.  Currently only NonPagedPool
                    is backed by physical memory; PagedPool falls through
                    to NonPagedPool.

    NumberOfBytes - Number of usable bytes requested.

    Tag           - Four-byte caller tag written into PoolTag.

Return Value:

    Pointer to the usable payload area, or NULL if the pool is exhausted.

Environment:

    Interrupts may be enabled; this routine acquires LOCK_POOL internally.

--*/
PVOID
MmAllocatePool(
    MM_POOL_TYPE PoolType,
    ULONG        NumberOfBytes,
    ULONG        Tag
    )
{
    ULONG        TotalSize;
    ULONG        WantIndex;
    ULONG        ScanIndex;
    ULONG        FoundIndex;
    PLIST_ENTRY  ListEntry;
    PPOOL_HEADER Block;
    PPOOL_HEADER SplitBlock;
    PPOOL_HEADER NextPhys;
    ULONG        SplitBlockSize;
    ULONG        SplitIndex;
    PUCHAR       Payload;

    (VOID)PoolType;

    if (NumberOfBytes == 0) {
        return NULL;
    }

    TotalSize  = MiRoundBlockSize(NumberOfBytes);
    WantIndex  = MiPoolIndexFromSize(TotalSize);
    FoundIndex = 0;

    LOCK_POOL();

    //
    // Scan from WantIndex upward to find the smallest available bucket.
    //
    for (ScanIndex = WantIndex; ScanIndex <= POOL_SMALL_LISTS; ScanIndex++) {
        if (!IsListEmpty(&MiNonPagedPoolFreeListHead[ScanIndex])) {
            FoundIndex = ScanIndex;
            break;
        }
    }

    if (FoundIndex == 0) {
        UNLOCK_POOL();
        return NULL;
    }

    ListEntry = MiNonPagedPoolFreeListHead[FoundIndex].Flink;
    RemoveEntryList(ListEntry);
    Block = (PPOOL_HEADER)((PUCHAR)ListEntry - MM_POOL_HEADER_SIZE);

    //
    // If the found block is larger than needed and can be split, carve
    // a new free block off the tail.
    //
    if ((ULONG)(Block->BlockSize * POOL_BLOCK_SIZE) >= TotalSize + MM_POOL_MIN_BLOCK) {

        SplitBlockSize = (ULONG)(Block->BlockSize * POOL_BLOCK_SIZE) - TotalSize;
        SplitBlock = (PPOOL_HEADER)((PUCHAR)Block + TotalSize);

        SplitBlock->Ulong1       = 0;
        SplitBlock->BlockSize    = (USHORT)(SplitBlockSize / POOL_BLOCK_SIZE);
        SplitBlock->PreviousSize = (USHORT)(TotalSize / POOL_BLOCK_SIZE);
        SplitBlock->PoolType     = POOL_TYPE_FREE;
        SplitBlock->PoolTag      = 0;

        //
        // Update the PreviousSize of the block that follows the split block.
        //
        NextPhys = (PPOOL_HEADER)((PUCHAR)SplitBlock + SplitBlockSize);
        if ((ULONG_PTR)NextPhys < MiNonPagedPoolEnd) {
            NextPhys->PreviousSize = SplitBlock->BlockSize;
        }

        //
        // Place the split block on the appropriate free list.
        //
        SplitIndex  = MiPoolIndexFromSize(SplitBlockSize);
        ListEntry   = (PLIST_ENTRY)((PUCHAR)SplitBlock + MM_POOL_HEADER_SIZE);
        InsertHeadList(&MiNonPagedPoolFreeListHead[SplitIndex], ListEntry);
        MmPoolFreeBytes += SplitBlockSize;

        Block->BlockSize = (USHORT)(TotalSize / POOL_BLOCK_SIZE);
    }

    Block->PoolType = (USHORT)POOL_TYPE_NONPAGED;
    Block->PoolTag  = Tag;
    MmPoolFreeBytes -= (ULONG)(Block->BlockSize * POOL_BLOCK_SIZE);

    UNLOCK_POOL();

    Payload = (PUCHAR)Block + MM_POOL_HEADER_SIZE;
    return (PVOID)Payload;
}

/* -----------------------------------------------------------------------
 * Public free entry point
 * ----------------------------------------------------------------------- */

/*++

Routine Description:

    Returns a block previously obtained from MmAllocatePool.

    The header is located by subtracting MM_POOL_HEADER_SIZE from P.
    If PoolType is not a recognised allocated type, KeBugCheckEx is called
    with BAD_POOL_HEADER.

    After marking the block free the routine attempts to coalesce:
      - Forward: if the physically next block is also free, merge.
      - Backward: if the physically preceding block is free, merge.
    The merged block is inserted into the appropriate free-list bucket.

Arguments:

    P   - Pointer returned by a prior MmAllocatePool call.  NULL is a
          safe no-op.

    Tag - Expected tag.  Currently advisory only; stored for diagnostics.

Return Value:

    None.

Environment:

    Interrupts may be enabled; this routine acquires LOCK_POOL internally.

--*/
VOID
MmFreePool(
    PVOID P,
    ULONG Tag
    )
{
    PPOOL_HEADER Block;
    PPOOL_HEADER PrevBlock;
    PPOOL_HEADER NextBlock;
    PLIST_ENTRY  ListEntry;
    ULONG        TotalSize;
    ULONG        FreeIndex;

    (VOID)Tag;

    if (P == NULL) {
        return;
    }

    Block = (PPOOL_HEADER)((PUCHAR)P - MM_POOL_HEADER_SIZE);

    LOCK_POOL();

    if (Block->PoolType == POOL_TYPE_FREE) {
        UNLOCK_POOL();
        KeBugCheckEx(BAD_POOL_HEADER, 0, (ULONG_PTR)P, 0, 0);
    }

    TotalSize = (ULONG)(Block->BlockSize * POOL_BLOCK_SIZE);
    Block->PoolType = POOL_TYPE_FREE;
    Block->PoolTag  = 0;
    MmPoolFreeBytes += TotalSize;

    //
    // Forward coalesce: merge with the next physical block if it is free.
    //
    NextBlock = (PPOOL_HEADER)((PUCHAR)Block + TotalSize);

    if ((ULONG_PTR)NextBlock < MiNonPagedPoolEnd &&
        NextBlock->PoolType == POOL_TYPE_FREE) {

        //
        // Remove next from its free list.
        //
        ListEntry = (PLIST_ENTRY)((PUCHAR)NextBlock + MM_POOL_HEADER_SIZE);
        RemoveEntryList(ListEntry);

        TotalSize += (ULONG)(NextBlock->BlockSize * POOL_BLOCK_SIZE);
        Block->BlockSize = (USHORT)(TotalSize / POOL_BLOCK_SIZE);

        //
        // Update PreviousSize of the block after the merged region.
        //
        NextBlock = (PPOOL_HEADER)((PUCHAR)Block + TotalSize);
        if ((ULONG_PTR)NextBlock < MiNonPagedPoolEnd) {
            NextBlock->PreviousSize = Block->BlockSize;
        }
    }

    //
    // Backward coalesce: merge into the previous block if it is free.
    //
    if (Block->PreviousSize != 0) {
        PrevBlock = (PPOOL_HEADER)((PUCHAR)Block -
                    (ULONG)(Block->PreviousSize * POOL_BLOCK_SIZE));

        if (PrevBlock->PoolType == POOL_TYPE_FREE) {

            ListEntry = (PLIST_ENTRY)((PUCHAR)PrevBlock + MM_POOL_HEADER_SIZE);
            RemoveEntryList(ListEntry);

            TotalSize += (ULONG)(PrevBlock->BlockSize * POOL_BLOCK_SIZE);
            PrevBlock->BlockSize = (USHORT)(TotalSize / POOL_BLOCK_SIZE);

            //
            // The successor block's PreviousSize must reflect the merged size.
            //
            NextBlock = (PPOOL_HEADER)((PUCHAR)PrevBlock + TotalSize);
            if ((ULONG_PTR)NextBlock < MiNonPagedPoolEnd) {
                NextBlock->PreviousSize = PrevBlock->BlockSize;
            }

            Block = PrevBlock;
        }
    }

    FreeIndex = MiPoolIndexFromSize(TotalSize);
    ListEntry = (PLIST_ENTRY)((PUCHAR)Block + MM_POOL_HEADER_SIZE);
    InsertHeadList(&MiNonPagedPoolFreeListHead[FreeIndex], ListEntry);

    UNLOCK_POOL();
}

/*++

Routine Description:

    Returns a snapshot of pool statistics.

Arguments:

    TotalBytes - Receives the total bytes under pool management.  May be
                 NULL.

    FreeBytes  - Receives the currently free bytes.  May be NULL.

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
        *TotalBytes = MmPoolTotalBytes;
    }

    if (FreeBytes != NULL) {
        *FreeBytes = MmPoolFreeBytes;
    }
}
