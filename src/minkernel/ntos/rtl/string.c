/*++

Copyright (c) 2026  Everywhere Computing, Inc. All Rights Reserved.

Module Name:

    string.c

Abstract:

    Runtime Library ANSI and Unicode string manipulation routines.

Author:

    Noah Juopperi <nipfswd@gmail.com>

Environment:

    Kernel-mode only.

--*/

#include "../inc/ntdef.h"
#include "../inc/rtl.h"

CHAR
RtlUpperChar(
    CHAR Character
    )
{
    if (Character >= 'a' && Character <= 'z') {
        return (CHAR)(Character - 'a' + 'A');
    }
    return Character;
}

WCHAR
RtlUpcaseUnicodeChar(
    WCHAR SourceCharacter
    )
{
    if (SourceCharacter >= L'a' && SourceCharacter <= L'z') {
        return (WCHAR)(SourceCharacter - L'a' + L'A');
    }
    return SourceCharacter;
}

VOID
RtlInitString(
    PSTRING DestinationString,
    PCSTR   SourceString
    )
{
    DestinationString->Length = 0;
    DestinationString->MaximumLength = 0;
    DestinationString->Buffer = (PCHAR)SourceString;

    if (SourceString != NULL) {
        USHORT Length = 0;
        while (SourceString[Length] != '\0') {
            Length++;
        }
        DestinationString->Length = Length;
        DestinationString->MaximumLength = (USHORT)(Length + 1);
    }
}

VOID
RtlInitAnsiString(
    PANSI_STRING DestinationString,
    PCSTR        SourceString
    )
{
    RtlInitString((PSTRING)DestinationString, SourceString);
}

VOID
RtlInitUnicodeString(
    PUNICODE_STRING DestinationString,
    PCWSTR          SourceString
    )
{
    DestinationString->Length = 0;
    DestinationString->MaximumLength = 0;
    DestinationString->Buffer = (PWSTR)SourceString;

    if (SourceString != NULL) {
        USHORT Length = 0;
        while (SourceString[Length] != L'\0') {
            Length++;
        }
        DestinationString->Length = (USHORT)(Length * sizeof(WCHAR));
        DestinationString->MaximumLength = (USHORT)((Length + 1) * sizeof(WCHAR));
    }
}

LONG
RtlCompareString(
    const STRING *String1,
    const STRING *String2,
    BOOLEAN       CaseInSensitive
    )
{
    USHORT Len1 = String1->Length;
    USHORT Len2 = String2->Length;
    USHORT MinLen = (Len1 < Len2) ? Len1 : Len2;
    USHORT i;

    for (i = 0; i < MinLen; i++) {
        CHAR c1 = String1->Buffer[i];
        CHAR c2 = String2->Buffer[i];

        if (CaseInSensitive) {
            c1 = RtlUpperChar(c1);
            c2 = RtlUpperChar(c2);
        }

        if (c1 != c2) {
            return (LONG)((UCHAR)c1 - (UCHAR)c2);
        }
    }

    return (LONG)(Len1 - Len2);
}

LONG
RtlCompareUnicodeString(
    PCUNICODE_STRING String1,
    PCUNICODE_STRING String2,
    BOOLEAN          CaseInSensitive
    )
{
    USHORT Len1 = (USHORT)(String1->Length / sizeof(WCHAR));
    USHORT Len2 = (USHORT)(String2->Length / sizeof(WCHAR));
    USHORT MinLen = (Len1 < Len2) ? Len1 : Len2;
    USHORT i;

    for (i = 0; i < MinLen; i++) {
        WCHAR c1 = String1->Buffer[i];
        WCHAR c2 = String2->Buffer[i];

        if (CaseInSensitive) {
            c1 = RtlUpcaseUnicodeChar(c1);
            c2 = RtlUpcaseUnicodeChar(c2);
        }

        if (c1 != c2) {
            return (LONG)c1 - (LONG)c2;
        }
    }

    return (LONG)Len1 - (LONG)Len2;
}

BOOLEAN
RtlEqualString(
    const STRING *String1,
    const STRING *String2,
    BOOLEAN       CaseInSensitive
    )
{
    if (String1->Length != String2->Length) {
        return FALSE;
    }
    return (RtlCompareString(String1, String2, CaseInSensitive) == 0);
}

