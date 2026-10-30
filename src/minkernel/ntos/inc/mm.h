/*++

Copyright (c) 2026  Everywhere Computing, Inc. All Rights Reserved.

Module Name:

    mm.h

Abstract:

    Memory Manager public interface.  Defines all types,
    page-management constants, PFN database structures, virtual address
    descriptor types, pool descriptor, working-set structures, MDL, and
    every exported Mm routine.

Author:

    Noah Juopperi <nipfswd@gmail.com>

Environment:

    Kernel-mode only

--*/

#ifndef _MM_H_
#define _MM_H_

#include <stdint.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"

/* -----------------------------------------------------------------------
 * Fundamental type aliases
 * ----------------------------------------------------------------------- */

typedef uint32_t    ULONG;
typedef int32_t     LONG;
typedef uint16_t    USHORT;
typedef int16_t     SHORT;
typedef uint8_t     UCHAR;
typedef int8_t      CHAR;
typedef void        VOID;
typedef void       *PVOID;
typedef uint8_t     BOOLEAN;
typedef uint32_t    LOGICAL;
typedef int32_t     NTSTATUS;
typedef uint32_t    ULONG_PTR;
typedef int32_t     LONG_PTR;
typedef uint32_t    SIZE_T;
typedef int32_t     SSIZE_T;
typedef uint64_t    ULONGLONG;
typedef int64_t     LONGLONG;
typedef uint32_t    ACCESS_MASK;
typedef uint32_t    PFN_NUMBER;
typedef uint32_t    PFN_COUNT;
typedef uint32_t    WSLE_NUMBER;
typedef ULONG      *PULONG;
typedef ULONG_PTR  *PULONG_PTR;
typedef USHORT     *PUSHORT;
typedef UCHAR      *PUCHAR;
typedef CHAR       *PCHAR;
typedef PVOID      *PPVOID;
typedef BOOLEAN    *PBOOLEAN;
typedef ULONG       CLONG;

typedef union _LARGE_INTEGER {
    struct {
        ULONG LowPart;
        LONG  HighPart;
    };
    LONGLONG QuadPart;
} LARGE_INTEGER, *PLARGE_INTEGER;

typedef struct _LIST_ENTRY {
    struct _LIST_ENTRY *Flink;
    struct _LIST_ENTRY *Blink;
} LIST_ENTRY, *PLIST_ENTRY;

#ifndef TRUE
#define TRUE  ((BOOLEAN)1)
#endif
#ifndef FALSE
#define FALSE ((BOOLEAN)0)
#endif
#ifndef NULL
#define NULL  ((PVOID)0)
#endif

/* -----------------------------------------------------------------------
 * List manipulation
 * ----------------------------------------------------------------------- */

#define InitializeListHead(ListHead) \
    ((ListHead)->Flink = (ListHead)->Blink = (ListHead))

#define IsListEmpty(ListHead) \
    ((ListHead)->Flink == (ListHead))

#define InsertTailList(ListHead, Entry)         \
    {                                           \
        PLIST_ENTRY _H = (ListHead);            \
        PLIST_ENTRY _T = _H->Blink;             \
        (Entry)->Flink = _H;                    \
        (Entry)->Blink = _T;                    \
        _T->Flink      = (Entry);               \
        _H->Blink      = (Entry);               \
    }

#define InsertHeadList(ListHead, Entry)         \
    {                                           \
        PLIST_ENTRY _H = (ListHead);            \
        PLIST_ENTRY _N = _H->Flink;             \
        (Entry)->Flink = _N;                    \
        (Entry)->Blink = _H;                    \
        _N->Blink      = (Entry);               \
        _H->Flink      = (Entry);               \
    }

#define RemoveEntryList(Entry)                  \
    {                                           \
        PLIST_ENTRY _B = (Entry)->Blink;        \
        PLIST_ENTRY _F = (Entry)->Flink;        \
        _B->Flink = _F;                         \
        _F->Blink = _B;                         \
    }

/* -----------------------------------------------------------------------
 * NTSTATUS codes
 * ----------------------------------------------------------------------- */

#include "ntstatus.h"

#define STATUS_GUARD_PAGE_VIOLATION ((NTSTATUS)0x80000001L)

/* -----------------------------------------------------------------------
 * Bug-check codes used by Mm
 * ----------------------------------------------------------------------- */

