/*++

Copyright (c) 2026  Everywhere Computing, Inc. All Rights Reserved.

Module Name:

    bitmapex.c

Abstract:

    Runtime Library Bitmap manipulation primitives.

Author:

    Noah Juopperi <nipfswd@gmail.com>

Environment:

    Kernel-mode and User-mode.

--*/

#include "../inc/ntdef.h"
#include "../inc/rtl.h"

VOID
RtlInitializeBitMap(
    PRTL_BITMAP BitMapHeader,
    PULONG      BitMapBuffer,
    ULONG       SizeOfBitMap
    )
{
    BitMapHeader->SizeOfBitMap = SizeOfBitMap;
    BitMapHeader->Buffer       = BitMapBuffer;
}

VOID
RtlSetBit(
    PRTL_BITMAP BitMapHeader,
    ULONG       BitPosition
    )
{
    if (BitPosition < BitMapHeader->SizeOfBitMap) {
        BitMapHeader->Buffer[BitPosition / 32] |= (1UL << (BitPosition % 32));
    }
}

VOID
RtlClearBit(
    PRTL_BITMAP BitMapHeader,
    ULONG       BitPosition
    )
{
    if (BitPosition < BitMapHeader->SizeOfBitMap) {
        BitMapHeader->Buffer[BitPosition / 32] &= ~(1UL << (BitPosition % 32));
    }
}

BOOLEAN
RtlTestBit(
    PRTL_BITMAP BitMapHeader,
    ULONG       BitPosition
    )
{
    if (BitPosition >= BitMapHeader->SizeOfBitMap) {
        return FALSE;
    }
    return ((BitMapHeader->Buffer[BitPosition / 32] & (1UL << (BitPosition % 32))) != 0);
}

VOID
RtlSetBits(
    PRTL_BITMAP BitMapHeader,
    ULONG       StartingIndex,
    ULONG       NumberToSet
    )
{
    ULONG i;

    for (i = 0; i < NumberToSet; i++) {
        RtlSetBit(BitMapHeader, StartingIndex + i);
    }
}

VOID
RtlClearBits(
    PRTL_BITMAP BitMapHeader,
    ULONG       StartingIndex,
    ULONG       NumberToClear
    )
{
    ULONG i;

    for (i = 0; i < NumberToClear; i++) {
        RtlClearBit(BitMapHeader, StartingIndex + i);
    }
}

BOOLEAN
RtlAreBitsClear(
    PRTL_BITMAP BitMapHeader,
    ULONG       StartingIndex,
    ULONG       Length
    )
{
    ULONG i;

    if (StartingIndex + Length > BitMapHeader->SizeOfBitMap) {
        return FALSE;
    }

    for (i = 0; i < Length; i++) {
        if (RtlTestBit(BitMapHeader, StartingIndex + i)) {
            return FALSE;
        }
    }

    return TRUE;
}

BOOLEAN
RtlAreBitsSet(
    PRTL_BITMAP BitMapHeader,
    ULONG       StartingIndex,
    ULONG       Length
    )
{
    ULONG i;

    if (StartingIndex + Length > BitMapHeader->SizeOfBitMap) {
        return FALSE;
    }

    for (i = 0; i < Length; i++) {
        if (!RtlTestBit(BitMapHeader, StartingIndex + i)) {
            return FALSE;
        }
    }

    return TRUE;
}

ULONG
RtlFindClearBits(
    PRTL_BITMAP BitMapHeader,
    ULONG       NumberToFind,
    ULONG       HintIndex
    )
{
    ULONG TotalBits = BitMapHeader->SizeOfBitMap;
    ULONG CurrentIndex;
    ULONG RunLength;

    if (NumberToFind == 0 || NumberToFind > TotalBits) {
        return 0xFFFFFFFFUL;
    }

    if (HintIndex >= TotalBits) {
        HintIndex = 0;
    }

    CurrentIndex = HintIndex;
    RunLength = 0;

    while (CurrentIndex < TotalBits) {
        if (!RtlTestBit(BitMapHeader, CurrentIndex)) {
            RunLength++;
            if (RunLength == NumberToFind) {
                return CurrentIndex - NumberToFind + 1;
            }
        } else {
            RunLength = 0;
        }
        CurrentIndex++;
    }

    /* Wrap around to test before HintIndex */
    CurrentIndex = 0;
    RunLength = 0;
    while (CurrentIndex < HintIndex) {
        if (!RtlTestBit(BitMapHeader, CurrentIndex)) {
            RunLength++;
            if (RunLength == NumberToFind) {
                return CurrentIndex - NumberToFind + 1;
            }
        } else {
            RunLength = 0;
        }
        CurrentIndex++;
    }

    return 0xFFFFFFFFUL;
}

ULONG
RtlFindSetBits(
    PRTL_BITMAP BitMapHeader,
    ULONG       NumberToFind,
    ULONG       HintIndex
    )
{
    ULONG TotalBits = BitMapHeader->SizeOfBitMap;
    ULONG CurrentIndex;
    ULONG RunLength;

    if (NumberToFind == 0 || NumberToFind > TotalBits) {
        return 0xFFFFFFFFUL;
    }

    if (HintIndex >= TotalBits) {
        HintIndex = 0;
    }

    CurrentIndex = HintIndex;
    RunLength = 0;

    while (CurrentIndex < TotalBits) {
        if (RtlTestBit(BitMapHeader, CurrentIndex)) {
            RunLength++;
            if (RunLength == NumberToFind) {
                return CurrentIndex - NumberToFind + 1;
            }
        } else {
            RunLength = 0;
        }
        CurrentIndex++;
    }

    CurrentIndex = 0;
    RunLength = 0;
    while (CurrentIndex < HintIndex) {
        if (RtlTestBit(BitMapHeader, CurrentIndex)) {
            RunLength++;
            if (RunLength == NumberToFind) {
                return CurrentIndex - NumberToFind + 1;
            }
        } else {
            RunLength = 0;
        }
        CurrentIndex++;
    }

    return 0xFFFFFFFFUL;
}

ULONG
RtlNumberOfClearBits(
    PRTL_BITMAP BitMapHeader
    )
{
    ULONG Count = 0;
    ULONG i;

    for (i = 0; i < BitMapHeader->SizeOfBitMap; i++) {
        if (!RtlTestBit(BitMapHeader, i)) {
            Count++;
        }
    }

    return Count;
}

ULONG
RtlNumberOfSetBits(
    PRTL_BITMAP BitMapHeader
    )
{
    ULONG Count = 0;
    ULONG i;

    for (i = 0; i < BitMapHeader->SizeOfBitMap; i++) {
        if (RtlTestBit(BitMapHeader, i)) {
            Count++;
        }
    }

    return Count;
}
