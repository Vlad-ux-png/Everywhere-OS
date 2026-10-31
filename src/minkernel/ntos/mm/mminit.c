/*++

Copyright (c) 2026  Everywhere Computing, Inc. All Rights Reserved.

Module Name:

    mminit.c

Abstract:

    Memory Manager initialisation.

    MmInit is the single public entry point, called once from kernelMain
    with the Multiboot v1 information pointer.  It performs initialisation
    in this order:

      1. Enable x86 paging with 4 MB PSE identity mapping covering the
         full 4 GB virtual address space (virtual == physical everywhere).

      2. Walk the Multiboot memory map to determine the highest physical
         page frame present in the machine and zero the PFN database that
         is placed at MI_POOL_BASE_MIN (physical/virtual 0x200000).

      3. Walk the memory map a second time to populate the page lists:
         pages below the pool base are recorded as ActiveAndValid (kernel/
         system use); pages above are inserted into the free list.

      4. Initialise the non-paged pool allocator over the physical region
         immediately following the PFN database.

    After MmInit returns, paging is enabled, the PFN database is live,
    and MmAllocatePool is fully operational.

Author:

    Noah Juopperi <nipfswd@gmail.com>
    Vlad Lymar <ggcc98765432110@gmail.com>

Environment:

    Kernel-mode only.  Called once during system startup before any other
    Mm routine.

--*/

#include "../inc/mm.h"
#include "mi.h"

/* -----------------------------------------------------------------------
 * Multiboot v1 information structure layout (byte offsets)
 * ----------------------------------------------------------------------- */

#define MBINFO_OFF_FLAGS        0
#define MBINFO_OFF_MMAP_LENGTH  44
#define MBINFO_OFF_MMAP_ADDR    48

#define MBINFO_FLAG_MMAP        (1UL << 6)

/* Multiboot memory-map entry layout (byte offsets).
 * Stride between entries is MiReadU32(entry, MMAPE_OFF_SIZE) + 4.       */
#define MMAPE_OFF_SIZE      0
#define MMAPE_OFF_BASE_LO   4
#define MMAPE_OFF_BASE_HI   8
#define MMAPE_OFF_LEN_LO    12
#define MMAPE_OFF_LEN_HI    16
#define MMAPE_OFF_TYPE      20
#define MMAPE_TYPE_USABLE   1UL

/* -----------------------------------------------------------------------
 * MiReadU32
 * ----------------------------------------------------------------------- */

/*++

Routine Description:

    Reads a 32-bit unsigned integer from an arbitrary byte offset in
    a byte array.  Uses explicit byte extraction to avoid any undefined
    behaviour from unaligned loads on strict-alignment platforms.

Arguments:

    Ptr    - Base address of the byte array.
    Offset - Byte offset of the 32-bit field.

Return Value:

    Value at that offset, little-endian.

--*/
static ULONG
MiReadU32(
    const UCHAR *Ptr,
    ULONG        Offset
    )
{
    const UCHAR *p = Ptr + Offset;
    ULONG        v;

    v  = (ULONG)p[0];
    v |= (ULONG)p[1] << 8;
    v |= (ULONG)p[2] << 16;
    v |= (ULONG)p[3] << 24;

    return v;
}

/* -----------------------------------------------------------------------
 * MiEnablePaging
 * ----------------------------------------------------------------------- */

/*++

Routine Description:

    Enables x86 32-bit paging with PSE (4 MB page) support.

    A page directory is written at physical address MI_PAGE_DIRECTORY_PHYS
    (0x1000).  Each of the 1024 PDE entries maps a 4 MB window at the same
    physical address as the virtual window (identity mapping):

        PDE[i] = (i << 22) | 0x83

    where 0x83 = Present | Read/Write | PS (4 MB page).

    Paging is activated by:
      1. Setting CR4.PSE (bit 4) to enable large-page support.
      2. Loading CR3 with MI_PAGE_DIRECTORY_PHYS.
      3. Setting CR0.WP (bit 16) and CR0.PG (bit 31).
      4. A near unconditional jump to flush the instruction pipeline.

    After return virtual addresses equal physical addresses for all memory
    that was accessible before the call.

Arguments:

    None.

Return Value:

    None.

--*/
VOID
MiEnablePaging(
    VOID
    )
{
    ULONG *PageDir;
    ULONG  i;

    PageDir = (ULONG *)MI_PAGE_DIRECTORY_PHYS;

    for (i = 0; i < 1024; i++) {
        PageDir[i] = (i << 22) | 0x83U;
    }

    __asm__ __volatile__ (
        //
        // Enable PSE in CR4.
        //
        "mov %%cr4, %%eax       \n\t"
        "or  $0x10, %%eax       \n\t"
        "mov %%eax, %%cr4       \n\t"

        //
        // Load CR3 with the page directory physical address.
        //
        "mov %0, %%eax          \n\t"
        "mov %%eax, %%cr3       \n\t"

        //
        // Enable paging (PG, bit 31) and write-protect (WP, bit 16)
        // in CR0.
        //
        "mov %%cr0, %%eax       \n\t"
        "or  $0x80010000, %%eax \n\t"
        "mov %%eax, %%cr0       \n\t"

        //
        // Pipeline flush: a near jump forces the processor to re-fetch
        // the instruction stream through the now-active TLB.
        //
        "jmp 1f                 \n\t"
        "1:                     \n\t"

        :
        : "i"(MI_PAGE_DIRECTORY_PHYS)
        : "eax", "memory"
    );
}

