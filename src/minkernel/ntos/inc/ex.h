/*++

Copyright (c) 2026  Everywhere Computing, Inc. All Rights Reserved.

Module Name:

    ex.h

Abstract:

    Executive Subsystem (EX) definitions, synchronization primitives,
    lookaside lists, and public prototypes.

Author:

    Noah Juopperi <nipfswd@gmail.com>
    Vlad Lymar <ggcc98765432110@gmail.com>

Environment:

    Kernel-mode only.

--*/

#ifndef _EX_H_
#define _EX_H_

#include "mm.h"
#include "ntstatus.h"
#include "ntdef.h"
#include "rtl.h"

/* -----------------------------------------------------------------------
 * Locally Unique Identifier (LUID)
 * ----------------------------------------------------------------------- */

typedef struct _LUID {
    ULONG LowPart;
    LONG  HighPart;
} LUID, *PLUID;

/* -----------------------------------------------------------------------
 * Single Linked List (S-List)
 * ----------------------------------------------------------------------- */

typedef struct _SLIST_ENTRY {
    struct _SLIST_ENTRY *Next;
} SLIST_ENTRY, *PSLIST_ENTRY;

typedef struct _SLIST_HEADER {
    PSLIST_ENTRY Next;
    USHORT       Depth;
    USHORT       Sequence;
} SLIST_HEADER, *PSLIST_HEADER;

VOID
InitializeSListHead(
    PSLIST_HEADER SListHead
    );

PSLIST_ENTRY
InterlockedPushEntrySList(
    PSLIST_HEADER SListHead,
    PSLIST_ENTRY  ListEntry
    );

PSLIST_ENTRY
InterlockedPopEntrySList(
    PSLIST_HEADER SListHead
    );

/* -----------------------------------------------------------------------
 * Interlocked Operations
 * ----------------------------------------------------------------------- */

static inline LONG
InterlockedIncrement(
    LONG volatile *Addend
    )
{
    return __sync_add_and_fetch(Addend, 1);
}

static inline LONG
InterlockedDecrement(
    LONG volatile *Addend
    )
{
    return __sync_sub_and_fetch(Addend, 1);
}

static inline LONG
InterlockedExchange(
    LONG volatile *Target,
    LONG           Value
    )
{
    return __sync_lock_test_and_set(Target, Value);
}

static inline LONG
InterlockedCompareExchange(
    LONG volatile *Destination,
    LONG           Exchange,
    LONG           Comperand
    )
{
    return __sync_val_compare_and_swap(Destination, Comperand, Exchange);
}

static inline LONG
InterlockedExchangeAdd(
    LONG volatile *Addend,
    LONG           Value
    )
{
    return __sync_fetch_and_add(Addend, Value);
}

/* -----------------------------------------------------------------------
 * Fast Mutex Primitive
 * ----------------------------------------------------------------------- */

typedef struct _FAST_MUTEX {
    LONG  Count;
    PVOID Owner;
    ULONG Contention;
    ULONG OldIrql;
} FAST_MUTEX, *PFAST_MUTEX;

VOID
ExInitializeFastMutex(
    PFAST_MUTEX FastMutex
    );

VOID
ExAcquireFastMutex(
    PFAST_MUTEX FastMutex
    );

VOID
ExReleaseFastMutex(
    PFAST_MUTEX FastMutex
    );

BOOLEAN
ExTryToAcquireFastMutex(
    PFAST_MUTEX FastMutex
    );

/* -----------------------------------------------------------------------
 * Lookaside Lists
 * ----------------------------------------------------------------------- */

typedef PVOID (*PALLOCATE_FUNCTION)(MM_POOL_TYPE PoolType, SIZE_T NumberOfBytes, ULONG Tag);
typedef VOID (*PFREE_FUNCTION)(PVOID Buffer);

typedef struct _GENERAL_LOOKASIDE {
    SLIST_HEADER        ListHead;
    USHORT              Depth;
    USHORT              MaximumDepth;
    ULONG               TotalAllocates;
    ULONG               AllocateMisses;
    ULONG               TotalFrees;
    ULONG               FreeMisses;
    MM_POOL_TYPE        Type;
    ULONG               Tag;
    ULONG               Size;
    PALLOCATE_FUNCTION  Allocate;
    PFREE_FUNCTION      Free;
} GENERAL_LOOKASIDE, *PGENERAL_LOOKASIDE;

typedef struct _NPAGED_LOOKASIDE_LIST {
    GENERAL_LOOKASIDE L;
} NPAGED_LOOKASIDE_LIST, *PNPAGED_LOOKASIDE_LIST;

VOID
ExInitializeNPagedLookasideList(
    PNPAGED_LOOKASIDE_LIST Lookaside,
    PALLOCATE_FUNCTION     Allocate,
    PFREE_FUNCTION         Free,
    ULONG                  Flags,
    SIZE_T                 Size,
    ULONG                  Tag,
    USHORT                 Depth
    );

PVOID
ExAllocateFromNPagedLookasideList(
    PNPAGED_LOOKASIDE_LIST Lookaside
    );

VOID
ExFreeToNPagedLookasideList(
    PNPAGED_LOOKASIDE_LIST Lookaside,
    PVOID                  Entry
    );

VOID
ExDeleteNPagedLookasideList(
    PNPAGED_LOOKASIDE_LIST Lookaside
    );

/* -----------------------------------------------------------------------
 * LUID Primitives
 * ----------------------------------------------------------------------- */

VOID
ExAllocateLocallyUniqueId(
    PLUID Luid
    );

/* -----------------------------------------------------------------------
 * Executive Subsystem Initialization
 * ----------------------------------------------------------------------- */

BOOLEAN
ExInitSystem(
    VOID
    );

/* -----------------------------------------------------------------------
 * Work Item Subsystem Primitives
 * ----------------------------------------------------------------------- */

typedef enum _WORK_QUEUE_TYPE {
    CriticalWorkQueue,
    DelayedWorkQueue,
    MaximumWorkQueue
} WORK_QUEUE_TYPE, *PWORK_QUEUE_TYPE;

typedef struct _WORK_QUEUE_ITEM {
    LIST_ENTRY List;
    PVOID WorkerRoutine; 
    PVOID Parameter;    
} WORK_QUEUE_ITEM, *PWORK_QUEUE_ITEM;

VOID
ExpInitializeWorkItems(
    VOID
    );

VOID
ExQueueWorkItem(
    IN PWORK_QUEUE_ITEM WorkItem,
    IN WORK_QUEUE_TYPE QueueType
    );

VOID
ExpProcessWorkItems(
    VOID
    );

#endif /* _EX_H_ */
