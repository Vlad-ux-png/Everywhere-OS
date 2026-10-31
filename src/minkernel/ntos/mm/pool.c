/*++

Copyright (c) 2026  Everywhere Computing, Inc. All Rights Reserved.

Module Name:

    pool.c

Abstract:

    Simplified non-paged pool allocator (KISS principle) with block splitting.

Author:

    Noah Juopperi <nipfswd@gmail.com>
    Vlad Lymar <ggcc98765432110@gmail.com>

Environment:

    Kernel-mode only

--*/

#include "../inc/mm.h"
#include "mi.h"

//
// Pool block type identifiers
//
#define POOL_TYPE_FREE      0
#define POOL_TYPE_ALLOCATED 1

//
// Helper to align block sizes to POOL_BLOCK_SIZE
//
static ULONG
MiRoundBlockSize(
    ULONG NumberOfBytes
    )
{
    ULONG TotalSize;

    // Add header size and round up to POOL_BLOCK_SIZE boundary
    TotalSize = NumberOfBytes + MM_POOL_HEADER_SIZE;
    if (TotalSize % POOL_BLOCK_SIZE != 0) {
        TotalSize = ((TotalSize + POOL_BLOCK_SIZE - 1) / POOL_BLOCK_SIZE) * POOL_BLOCK_SIZE;
    }

    return TotalSize;
}

//
// Get appropriate free list bucket index
//
static ULONG
MiPoolIndexFromSize(
    ULONG BlockBytes
    )
{
    ULONG Index = BlockBytes / POOL_BLOCK_SIZE;
    if (Index > POOL_SMALL_LISTS) {
        Index = POOL_SMALL_LISTS;
    }
    return Index;
}

VOID
MiInitializeNonPagedPool(
    ULONG_PTR PoolBase,
    ULONG     PoolSize
    )
{
    PPOOL_HEADER InitialBlock;
    ULONG        BlockSize;
    ULONG        Index;
    ULONG        i;

    //
    // Zero all free-list heads (ensuring POOL_SMALL_LISTS + 1 items)
    //
    for (i = 0; i <= POOL_SMALL_LISTS; i++) {
        InitializeListHead(&MiNonPagedPoolFreeListHead[i]);
    }

    MiNonPagedPoolStart = PoolBase;
    MiNonPagedPoolEnd   = PoolBase + PoolSize;
    MmPoolTotalBytes    = PoolSize;
    MmPoolFreeBytes     = 0;

    //
    // Align available size down to POOL_BLOCK_SIZE
    //
    BlockSize = PoolSize & ~(POOL_BLOCK_SIZE - 1);

    if (BlockSize < MM_POOL_MIN_BLOCK) {
        KeBugCheckEx(MEMORY_MANAGEMENT, 0x31, PoolBase, PoolSize, 0);
    }

    //
    // Create the initial single free block spanning the whole pool
    //
    InitialBlock = (PPOOL_HEADER)PoolBase;
    InitialBlock->PreviousSize = 0;
    InitialBlock->BlockSize    = BlockSize;
    InitialBlock->PoolType     = POOL_TYPE_FREE;
    InitialBlock->PoolTag      = 0;

    //
    // Put into the overflow free list
    //
    Index = MiPoolIndexFromSize(BlockSize);
    InsertHeadList(&MiNonPagedPoolFreeListHead[Index], (PLIST_ENTRY)&InitialBlock->FreeNext);

    MmPoolFreeBytes = BlockSize;
}

PVOID
MmAllocatePool(
    MM_POOL_TYPE PoolType,
    ULONG        NumberOfBytes,
    ULONG        Tag
    )
{
    ULONG        TotalSize;
    ULONG        ScanIndex;
    ULONG        FoundIndex = 0;
    PLIST_ENTRY  ListEntry;
    PPOOL_HEADER Block;
    PUCHAR       Payload;
    ULONG        RemainingSize;
    PPOOL_HEADER RemainingBlock;
    ULONG        RemIndex;

    (VOID)PoolType;

    if (NumberOfBytes == 0) {
        return NULL;
    }

    TotalSize = MiRoundBlockSize(NumberOfBytes);

    LOCK_POOL();

    //
    // Search for a suitable free block starting from the matching bucket
    //
    for (ScanIndex = MiPoolIndexFromSize(TotalSize); ScanIndex <= POOL_SMALL_LISTS; ScanIndex++) {
        if (!IsListEmpty(&MiNonPagedPoolFreeListHead[ScanIndex])) {
            FoundIndex = ScanIndex;
            break;
        }
    }

    if (FoundIndex == 0 || IsListEmpty(&MiNonPagedPoolFreeListHead[FoundIndex])) {
        UNLOCK_POOL();
        return NULL;
    }

    //
    // Grab the first available block from the list
    //
    ListEntry = MiNonPagedPoolFreeListHead[FoundIndex].Flink;
    RemoveEntryList(ListEntry);

    //
    // Safely retrieve the pool header using CONTAINING_RECORD
    //
    Block = CONTAINING_RECORD(ListEntry, POOL_HEADER, FreeNext);

    //
    // Check if the block can be split
    //
    RemainingSize = Block->BlockSize - TotalSize;
    if (RemainingSize >= MM_POOL_MIN_BLOCK) {
        //
        // Create a new free block from the remaining space
        //
        RemainingBlock = (PPOOL_HEADER)((PUCHAR)Block + TotalSize);
        RemainingBlock->PreviousSize = TotalSize;
        RemainingBlock->BlockSize    = RemainingSize;
        RemainingBlock->PoolType     = POOL_TYPE_FREE;
        RemainingBlock->PoolTag      = 0;

        // Truncate current block to the requested size
        Block->BlockSize = TotalSize;

        // Insert the remainder back into the appropriate free list
        RemIndex = MiPoolIndexFromSize(RemainingSize);
        InsertHeadList(&MiNonPagedPoolFreeListHead[RemIndex], (PLIST_ENTRY)&RemainingBlock->FreeNext);

        MmPoolFreeBytes -= TotalSize;
    } else {
        // Take the whole block if remainder is too small
        MmPoolFreeBytes -= Block->BlockSize;
    }

    //
    // Mark as allocated
    //
    Block->PoolType = POOL_TYPE_ALLOCATED;
    Block->PoolTag  = Tag;

    UNLOCK_POOL();

    //
    // Return pointer to the payload area right after the header
    //
    Payload = (PUCHAR)Block + MM_POOL_HEADER_SIZE;
    return (PVOID)Payload;
}

VOID
MmFreePool(
    PVOID P,
    ULONG Tag
    )
{
    PPOOL_HEADER Block;
    ULONG        FreeIndex;

    (VOID)Tag;

    if (P == NULL) {
        return;
    }

    //
    // Locate the header by subtracting header size from the payload pointer
    //
    Block = (PPOOL_HEADER)((PUCHAR)P - MM_POOL_HEADER_SIZE);

    LOCK_POOL();

    if (Block->PoolType == POOL_TYPE_FREE) {
        UNLOCK_POOL();
        KeBugCheckEx(BAD_POOL_HEADER, 0, (ULONG_PTR)P, 0, 0);
    }

    //
    // Mark block as free and insert back into the appropriate free list
    //
    Block->PoolType = POOL_TYPE_FREE;
    Block->PoolTag  = 0;

    FreeIndex = MiPoolIndexFromSize(Block->BlockSize);
    InsertHeadList(&MiNonPagedPoolFreeListHead[FreeIndex], (PLIST_ENTRY)&Block->FreeNext);

    MmPoolFreeBytes += Block->BlockSize;

    UNLOCK_POOL();
}

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