BOOLEAN
RtlEqualUnicodeString(
    PCUNICODE_STRING String1,
    PCUNICODE_STRING String2,
    BOOLEAN          CaseInSensitive
    )
{
    if (String1->Length != String2->Length) {
        return FALSE;
    }
    return (RtlCompareUnicodeString(String1, String2, CaseInSensitive) == 0);
}

VOID
RtlCopyString(
    PSTRING       DestinationString,
    const STRING *SourceString
    )
{
    if (SourceString == NULL) {
        DestinationString->Length = 0;
        return;
    }

    DestinationString->Length = (SourceString->Length < DestinationString->MaximumLength)
        ? SourceString->Length
        : DestinationString->MaximumLength;

    RtlCopyMemory(DestinationString->Buffer, SourceString->Buffer, DestinationString->Length);
}

VOID
RtlCopyUnicodeString(
    PUNICODE_STRING  DestinationString,
    PCUNICODE_STRING SourceString
    )
{
    if (SourceString == NULL) {
        DestinationString->Length = 0;
        return;
    }

    DestinationString->Length = (SourceString->Length < DestinationString->MaximumLength)
        ? SourceString->Length
        : DestinationString->MaximumLength;

    RtlCopyMemory(DestinationString->Buffer, SourceString->Buffer, DestinationString->Length);
}

NTSTATUS
RtlAppendStringToString(
    PSTRING       Destination,
    const STRING *Source
    )
{
    USHORT TotalLength;

    if (Source == NULL || Source->Length == 0) {
        return STATUS_SUCCESS;
    }

    TotalLength = (USHORT)(Destination->Length + Source->Length);
    if (TotalLength > Destination->MaximumLength) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    RtlCopyMemory(Destination->Buffer + Destination->Length,
                  Source->Buffer,
                  Source->Length);
    Destination->Length = TotalLength;
    return STATUS_SUCCESS;
}

NTSTATUS
RtlAppendUnicodeStringToString(
    PUNICODE_STRING  Destination,
    PCUNICODE_STRING Source
    )
{
    USHORT TotalLength;

    if (Source == NULL || Source->Length == 0) {
        return STATUS_SUCCESS;
    }

    TotalLength = (USHORT)(Destination->Length + Source->Length);
    if (TotalLength > Destination->MaximumLength) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    RtlCopyMemory((PUCHAR)Destination->Buffer + Destination->Length,
                  Source->Buffer,
                  Source->Length);
    Destination->Length = TotalLength;
    return STATUS_SUCCESS;
}

#define RTL_TAG_STRING 0x536C7452UL /* 'RtlS' */

NTSTATUS
RtlAnsiStringToUnicodeString(
    PUNICODE_STRING DestinationString,
    PCANSI_STRING   SourceString,
    BOOLEAN         AllocateDestinationString
    )
{
    USHORT Length;
    USHORT i;

    Length = (USHORT)(SourceString->Length * sizeof(WCHAR));
    if (AllocateDestinationString) {
        DestinationString->MaximumLength = (USHORT)(Length + sizeof(WCHAR));
        DestinationString->Buffer = (PWSTR)MmAllocatePool(NonPagedPool,
                                                          DestinationString->MaximumLength,
                                                          RTL_TAG_STRING);
        if (DestinationString->Buffer == NULL) {
            return STATUS_NO_MEMORY;
        }
    } else if (Length > DestinationString->MaximumLength) {
        return STATUS_BUFFER_OVERFLOW;
    }

    DestinationString->Length = Length;
    for (i = 0; i < SourceString->Length; i++) {
        DestinationString->Buffer[i] = (WCHAR)(UCHAR)SourceString->Buffer[i];
    }

    if (DestinationString->MaximumLength >= (USHORT)(Length + sizeof(WCHAR))) {
        DestinationString->Buffer[SourceString->Length] = L'\0';
    }

    return STATUS_SUCCESS;
}

