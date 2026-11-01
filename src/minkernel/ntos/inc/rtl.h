/*++

Copyright (c) 2026  Everywhere Computing, Inc. All Rights Reserved.

Module Name:

    rtl.h

Abstract:

    Runtime Library (RTL) definitions and public prototypes.

Author:

    Noah Juopperi <nipfswd@gmail.com>
    Vlad Lymar <ggcc98765432110@gmail.com>

Environment:

    Kernel-mode and User-mode.

--*/

#ifndef _RTL_H_
#define _RTL_H_

#include "mm.h"
#include "ntstatus.h"

/* -----------------------------------------------------------------------
 * Character and String Types
 * ----------------------------------------------------------------------- */

typedef uint16_t        WCHAR;
typedef WCHAR          *PWSTR;
typedef const WCHAR    *PCWSTR;
typedef CHAR           *PSTR;
typedef const CHAR     *PCSTR;

typedef struct _STRING {
    USHORT Length;
    USHORT MaximumLength;
    PCHAR  Buffer;
} STRING, *PSTRING, ANSI_STRING, *PANSI_STRING;

typedef const STRING      *PCSTRING;
typedef const ANSI_STRING *PCANSI_STRING;

typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef const UNICODE_STRING *PCUNICODE_STRING;

#define RTL_CONSTANT_STRING(s) \
    { sizeof(s) - sizeof((s)[0]), sizeof(s), (s) }

/* -----------------------------------------------------------------------
 * Bitmap Header
 * ----------------------------------------------------------------------- */

typedef struct _RTL_BITMAP {
    ULONG  SizeOfBitMap;
    PULONG Buffer;
} RTL_BITMAP, *PRTL_BITMAP;

/* -----------------------------------------------------------------------
 * Memory Manipulation Primitives
 * ----------------------------------------------------------------------- */

VOID
RtlCopyMemory(
    PVOID       Destination,
    const VOID *Source,
    SIZE_T      Length
    );

VOID
RtlMoveMemory(
    PVOID       Destination,
    const VOID *Source,
    SIZE_T      Length
    );

VOID
RtlZeroMemory(
    PVOID  Destination,
    SIZE_T Length
    );

VOID
RtlFillMemory(
    PVOID  Destination,
    SIZE_T Length,
    UCHAR  Fill
    );

SIZE_T
RtlCompareMemory(
    const VOID *Source1,
    const VOID *Source2,
    SIZE_T      Length
    );

BOOLEAN
RtlEqualMemory(
    const VOID *Source1,
    const VOID *Source2,
    SIZE_T      Length
    );

/* -----------------------------------------------------------------------
 * String Manipulation Primitives
 * ----------------------------------------------------------------------- */

VOID
RtlInitString(
    PSTRING DestinationString,
    PCSTR   SourceString
    );

VOID
RtlInitAnsiString(
    PANSI_STRING DestinationString,
    PCSTR        SourceString
    );

VOID
RtlInitUnicodeString(
    PUNICODE_STRING DestinationString,
    PCWSTR          SourceString
    );

LONG
RtlCompareString(
    const STRING *String1,
    const STRING *String2,
    BOOLEAN       CaseInSensitive
    );

LONG
RtlCompareUnicodeString(
    PCUNICODE_STRING String1,
    PCUNICODE_STRING String2,
    BOOLEAN          CaseInSensitive
    );

BOOLEAN
RtlEqualString(
    const STRING *String1,
    const STRING *String2,
    BOOLEAN       CaseInSensitive
    );

BOOLEAN
RtlEqualUnicodeString(
    PCUNICODE_STRING String1,
    PCUNICODE_STRING String2,
    BOOLEAN          CaseInSensitive
    );

VOID
RtlCopyString(
    PSTRING       DestinationString,
    const STRING *SourceString
    );

VOID
RtlCopyUnicodeString(
    PUNICODE_STRING  DestinationString,
    PCUNICODE_STRING SourceString
    );

NTSTATUS
RtlAppendStringToString(
    PSTRING       Destination,
    const STRING *Source
    );

NTSTATUS
RtlAppendUnicodeStringToString(
    PUNICODE_STRING  Destination,
    PCUNICODE_STRING Source
    );

NTSTATUS
RtlAnsiStringToUnicodeString(
    PUNICODE_STRING DestinationString,
    PCANSI_STRING   SourceString,
    BOOLEAN         AllocateDestinationString
    );

NTSTATUS
RtlUnicodeStringToAnsiString(
    PANSI_STRING     DestinationString,
    PCUNICODE_STRING SourceString,
    BOOLEAN          AllocateDestinationString
    );

VOID
RtlFreeAnsiString(
    PANSI_STRING AnsiString
    );

VOID
RtlFreeUnicodeString(
    PUNICODE_STRING UnicodeString
    );

CHAR
RtlUpperChar(
    CHAR Character
    );

WCHAR
RtlUpcaseUnicodeChar(
    WCHAR SourceCharacter
    );

NTSTATUS
RtlIntegerToChar(
    ULONG Value,
    ULONG Radix,
    LONG  OutputLength,
    PSTR  String
    );

