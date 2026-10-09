/*++

Copyright (c) 2026  Everywhere Computing, Inc. All Rights Reserved.

Module Name:

    desktop.c

Abstract:

    Desktop background and boot splash screen drawing.

Author:

    Noah Juopperi <nipfswd@gmail.com>
    Clay Sanders (made the first version of the kernel) <claylikepython@yahoo.com>

Environment:

    User-mode

--*/

#include "explorer.h"

/*++

Routine Description:

    Draws the desktop background and taskbar base.

Arguments:

    None.

Return Value:

    None.

--*/

void DrawDesktop(void) {
    /* deep blue background, taskbar */
    FillRect(0, 0, SCR_W, SCR_H, 0x01);
    FillRect(0, SCR_H - 12, SCR_W, 12, 0x08);
    DrawString(4, SCR_H - 10, "Everywhere OS", 0x0F);
}

/*++

Routine Description:

    Displays the boot splash screen with the OS tagline.

Arguments:

    None.

Return Value:

    None.

--*/

void DrawBootScreen(void) {
    FillRect(0, 0, SCR_W, SCR_H, 0x00);
    DrawString(168, 168,
        "Why Go Anywhere When You Are Everywhere?\nEverywhere OS",
        0x0F);
}
