/*++

Copyright (c) 2026  O&V Soft. All Rights Reserved.

Module Name:

    poapi.c

Abstract:

    This module implements the core Power Manager (po) subsystem APIs.

Author(s):

    Vlad Lymar <ggcc98765432110@gmail.com>

Environment:

    Kernel-mode only.

--*/

#include "po.h"


/*++

Routine Description:

    PoShutdownSystem initiates a system shutdown, reboot, or power-off sequence
    by coordinating with the Hardware Abstraction Layer (HAL).

Arguments:

    Action - Specifies the power action to be performed (e.g., reset, power off).

Return Value:

    None. Does not return if successful.

--*/
VOID
NTAPI
PoShutdownSystem(
    IN POWER_ACTION Action
)
{
    //
    // Dispatch based on the requested power action.
    //
    if (Action == PowerActionShutdownReset) {
        RebootSystem();
    } else {
        return;
    }
}

/*++

Routine Description:

    NtShutdownSystem is the native system service entry point for initiating
    a system shutdown, reboot, or power-off sequence.

Arguments:

    Action - Specifies the power action requested by the caller.

ReturnValue:

    NTSTATUS - Returns an error status only if the shutdown attempt fails.

--*/
NTSTATUS
NTAPI
NtShutdownSystem(
    IN POWER_ACTION Action
)
{
    //
    // Validate the requested power action parameter.
    //
    if ((Action < PowerActionNone) || (Action > PowerActionShutdownPowerOff)) {
        return STATUS_INVALID_PARAMETER;
    }

    PoShutdownSystem(Action);

    //
    // If the system reaches this point, the hardware failed to reboot or power off.
    //
    return STATUS_UNSUCCESSFUL;
}