NTSTATUS
RtlCharToInteger(
    PCSTR  String,
    ULONG  Radix,
    PULONG Value
    );

/*++

Routine Description:

    Compares two wide character strings case-insensitively.

Arguments:

    String1 - Supplies the first string.
    String2 - Supplies the second string.

Return Value:

    0   - Strings are equal.
    < 0 - String1 < String2.
    > 0 - String1 > String2.

--*/
LONG
RtlCompareWcharCaseInsensitive(
    PCWSTR String1,
    PCWSTR String2
);

/* -----------------------------------------------------------------------
 * Bitmap Primitives
 * ----------------------------------------------------------------------- */

VOID
RtlInitializeBitMap(
    PRTL_BITMAP BitMapHeader,
    PULONG      BitMapBuffer,
    ULONG       SizeOfBitMap
    );

VOID
RtlSetBit(
    PRTL_BITMAP BitMapHeader,
    ULONG       BitPosition
    );

VOID
RtlClearBit(
    PRTL_BITMAP BitMapHeader,
    ULONG       BitPosition
    );

BOOLEAN
RtlTestBit(
    PRTL_BITMAP BitMapHeader,
    ULONG       BitPosition
    );

VOID
RtlSetBits(
    PRTL_BITMAP BitMapHeader,
    ULONG       StartingIndex,
    ULONG       NumberToSet
    );

VOID
RtlClearBits(
    PRTL_BITMAP BitMapHeader,
    ULONG       StartingIndex,
    ULONG       NumberToClear
    );

ULONG
RtlFindClearBits(
    PRTL_BITMAP BitMapHeader,
    ULONG       NumberToFind,
    ULONG       HintIndex
    );

ULONG
RtlFindSetBits(
    PRTL_BITMAP BitMapHeader,
    ULONG       NumberToFind,
    ULONG       HintIndex
    );

ULONG
RtlNumberOfClearBits(
    PRTL_BITMAP BitMapHeader
    );

ULONG
RtlNumberOfSetBits(
    PRTL_BITMAP BitMapHeader
    );

BOOLEAN
RtlAreBitsClear(
    PRTL_BITMAP BitMapHeader,
    ULONG       StartingIndex,
    ULONG       Length
    );

BOOLEAN
RtlAreBitsSet(
    PRTL_BITMAP BitMapHeader,
    ULONG       StartingIndex,
    ULONG       Length
    );

/* -----------------------------------------------------------------------
 * Random Number Generator Primitives
 * ----------------------------------------------------------------------- */

ULONG
RtlRandom(
    PULONG Seed
    );

/* -----------------------------------------------------------------------
 * Generic Table Primitives
 * ----------------------------------------------------------------------- */

typedef struct _RTL_GENERIC_TABLE {
    PVOID TableRoot;
    LIST_ENTRY LLHead;
    PVOID LastElementFound;
    ULONG LastIndexFound;
    ULONG NumberOfElements;
    PVOID CompareRoutine;
    PVOID AllocateRoutine;
    PVOID FreeRoutine;
    PVOID TableContext;
    ULONG LastElementIndex;
} RTL_GENERIC_TABLE, *PRTL_GENERIC_TABLE;

typedef struct _RTL_TABLE_NODE {
    struct _RTL_TABLE_NODE *Left;
    struct _RTL_TABLE_NODE *Right;
    struct _RTL_TABLE_NODE *Parent;
    ULONG                   BalanceFactor; 
    CHAR                    Buffer[1];
} RTL_TABLE_NODE, *PRTL_TABLE_NODE;

typedef 
PVOID
(*PRTL_GENERIC_ALLOCATE_ROUTINE)(
    struct _RTL_GENERIC_TABLE *Table,
    CLONG ByteSize
);

typedef 
VOID
(*PRTL_GENERIC_FREE_ROUTINE)(
    struct _RTL_GENERIC_TABLE *Table,
    PVOID Buffer
);

typedef 
LONG
(*PRTL_GENERIC_COMPARE_ROUTINE)(
    struct _RTL_GENERIC_TABLE *Table,
    PVOID FirstStruct,
    PVOID SecondStruct
);

VOID
RtlInitializeGenericTable(
    PRTL_GENERIC_TABLE          Table,
    PRTL_GENERIC_COMPARE_ROUTINE CompareRoutine,
    PRTL_GENERIC_ALLOCATE_ROUTINE AllocateRoutine,
    PRTL_GENERIC_FREE_ROUTINE     FreeRoutine,
    PVOID                       TableContext
    );

ULONG
RtlNumberGenericTableElements(
    PRTL_GENERIC_TABLE Table
    );

BOOLEAN
RtlIsGenericTableEmpty(
    PRTL_GENERIC_TABLE Table
    );

PVOID
NTAPI
RtlGenericTableAllocate(
    PRTL_GENERIC_TABLE Table,
    CLONG ByteSize
    );

VOID
NTAPI
RtlGenericTableFree(
    PRTL_GENERIC_TABLE Table,
    PVOID Buffer
    );

LONG
NTAPI
RtlGenericTableCompare(
    PRTL_GENERIC_TABLE Table,
    PVOID First,
    PVOID Second
    );

#endif /* _RTL_H_ */
