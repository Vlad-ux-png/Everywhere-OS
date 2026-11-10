/*++

Copyright (c) 2026  O&V Soft. All Rights Reserved.

Module Name:

    io.h

Abstract:

    Primitives for the io.

Author:

    Vlad Lymar <ggcc98765432110@gmail.com>

Environment:

    Kernel-mode only.

--*/

#ifndef _IO_H_
#define _IO_H_

#include "../inc/ntdef.h"
#include "../inc/ob.h"

NTSTATUS
NtOpenFile(
    PCHAR  FileName,
    PPVOID OutFileObject
);

NTSTATUS
NtCloseFile(
    PVOID FileObject
);

NTSTATUS
NtReadFile(
    PVOID  FileObject,
    PVOID  Buffer,
    ULONG  Length,
    PULONG BytesRead
);

NTSTATUS
NtWriteFile(
    PVOID  FileObject,
    PVOID  Buffer,
    ULONG  Length,
    PULONG BytesWritten
);

NTSTATUS
NtQueryDirectory(
    PVOID  FileObject,
    PVOID  Buffer,
    ULONG  Length,
    PULONG EntriesCount
);

NTSTATUS
NtCreateFile(
    PCHAR FileName,
    PCHAR InitialData,
    ULONG Length,
    PVOID OutFileObject
);

#endif /* _IO_H_ */
