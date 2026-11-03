/*++

Copyright (c) 2026  O&V Soft. All Rights Reserved.

Module Name:

    ob.h

Abstract:

    Constant definitions and primitives for the Object Manager.

Author:

    Vlad Lymar <ggcc98765432110@gmail.com>

Environment:

    Kernel-mode only.

--*/

#ifndef _OB_H_
#define _OB_H_

#pragma once

#include "ntdef.h"
#include "rtl.h"

#define MAXIMUM_OBJECT_PAYLOAD_SIZE 256

#define OBJECT_POOL_TAG  0x4A4F626A // 'JObj'
#define OB_TYPE_POOL_TAG  0x4F626A54 // 'ObjT'

struct _OBJECT_TYPE;

typedef struct _OBJECT_HEADER {
    LONG PointerCount;          
    LONG HandleCount;       
    struct _OBJECT_TYPE *Type; 
    ULONG Attributes;      
} OBJECT_HEADER, *POBJECT_HEADER;

#define OBJECT_TO_OBJECT_HEADER(o) \
    ((POBJECT_HEADER)((PUCHAR)(o) - sizeof(OBJECT_HEADER)))

#define OBJECT_HEADER_TO_OBJECT(h) \
    ((PVOID)((PUCHAR)(h) + sizeof(OBJECT_HEADER)))

typedef struct _OBJECT_TYPE {
    UNICODE_STRING TypeName;  
    ULONG TotalNumberOfObjects;
    ULONG TotalNumberOfHandles;
    ULONG HighWaterNumberOfObjects;
    ULONG HighWaterNumberOfHandles;
    ULONG PoolType;         
} OBJECT_TYPE, *POBJECT_TYPE;

/* -----------------------------------------------------------------------
 * Object Manager Functions Primitives
 * ----------------------------------------------------------------------- */

VOID
ObInitSystem(
    VOID
    );

NTSTATUS
ObCreateObjectType(
    PCWSTR       TypeNameString,
    MM_POOL_TYPE PoolType
    );

POBJECT_TYPE
ObReferenceObjectTypeByName(
    PCWSTR TypeNameString
    );

NTSTATUS
ObCreateObject(
    POBJECT_TYPE ObjectType,
    ULONG        ObjectSize,
    PVOID       *Object
    );

LONG
ObReferenceObject(
    PVOID Object
    );

LONG
ObDereferenceObject(
    PVOID Object
    );

#endif // _OB_H_
