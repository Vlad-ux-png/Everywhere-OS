/*++

Copyright (c) 2026  Everywhere Computing, Inc. All Rights Reserved.

Module Name:

    movemem.c

Abstract:

    Runtime Library memory manipulation routines.

Author:

    Noah Juopperi <nipfswd@gmail.com>

Environment:

    Kernel-mode and User-mode.

--*/

#include "../inc/ntdef.h"
#include "../inc/rtl.h"

VOID
RtlCopyMemory(
    PVOID       Destination,
    const VOID *Source,
    SIZE_T      Length
    )
{
    PUCHAR       d = (PUCHAR)Destination;
    const UCHAR *s = (const UCHAR *)Source;

    while (Length--) {
        *d++ = *s++;
    }
}

VOID
RtlMoveMemory(
    PVOID       Destination,
    const VOID *Source,
    SIZE_T      Length
    )
{
    PUCHAR       d = (PUCHAR)Destination;
    const UCHAR *s = (const UCHAR *)Source;

    if (d == s || Length == 0) {
        return;
    }

    if (d < s) {
        while (Length--) {
            *d++ = *s++;
        }
    } else {
        d += Length;
        s += Length;
        while (Length--) {
            *--d = *--s;
        }
    }
}

VOID
RtlZeroMemory(
    PVOID  Destination,
    SIZE_T Length
    )
{
    PUCHAR d = (PUCHAR)Destination;

    while (Length--) {
        *d++ = 0;
    }
}

VOID
RtlFillMemory(
    PVOID  Destination,
    SIZE_T Length,
    UCHAR  Fill
    )
{
    PUCHAR d = (PUCHAR)Destination;

    while (Length--) {
        *d++ = Fill;
    }
}

SIZE_T
RtlCompareMemory(
    const VOID *Source1,
    const VOID *Source2,
    SIZE_T      Length
    )
{
    const UCHAR *s1 = (const UCHAR *)Source1;
    const UCHAR *s2 = (const UCHAR *)Source2;
    SIZE_T       matched = 0;

    while (Length-- && (*s1++ == *s2++)) {
        matched++;
    }

    return matched;
}

BOOLEAN
RtlEqualMemory(
    const VOID *Source1,
    const VOID *Source2,
    SIZE_T      Length
    )
{
    return (RtlCompareMemory(Source1, Source2, Length) == Length);
}