/* -----------------------------------------------------------------------
 * MiBuildPfnDatabase
 * ----------------------------------------------------------------------- */

/*++

Routine Description:

    Sizes and zeroes the PFN database.

    The database is placed at physical/virtual MI_POOL_BASE_MIN (0x200000).
    Its extent is (HighestPage + 1) * sizeof(MMPFN) bytes, rounded up to
    a page boundary.

    On return:
        MmPfnDatabase         - points to the base of the MMPFN array.
        MmHighestPhysicalPage - highest page frame in the machine.
        MmNumberOfPhysicalPages - count of pages in the machine.
        MiNonPagedPoolStart   - first byte of the pool region (immediately
                                after the PFN database).

Arguments:

    HighestPage - The highest physical page frame number found in the
                  Multiboot memory map.

    TotalPages  - Total count of usable physical pages.

Return Value:

    None.

--*/
static VOID
MiBuildPfnDatabase(
    PFN_NUMBER HighestPage,
    PFN_NUMBER TotalPages
    )
{
    ULONG_PTR PfnSize;
    ULONG_PTR PfnSizeAligned;
    PUCHAR    p;
    ULONG_PTR i;

    MmHighestPhysicalPage   = HighestPage;
    MmNumberOfPhysicalPages = TotalPages;
    MmPfnDatabase           = (PMMPFN)MI_POOL_BASE_MIN;

    //
    // Compute the byte size of the database and round to a page boundary.
    //
    PfnSize        = (ULONG_PTR)(HighestPage + 1) * sizeof(MMPFN);
    PfnSizeAligned = (PfnSize + PAGE_SIZE - 1) & ~(ULONG_PTR)(PAGE_SIZE - 1);

    //
    // Zero every byte of the PFN array.
    //
    p = (PUCHAR)MmPfnDatabase;
    for (i = 0; i < PfnSizeAligned; i++) {
        p[i] = 0;
    }

    //
    // Pool begins immediately after the database.
    //
    MiNonPagedPoolStart = MI_POOL_BASE_MIN + PfnSizeAligned;

    MiNonPagedPoolEnd = MiNonPagedPoolStart + (16UL * 1024UL * 1024UL);
}

/* -----------------------------------------------------------------------
 * MiPopulatePfnLists
 * ----------------------------------------------------------------------- */