#define MEMORY_MANAGEMENT           0x0000001AUL
#define PFN_LIST_CORRUPT            0x0000004EUL
#define PAGE_FAULT_IN_NONPAGED_AREA 0x00000050UL
#define BAD_POOL_HEADER             0x00000019UL
#define BAD_POOL_CALLER             0x000000C2UL

/* -----------------------------------------------------------------------
 * Page size and alignment
 * ----------------------------------------------------------------------- */

#define PAGE_SIZE               4096UL
#define PAGE_SHIFT              12
#define PAGE_MASK               (~(PAGE_SIZE - 1UL))
#define PAGE_ALIGN(Va)          ((PVOID)((ULONG_PTR)(Va) & PAGE_MASK))
#define ROUND_TO_PAGES(Size)    (((ULONG_PTR)(Size) + PAGE_SIZE - 1UL) & PAGE_MASK)
#define BYTES_TO_PAGES(Size)    (((ULONG_PTR)(Size) + PAGE_SIZE - 1UL) >> PAGE_SHIFT)
#define PAGES_TO_BYTES(Pages)   ((ULONG_PTR)(Pages) << PAGE_SHIFT)
#define BYTE_OFFSET(Va)         ((ULONG)(((ULONG_PTR)(Va)) & (PAGE_SIZE - 1UL)))

#define MM_EMPTY_LIST           ((PFN_NUMBER)0xFFFFFFFFUL)

/* -----------------------------------------------------------------------
 * Virtual address space layout (32-bit, 3 GB user / 1 GB kernel)
 * ----------------------------------------------------------------------- */

#define MM_LOWEST_USER_ADDRESS      ((PVOID)0x00010000UL)
#define MM_HIGHEST_USER_ADDRESS     ((PVOID)0xBFFFFFFFUL)
#define MM_SYSTEM_RANGE_START       ((PVOID)0xC0000000UL)
#define MM_HIGHEST_SYSTEM_ADDRESS   ((PVOID)0xFFFFFFFFUL)

/* -----------------------------------------------------------------------
 * Pool type
 * ----------------------------------------------------------------------- */

typedef enum _MM_POOL_TYPE {
    NonPagedPool                            = 0,
    PagedPool                               = 1,
    NonPagedPoolMustSucceed                 = 2,
    DontUseThisType                         = 3,
    NonPagedPoolCacheAligned                = 4,
    PagedPoolCacheAligned                   = 5,
    NonPagedPoolCacheAlignedMustSucceed     = 6,
    MaximumPoolType                         = 7
} MM_POOL_TYPE;

/* -----------------------------------------------------------------------
 * PFN page list identifiers
 * ----------------------------------------------------------------------- */

typedef enum _MMLISTS {
    ZeroedPageList          = 0,
    FreePageList            = 1,
    StandbyPageList         = 2,
    ModifiedPageList        = 3,
    ModifiedNoWritePageList = 4,
    BadPageList             = 5,
    ActiveAndValid          = 6,
    TransitionPage          = 7,
    MaximumPageList         = 8
} MMLISTS;

/* -----------------------------------------------------------------------
 * Page protection constants (internal Mm encoding, 5 bits)
 * ----------------------------------------------------------------------- */

#define MM_ZERO_ACCESS          0
#define MM_READONLY             1
#define MM_EXECUTE              2
#define MM_EXECUTE_READ         3
#define MM_READWRITE            4
#define MM_WRITECOPY            5
#define MM_EXECUTE_READWRITE    6
#define MM_EXECUTE_WRITECOPY    7
#define MM_NOCACHE              0x8
#define MM_GUARD_PAGE           0x10
#define MM_NOACCESS             0x18
#define MM_PROTECTION_WRITE_MASK    4
#define MM_PROTECTION_COPY_MASK     1
#define MM_PROTECTION_EXECUTE_MASK  2

/* Win32-style PAGE_* protection flags */
#define PAGE_NOACCESS           0x001
#define PAGE_READONLY           0x002
#define PAGE_READWRITE          0x004
#define PAGE_WRITECOPY          0x008
#define PAGE_EXECUTE            0x010
#define PAGE_EXECUTE_READ       0x020
#define PAGE_EXECUTE_READWRITE  0x040
#define PAGE_EXECUTE_WRITECOPY  0x080
#define PAGE_GUARD              0x100
#define PAGE_NOCACHE            0x200
#define PAGE_WRITECOMBINE       0x400

