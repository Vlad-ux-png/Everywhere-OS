/*++

Copyright (c) 2026  O&V Soft. All Rights Reserved.

Module Name:

    heap.c

Abstract:

    RTL heap allocator with free support using a linked list block layout
    compatible with MM test suite requirements.

Author(s):

    Vlad Lymar <ggcc98765432110@gmail.com>

Environment:

    Kernel-mode and User-mode.

--*/

#include "../inc/ntdef.h"
#include "../inc/rtl.h"
#include "mi.h" 

//
// Heap block header structure aligned with MM expectations
//
typedef struct _HEAP_BLOCK {
    ULONG               BlockSize;   
    ULONG               PreviousSize; 
    ULONG               PoolTag;    
    BOOLEAN             IsFree;     
    struct _HEAP_BLOCK* Next;        
} HEAP_BLOCK, *PHEAP_BLOCK;

#define HEAP_HEADER_SIZE  ((ULONG)sizeof(HEAP_BLOCK))

static PUCHAR        RtlpHeapMemoryStart = NULL;
static ULONG         RtlpHeapTotalSize   = 0;
static PHEAP_BLOCK   RtlpHeapHead        = NULL;

/*++

Routine Description:

    Initializes the RTL heap over a provided memory region.

Arguments:

    HeapMemory - Pointer to the raw memory buffer.
    HeapSize   - Size of the buffer in bytes.

--*/
VOID
RtlInitializeHeap(
    PVOID HeapMemory,
    ULONG HeapSize
)
{
    RtlpHeapMemoryStart = (PUCHAR)HeapMemory;
    RtlpHeapTotalSize   = HeapSize;

    //
    // Create a single large free block covering the entire heap
    //
    RtlpHeapHead = (PHEAP_BLOCK)RtlpHeapMemoryStart;
    RtlpHeapHead->BlockSize    = HeapSize / POOL_BLOCK_SIZE; // Або у байтах, залежно від вашого mi.h
    RtlpHeapHead->PreviousSize = 0;
    RtlpHeapHead->PoolTag      = 0;
    RtlpHeapHead->IsFree       = TRUE;
    RtlpHeapHead->Next         = NULL;
}

/*++

Routine Description:

    Allocates a block of memory from the heap.

Arguments:

    NumberOfBytes - Size of usable memory requested.

Return Value:

    Pointer to the payload area, or NULL if out of memory.

--*/
PVOID
RtlAllocateHeap(
    ULONG NumberOfBytes
)
{
    PHEAP_BLOCK Current;
    ULONG       RequiredSize;

    if (NumberOfBytes == 0 || RtlpHeapHead == NULL) {
        return NULL;
    }

    // Align required size to granularity (e.g., 8 bytes)
    RequiredSize = (NumberOfBytes + 7) & ~7;
    RequiredSize += HEAP_HEADER_SIZE;

    Current = RtlpHeapHead;

    //
    // Traverse the memory block list (First-Fit strategy)
    //
    while (Current != NULL) {
        if (Current->IsFree && (Current->BlockSize * POOL_BLOCK_SIZE) >= RequiredSize) {
            
            ULONG CurrentTotalSize = Current->BlockSize * POOL_BLOCK_SIZE;

            //
            // Check if we can split the block
            //
            if (CurrentTotalSize >= RequiredSize + HEAP_HEADER_SIZE + POOL_BLOCK_SIZE) {
                PHEAP_BLOCK NextBlock = (PHEAP_BLOCK)((PUCHAR)Current + RequiredSize);
                
                NextBlock->BlockSize    = (CurrentTotalSize - RequiredSize) / POOL_BLOCK_SIZE;
                NextBlock->PreviousSize = RequiredSize / POOL_BLOCK_SIZE;
                NextBlock->PoolTag      = 0;
                NextBlock->IsFree       = TRUE;
                NextBlock->Next         = Current->Next;

                Current->BlockSize = RequiredSize / POOL_BLOCK_SIZE;
                Current->Next      = NextBlock;

                if (NextBlock->Next != NULL) {
                    NextBlock->Next->PreviousSize = NextBlock->BlockSize;
                }
            }

            Current->IsFree = FALSE;
            
            // Return pointer past the header (payload)
            return (PVOID)((PUCHAR)Current + HEAP_HEADER_SIZE);
        }

        Current = Current->Next;
    }

    return NULL; // Out of memory
}

/*++

Routine Description:

    Frees a previously allocated memory block and coalesces adjacent free blocks.

Arguments:

    MemPointer - Pointer returned by RtlAllocateHeap.

--*/
VOID
RtlFreeHeap(
    PVOID MemPointer
)
{
    PHEAP_BLOCK TargetBlock;
    PHEAP_BLOCK Current;

    if (MemPointer == NULL) {
        return;
    }

    // Locate the header preceding the payload
    TargetBlock = (PHEAP_BLOCK)((PUCHAR)MemPointer - HEAP_HEADER_SIZE);
    TargetBlock->IsFree = TRUE;

    //
    // Coalesce adjacent free blocks (Merge pass)
    //
    Current = RtlpHeapHead;
    while (Current != NULL && Current->Next != NULL) {
        if (Current->IsFree && Current->Next->IsFree) {
            Current->BlockSize += Current->Next->BlockSize;
            Current->Next      = Current->Next->Next;
            
            if (Current->Next != NULL) {
                Current->Next->PreviousSize = Current->BlockSize;
            }
            continue;
        }
        Current = Current->Next;
    }
}