/*++

Routine Description:

    Walks the Multiboot memory map a second time and places each usable
    page onto the correct list.

    Pages below MiNonPagedPoolStart are considered reserved (used by the
    kernel image, the page directory, and the PFN database) and their
    MMPFN entries are marked ActiveAndValid with a share count of 1.

    Pages at or above MiNonPagedPoolStart are inserted into the free list
    up to MI_POOL_REGION_MAX bytes from the pool base to avoid making the
    pool unreasonably large on machines with many gigabytes of RAM.  The
    pool end is recorded in MiNonPagedPoolEnd.

Arguments:

    Mbi        - Pointer to the Multiboot information structure.

    MmapLength - Value of the mmap_length field (pre-read by caller).

    MmapAddr   - Value of the mmap_addr field (pre-read by caller).

Return Value:

    None.

--*/
static VOID
MiPopulatePfnLists(
    const UCHAR *Mbi,
    ULONG        MmapLength,
    ULONG        MmapAddr
    )
{
    const UCHAR *entry;
    const UCHAR *mmap_end;
    ULONG        stride;
    ULONG        base_lo;
    ULONG        base_hi;
    ULONG        len_lo;
    ULONG        len_hi;
    ULONG        type;
    PFN_NUMBER   StartPage;
    PFN_NUMBER   EndPage;
    PFN_NUMBER   Page;
    PFN_NUMBER   SystemEndPage;
    PFN_NUMBER   PoolEndPage;
    PMMPFN       Pfn;

    (VOID)Mbi;

    SystemEndPage = (PFN_NUMBER)(MiNonPagedPoolStart >> PAGE_SHIFT);
    PoolEndPage   = (PFN_NUMBER)((MiNonPagedPoolStart + MI_POOL_REGION_MAX) >> PAGE_SHIFT);

    if (PoolEndPage > MmHighestPhysicalPage) {
        PoolEndPage = MmHighestPhysicalPage;
    }

    //
    // Record where the pool ends in virtual address terms.
    //
    MiNonPagedPoolEnd = (ULONG_PTR)PoolEndPage << PAGE_SHIFT;

    entry    = (const UCHAR *)MmapAddr;
    mmap_end = entry + MmapLength;

    while (entry < mmap_end) {
        stride  = MiReadU32(entry, MMAPE_OFF_SIZE);
        base_lo = MiReadU32(entry, MMAPE_OFF_BASE_LO);
        base_hi = MiReadU32(entry, MMAPE_OFF_BASE_HI);
        len_lo  = MiReadU32(entry, MMAPE_OFF_LEN_LO);
        len_hi  = MiReadU32(entry, MMAPE_OFF_LEN_HI);
        type    = MiReadU32(entry, MMAPE_OFF_TYPE);

        if (type == MMAPE_TYPE_USABLE &&
            base_hi == 0 &&
            len_hi  == 0 &&
            len_lo  >  0) {

            StartPage = (PFN_NUMBER)(base_lo >> PAGE_SHIFT);
            EndPage   = (PFN_NUMBER)((base_lo + len_lo - 1) >> PAGE_SHIFT);

            if (EndPage > MmHighestPhysicalPage) {
                EndPage = MmHighestPhysicalPage;
            }

            for (Page = StartPage; Page <= EndPage; Page++) {

                if (Page >= SystemEndPage && Page <= PoolEndPage) {

                    //
                    // Free page -- add to the free list and update the
                    // PFN entry's list membership.
                    //
                    Pfn = MI_PFN_ELEMENT(Page);
                    Pfn->u3.e1.PageLocation = (ULONG)FreePageList;
                    MiInsertPageInFreeList(Page);

                } else if (Page < SystemEndPage) {

                    //
                    // System-reserved page (kernel, page directory, or
                    // PFN database).  Mark it active so the eviction path
                    // never attempts to steal it.
                    //
                    Pfn = MI_PFN_ELEMENT(Page);
                    Pfn->u1.ShareCount      = 1;
                    Pfn->u2.ReferenceCount  = 1;
                    Pfn->u3.e1.PageLocation = (ULONG)ActiveAndValid;
                }
                //
                // Pages above PoolEndPage exist but are not managed;
                // their MMPFN entries remain zeroed.
                //
            }
        }

        entry += stride + 4;
    }
}

/* -----------------------------------------------------------------------
 * MmInit
 * ----------------------------------------------------------------------- */