/* MmAllocateVirtualMemory AllocationType flags */
#define MEM_COMMIT              0x00001000UL
#define MEM_RESERVE             0x00002000UL
#define MEM_DECOMMIT            0x00004000UL
#define MEM_RELEASE             0x00008000UL
#define MEM_FREE                0x00010000UL
#define MEM_PRIVATE             0x00020000UL
#define MEM_MAPPED              0x00040000UL
#define MEM_RESET               0x00080000UL
#define MEM_TOP_DOWN            0x00100000UL

/* -----------------------------------------------------------------------
 * x86 hardware PTE (32-bit non-PAE)
 * ----------------------------------------------------------------------- */

typedef struct _HARDWARE_PTE {
    ULONG Valid          : 1;
    ULONG Write          : 1;
    ULONG Owner          : 1;
    ULONG WriteThrough   : 1;
    ULONG CacheDisable   : 1;
    ULONG Accessed       : 1;
    ULONG Dirty          : 1;
    ULONG LargePage      : 1;
    ULONG Global         : 1;
    ULONG CopyOnWrite    : 1;
    ULONG Prototype      : 1;
    ULONG reserved       : 1;
    ULONG PageFrameNumber : 20;
} HARDWARE_PTE, *PHARDWARE_PTE;

typedef struct _MMPTE_SOFTWARE {
    ULONG Valid          : 1;
    ULONG PageFileLow    : 4;
    ULONG Protection     : 5;
    ULONG Prototype      : 1;
    ULONG Transition     : 1;
    ULONG PageFileHigh   : 20;
} MMPTE_SOFTWARE;

typedef struct _MMPTE_TRANSITION {
    ULONG Valid          : 1;
    ULONG Write          : 1;
    ULONG Owner          : 1;
    ULONG WriteThrough   : 1;
    ULONG CacheDisable   : 1;
    ULONG Protection     : 5;
    ULONG Prototype      : 1;
    ULONG Transition     : 1;
    ULONG PageFrameNumber : 20;
} MMPTE_TRANSITION;

typedef struct _MMPTE_PROTOTYPE {
    ULONG Valid          : 1;
    ULONG ProtoAddressLow : 7;
    ULONG ReadOnly       : 1;
    ULONG WhichPool      : 1;
    ULONG Prototype      : 1;
    ULONG ProtoAddressHigh : 21;
} MMPTE_PROTOTYPE;

typedef union _MMPTE {
    ULONG              Long;
    HARDWARE_PTE       Hard;
    MMPTE_SOFTWARE     Soft;
    MMPTE_TRANSITION   Trans;
    MMPTE_PROTOTYPE    Proto;
} MMPTE, *PMMPTE;

/* -----------------------------------------------------------------------
 * PFN database entry
 * ----------------------------------------------------------------------- */

typedef struct _MMPFNENTRY {
    ULONG PageLocation       : 3;
    ULONG WriteInProgress    : 1;
    ULONG Modified           : 1;
    ULONG ReadInProgress     : 1;
    ULONG CacheAttribute     : 2;
    ULONG PageColor          : 4;
    ULONG PrototypePte       : 1;
    ULONG PageTransition     : 1;
    ULONG InPageError        : 1;
    ULONG SystemChargedPage  : 1;
    ULONG RemovalRequested   : 1;
    ULONG ParityError        : 1;
    ULONG StartOfAllocation  : 1;
    ULONG EndOfAllocation    : 1;
    ULONG spare              : 12;
} MMPFNENTRY;

typedef struct _MMPFN {
    union {
        PFN_NUMBER      Flink;
        ULONG_PTR       ShareCount;
        PMMPTE          PteFrame;
    } u1;
    PMMPTE              PteAddress;
    union {
        ULONG           ReferenceCount;
        struct {
            USHORT      ReferenceCount;
            USHORT      ShortFlags;
        } e2;
    } u2;
    union {
        MMPFNENTRY      e1;
        ULONG           EntireField;
    } u3;
    MMPTE               OriginalPte;
    PFN_NUMBER          Blink;
} MMPFN, *PMMPFN;

/* -----------------------------------------------------------------------
 * Page list head
 * ----------------------------------------------------------------------- */

typedef struct _MM_PAGE_LIST_HEAD {
    PFN_NUMBER  Flink;
    PFN_NUMBER  Blink;
    PFN_NUMBER  Total;
} MM_PAGE_LIST_HEAD, *PMM_PAGE_LIST_HEAD;

/* -----------------------------------------------------------------------
 * Virtual Address Descriptor (VAD)
 * ----------------------------------------------------------------------- */

