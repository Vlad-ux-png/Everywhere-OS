/*++

Copyright (c) 2026  O&V Soft. All Rights Reserved.

Module Name:

    ob.h

Abstract:

    Constant definitions and primitives for the Object Manager.

Author:

    Vlad Lymar <ggcc98765432110@gmail.com>

Environment:

    Kernel mode.

--*/

#ifndef _OB_H_
#define _OB_H_

#pragma once

#include "ntdef.h"
#include "rtl.h"

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

#endif // _OB_H_
