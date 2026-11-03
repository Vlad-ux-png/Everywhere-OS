/*++

Copyright (c) 2026  O&V Soft. All Rights Reserved.

Module Name:

    po.h

Abstract:

    This header file contains the public definitions, structures, and
    routines for the Power Manager (po) component.

Author(s):

    Vlad Lymar <ggcc98765432110@gmail.com>

--*/

#ifndef _PO_H_
#define _PO_H_

#include "ntdef.h"
#include "ke.h"

//
// Power Action Types
//

typedef enum _POWER_ACTION {
    PowerActionNone = 0,
    PowerActionReserved,
    PowerActionShutdown,
    PowerActionShutdownReset,
    PowerActionHibernate,
    PowerActionShutdownPowerOff
} POWER_ACTION, *PPOWER_ACTION;

//
// Power Manager Exported APIs
//

VOID
NTAPI
PoShutdownSystem(
    IN POWER_ACTION Action
);

NTSTATUS
NTAPI
NtShutdownSystem(
    IN POWER_ACTION Action
);

#endif // _PO_H_