typedef struct _MMVAD_FLAGS {
    ULONG CommitCharge      : 19;
    ULONG PhysicalMapping   : 1;
    ULONG ImageMap          : 1;
    ULONG UserPhysicalPages : 1;
    ULONG NoChange          : 1;
    ULONG WriteWatch        : 1;
    ULONG PrivateMemory     : 1;
    ULONG TebChpe           : 1;
    ULONG Protection        : 5;
    ULONG Spare             : 1;
} MMVAD_FLAGS;

typedef struct _MMVAD {
    ULONG_PTR           StartingVpn;
    ULONG_PTR           EndingVpn;
    struct _MMVAD      *LeftChild;
    struct _MMVAD      *RightChild;
    struct _MMVAD      *Parent;
    union {
        ULONG           LongFlags;
        MMVAD_FLAGS     VadFlags;
    } u;
    LONG                Balance;
} MMVAD, *PMMVAD;

/* -----------------------------------------------------------------------
 * Working set structures
 * ----------------------------------------------------------------------- */

typedef union _MMWSLE {
    struct {
        ULONG Valid              : 1;
        ULONG LockedInWs         : 1;
        ULONG LockedInMemory     : 1;
        ULONG Protection         : 5;
        ULONG Hashed             : 1;
        ULONG Direct             : 1;
        ULONG Age                : 2;
        ULONG VirtualPageNumber  : 20;
    } e1;
    PVOID VirtualAddress;
    ULONG Long;
} MMWSLE, *PMMWSLE;

#define MM_MAXIMUM_WORKING_SET  2048UL

typedef struct _MMWSL {
    WSLE_NUMBER         FirstFree;
    WSLE_NUMBER         FirstDynamic;
    WSLE_NUMBER         LastEntry;
    WSLE_NUMBER         NextSlot;
    PMMWSLE             Wsle;
    WSLE_NUMBER         LastInitializedWsle;
    WSLE_NUMBER         NonDirectCount;
    WSLE_NUMBER         WorkingSetSize;
    WSLE_NUMBER         MaximumWorkingSetSize;
    WSLE_NUMBER         MinimumWorkingSetSize;
    WSLE_NUMBER         WsleMappingCount;
    MMWSLE              WsleArray[MM_MAXIMUM_WORKING_SET];
} MMWSL, *PMMWSL;

typedef struct _MMSUPPORT {
    WSLE_NUMBER         LastTrimStamp;
    WSLE_NUMBER         NextPageColor;
    PMMWSL              VmWorkingSetList;
    ULONG               PageFaultCount;
    WSLE_NUMBER         WorkingSetSize;
    WSLE_NUMBER         WorkingSetPrivateSize;
    WSLE_NUMBER         MaximumWorkingSetSize;
    WSLE_NUMBER         MinimumWorkingSetSize;
    ULONG               Flags;
    PMMVAD              VadRoot;
} MMSUPPORT, *PMMSUPPORT;

/* -----------------------------------------------------------------------
 * Memory Descriptor List (MDL)
 * ----------------------------------------------------------------------- */

#define MDL_MAPPED_TO_SYSTEM_VA     0x0001
#define MDL_PAGES_LOCKED            0x0002
#define MDL_SOURCE_IS_NONPAGED_POOL 0x0004
#define MDL_ALLOCATED_FIXED_SIZE    0x0008
#define MDL_PARTIAL                 0x0010
#define MDL_IO_PAGE_READ            0x0040
#define MDL_WRITE_OPERATION         0x0080
#define MDL_IO_SPACE                0x0800
#define MDL_NETWORK_HEADER          0x1000
#define MDL_MAPPING_CAN_FAIL        0x2000

typedef struct _MDL {
    struct _MDL        *Next;
    USHORT              Size;
    USHORT              MdlFlags;
    PVOID               Process;
    PVOID               MappedSystemVa;
    PVOID               StartVa;
    ULONG               ByteCount;
    ULONG               ByteOffset;
} MDL, *PMDL;

#define MmInitializeMdl(Mdl, BaseVa, Length)                                  \
    {                                                                          \
        (Mdl)->Next           = (PMDL)NULL;                                   \
        (Mdl)->Size           = (USHORT)(sizeof(MDL) +                        \
            (sizeof(PFN_NUMBER) * BYTES_TO_PAGES(BYTE_OFFSET(BaseVa) + (Length)))); \
        (Mdl)->MdlFlags       = 0;                                            \
        (Mdl)->StartVa        = (PVOID)PAGE_ALIGN(BaseVa);                    \
        (Mdl)->ByteOffset     = BYTE_OFFSET(BaseVa);                          \
        (Mdl)->ByteCount      = (ULONG)(Length);                              \
        (Mdl)->MappedSystemVa = NULL;                                         \
        (Mdl)->Process        = NULL;                                         \
    }