NTSTATUS
RtlUnicodeStringToAnsiString(
    PANSI_STRING     DestinationString,
    PCUNICODE_STRING SourceString,
    BOOLEAN          AllocateDestinationString
    )
{
    USHORT Characters;
    USHORT i;

    Characters = (USHORT)(SourceString->Length / sizeof(WCHAR));
    if (AllocateDestinationString) {
        DestinationString->MaximumLength = (USHORT)(Characters + 1);
        DestinationString->Buffer = (PCHAR)MmAllocatePool(NonPagedPool,
                                                          DestinationString->MaximumLength,
                                                          RTL_TAG_STRING);
        if (DestinationString->Buffer == NULL) {
            return STATUS_NO_MEMORY;
        }
    } else if (Characters > DestinationString->MaximumLength) {
        return STATUS_BUFFER_OVERFLOW;
    }

    DestinationString->Length = Characters;
    for (i = 0; i < Characters; i++) {
        WCHAR wc = SourceString->Buffer[i];
        DestinationString->Buffer[i] = (wc < 0x80) ? (CHAR)wc : '?';
    }

    if (DestinationString->MaximumLength > Characters) {
        DestinationString->Buffer[Characters] = '\0';
    }

    return STATUS_SUCCESS;
}

VOID
RtlFreeAnsiString(
    PANSI_STRING AnsiString
    )
{
    if (AnsiString && AnsiString->Buffer) {
        MmFreePool(AnsiString->Buffer, RTL_TAG_STRING);
        AnsiString->Buffer = NULL;
        AnsiString->Length = 0;
        AnsiString->MaximumLength = 0;
    }
}

VOID
RtlFreeUnicodeString(
    PUNICODE_STRING UnicodeString
    )
{
    if (UnicodeString && UnicodeString->Buffer) {
        MmFreePool(UnicodeString->Buffer, RTL_TAG_STRING);
        UnicodeString->Buffer = NULL;
        UnicodeString->Length = 0;
        UnicodeString->MaximumLength = 0;
    }
}

NTSTATUS
RtlIntegerToChar(
    ULONG Value,
    ULONG Radix,
    LONG  OutputLength,
    PSTR  String
    )
{
    static const CHAR Digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    CHAR  Buf[33];
    LONG  Pos = 0;
    LONG  i;

    if (Radix != 2 && Radix != 8 && Radix != 10 && Radix != 16) {
        return STATUS_INVALID_PARAMETER;
    }

    if (Value == 0) {
        Buf[Pos++] = '0';
    } else {
        while (Value > 0) {
            Buf[Pos++] = Digits[Value % Radix];
            Value /= Radix;
        }
    }

    if (OutputLength > 0 && Pos + 1 > OutputLength) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    for (i = 0; i < Pos; i++) {
        String[i] = Buf[Pos - 1 - i];
    }
    String[Pos] = '\0';

    return STATUS_SUCCESS;
}

NTSTATUS
RtlCharToInteger(
    PCSTR  String,
    ULONG  Radix,
    PULONG Value
    )
{
    ULONG Result = 0;
    BOOLEAN Negative = FALSE;

    if (String == NULL || Value == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    while (*String == ' ' || *String == '\t') {
        String++;
    }

    if (*String == '+') {
        String++;
    } else if (*String == '-') {
        Negative = TRUE;
        String++;
    }

    if (Radix == 0) {
        if (*String == '0' && (*(String + 1) == 'x' || *(String + 1) == 'X')) {
            Radix = 16;
            String += 2;
        } else if (*String == '0') {
            Radix = 8;
        } else {
            Radix = 10;
        }
    }

    while (*String != '\0') {
        ULONG Digit;
        CHAR c = *String;

        if (c >= '0' && c <= '9') {
            Digit = (ULONG)(c - '0');
        } else if (c >= 'a' && c <= 'z') {
            Digit = (ULONG)(c - 'a' + 10);
        } else if (c >= 'A' && c <= 'Z') {
            Digit = (ULONG)(c - 'A' + 10);
        } else {
            break;
        }

        if (Digit >= Radix) {
            break;
        }

        Result = Result * Radix + Digit;
        String++;
    }

    if (Negative) {
        Result = (ULONG)(-(LONG)Result);
    }

    *Value = Result;
    return STATUS_SUCCESS;
}