/*++

Routine Description:

    Memory Manager initialisation entry point.  Called once from
    kernelMain immediately after the kernel image is loaded.

    Sequence of operations:
      1. Enable paging (4 MB PSE identity map).
      2. Walk Multiboot mmap to find HighestPage and TotalPages.
      3. Build and zero the PFN database.
      4. Walk mmap again to populate page lists.
      5. Initialise the non-paged pool allocator.

    On return the Mm subsystem is fully operational.

Arguments:

    MultibootInfo - Pointer to the Multiboot v1 information block as
                    passed in EBX by GRUB.

Return Value:

    None.

--*/
VOID
MmInit(
    uint32_t *MultibootInfo
    )
{
    const UCHAR *mbi;
    ULONG        flags;
    ULONG        mmap_length;
    ULONG        mmap_addr;
    const UCHAR *entry;
    const UCHAR *mmap_end;
    ULONG        stride;
    ULONG        base_lo;
    ULONG        base_hi;
    ULONG        len_lo;
    ULONG        len_hi;
    ULONG        type;
    PFN_NUMBER   last_page;
    PFN_NUMBER   highest;
    PFN_NUMBER   total;
    ULONG        pool_size;

    //
    // Step 1: enable paging.  After this call virtual == physical.
    //
    MiEnablePaging();

    mbi   = (const UCHAR *)MultibootInfo;
    flags = MiReadU32(mbi, MBINFO_OFF_FLAGS);

    if (!(flags & MBINFO_FLAG_MMAP)) {

        //
        // No memory map: fall back to a minimal 4 MB pool and skip
        // the full PFN database.  The system will function but with
        // no page-list management.
        //
        MmHighestPhysicalPage   = (PFN_NUMBER)((MI_POOL_BASE_MIN + MI_POOL_FALLBACK_SIZE) >> PAGE_SHIFT);
        MmNumberOfPhysicalPages = (PFN_NUMBER)(MI_POOL_FALLBACK_SIZE >> PAGE_SHIFT);
        MmPfnDatabase           = (PMMPFN)MI_POOL_BASE_MIN;
        MiNonPagedPoolStart     = MI_POOL_BASE_MIN + PAGE_SIZE;
        MiNonPagedPoolEnd       = MiNonPagedPoolStart + MI_POOL_FALLBACK_SIZE;
        MiInitializeNonPagedPool(MiNonPagedPoolStart,
                                 (ULONG)(MiNonPagedPoolEnd - MiNonPagedPoolStart));
        return;
    }

    mmap_length = MiReadU32(mbi, MBINFO_OFF_MMAP_LENGTH);
    mmap_addr   = MiReadU32(mbi, MBINFO_OFF_MMAP_ADDR);

    //
    // Step 2: first pass -- find HighestPage and TotalPages.
    //
    highest = 0;
    total   = 0;
    entry   = (const UCHAR *)mmap_addr;
    mmap_end = entry + mmap_length;

    while (entry < mmap_end) {
        stride  = MiReadU32(entry, MMAPE_OFF_SIZE);
        base_lo = MiReadU32(entry, MMAPE_OFF_BASE_LO);
        base_hi = MiReadU32(entry, MMAPE_OFF_BASE_HI);
        len_lo  = MiReadU32(entry, MMAPE_OFF_LEN_LO);
        len_hi  = MiReadU32(entry, MMAPE_OFF_LEN_HI);
        type    = MiReadU32(entry, MMAPE_OFF_TYPE);

        if (type == MMAPE_TYPE_USABLE &&
            base_hi == 0 &&
            len_hi  == 0 &&
            len_lo  >  0) {

            last_page = (PFN_NUMBER)((base_lo + len_lo - 1) >> PAGE_SHIFT);

            if (last_page > highest) {
                highest = last_page;
            }

            total += (PFN_NUMBER)(len_lo >> PAGE_SHIFT);
        }

        entry += stride + 4;
    }

    if (highest == 0) {
        highest = (PFN_NUMBER)((MI_POOL_BASE_MIN + MI_POOL_FALLBACK_SIZE) >> PAGE_SHIFT);
        total   = (PFN_NUMBER)(MI_POOL_FALLBACK_SIZE >> PAGE_SHIFT);
    }

    //
    // Step 3: build and zero the PFN database.
    //
    MiBuildPfnDatabase(highest, total);

    //
    // Initialise page list head sentinels.
    //
    MmZeroedPageListHead.Flink  = MM_EMPTY_LIST;
    MmZeroedPageListHead.Blink  = MM_EMPTY_LIST;
    MmZeroedPageListHead.Total  = 0;
    MmFreePageListHead.Flink    = MM_EMPTY_LIST;
    MmFreePageListHead.Blink    = MM_EMPTY_LIST;
    MmFreePageListHead.Total    = 0;
    MmStandbyPageListHead.Flink = MM_EMPTY_LIST;
    MmStandbyPageListHead.Blink = MM_EMPTY_LIST;
    MmStandbyPageListHead.Total = 0;
    MmModifiedPageListHead.Flink = MM_EMPTY_LIST;
    MmModifiedPageListHead.Blink = MM_EMPTY_LIST;
    MmModifiedPageListHead.Total = 0;
    MmBadPageListHead.Flink     = MM_EMPTY_LIST;
    MmBadPageListHead.Blink     = MM_EMPTY_LIST;
    MmBadPageListHead.Total     = 0;
    MmAvailablePages            = 0;
    MmResidentAvailablePages    = 0;

    //
    // Step 4: populate page lists.
    //
    MiPopulatePfnLists(mbi, mmap_length, mmap_addr);

    //
    // Step 5: initialise the pool allocator.  The pool occupies
    // [MiNonPagedPoolStart, MiNonPagedPoolEnd).
    //
    pool_size = (ULONG)(MiNonPagedPoolEnd - MiNonPagedPoolStart);

    if (pool_size < MM_POOL_MIN_BLOCK) {
        KeBugCheckEx(MEMORY_MANAGEMENT, 0x30, MiNonPagedPoolStart,
                     MiNonPagedPoolEnd, 0);
    }

    MiInitializeNonPagedPool(MiNonPagedPoolStart, pool_size);
}