/* -----------------------------------------------------------------------
 * Pool header (8-byte aligned blocks)
 * ----------------------------------------------------------------------- */

#define POOL_BLOCK_SIZE     8UL
#define POOL_BLOCK_SHIFT    3
#define POOL_MAX_BLOCK_SIZE 512UL
#define POOL_SMALL_LISTS    ((ULONG)(POOL_MAX_BLOCK_SIZE / POOL_BLOCK_SIZE))

typedef struct _POOL_HEADER {
    ULONG PreviousSize;  
    ULONG BlockSize;    
    ULONG PoolType;       
    ULONG PoolTag;       
    struct _POOL_HEADER *FreeNext;       
    struct _POOL_HEADER *FreePrev; 
} POOL_HEADER, *PPOOL_HEADER;

//
// Compatibility aliases so existing code using the old struct names compiles.
//
typedef POOL_HEADER  MM_POOL_HEADER;
typedef PPOOL_HEADER PMM_POOL_HEADER;

#define MM_POOL_HEADER_SIZE  ((ULONG)sizeof(POOL_HEADER))
#define MM_POOL_MIN_BLOCK    (MM_POOL_HEADER_SIZE + POOL_BLOCK_SIZE)

/* -----------------------------------------------------------------------
 * Exported globals
 * ----------------------------------------------------------------------- */

extern PMMPFN               MmPfnDatabase;
extern PFN_NUMBER           MmHighestPhysicalPage;
extern PFN_NUMBER           MmNumberOfPhysicalPages;
extern PFN_NUMBER           MmAvailablePages;
extern PFN_NUMBER           MmResidentAvailablePages;
extern ULONG                MmPoolTotalBytes;
extern ULONG                MmPoolFreeBytes;
extern MM_PAGE_LIST_HEAD    MmZeroedPageListHead;
extern MM_PAGE_LIST_HEAD    MmFreePageListHead;
extern MM_PAGE_LIST_HEAD    MmStandbyPageListHead;
extern MM_PAGE_LIST_HEAD    MmModifiedPageListHead;
extern MM_PAGE_LIST_HEAD    MmBadPageListHead;

/* -----------------------------------------------------------------------
 * Exported routines
 * ----------------------------------------------------------------------- */

VOID    MmInit(uint32_t *MultibootInfo);

PVOID   MmAllocatePool(MM_POOL_TYPE PoolType, ULONG NumberOfBytes, ULONG Tag);
VOID    MmFreePool(PVOID P, ULONG Tag);
VOID    MmQueryPoolStats(ULONG *TotalBytes, ULONG *FreeBytes);

PFN_NUMBER  MiAllocatePfn(PMMPTE TargetPte);
VOID        MiFreePfn(PFN_NUMBER PageFrameIndex);

NTSTATUS MmAllocateVirtualMemory(
    PVOID    *BaseAddress,
    ULONG_PTR ZeroBits,
    SIZE_T   *RegionSize,
    ULONG     AllocationType,
    ULONG     Protect);

NTSTATUS MmFreeVirtualMemory(
    PVOID  *BaseAddress,
    SIZE_T *RegionSize,
    ULONG   FreeType);

NTSTATUS MmProtectVirtualMemory(
    PVOID  *BaseAddress,
    SIZE_T *RegionSize,
    ULONG   NewProtect,
    PULONG  OldProtect);

NTSTATUS MmQueryVirtualMemory(
    PVOID   BaseAddress,
    PVOID  *RegionBaseAddress,
    SIZE_T *RegionSize,
    PULONG  State,
    PULONG  Protect,
    PULONG  Type);

NTSTATUS MmAccessFault(
    ULONG  FaultStatus,
    PVOID  VirtualAddress,
    ULONG  PreviousMode,
    PVOID  TrapFrame);

PMDL    MmBuildMdlForNonPagedPool(PVOID VirtualAddress, SIZE_T Length);
VOID    MmProbeAndLockPages(PMDL MemoryDescriptorList, ULONG AccessMode, ULONG Operation);
VOID    MmUnlockPages(PMDL MemoryDescriptorList);
VOID    MmZeroPageThread(VOID);

#pragma GCC diagnostic pop

#endif /* _MM_H_ */
