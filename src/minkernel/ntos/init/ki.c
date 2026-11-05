/*++

Copyright (c) 2026  Everywhere Computing, Inc. All Rights Reserved.

Module Name:

    kernel.c
    
Abstract:

    Main entry point for the GUI kernel. All subsystems are now split
    into src\minkernel\ntos\ke\* (kernel) and src\shell\explorer\* (userspace).

Author(s):

    Noah Juopperi <nipfswd@gmail.com>
    Clay Sanders <claylikepython@yahoo.com>
    Vlad Lymar <ggcc98765432110@gmail.com>

Environment:

    Literally the kernel.

--*/

#include "ke.h"
#include "ex.h"
#include "explorer.h"
#include "evryfs.h"
#include "cm.h"

/*++

Routine Description:

    Main entry point for the GUI kernel. Initializes hardware, shows the
    boot screen, then enters the main event loop handling keyboard input,
    mouse interaction, window management, and rendering.

Arguments:

    None.

Return Value:

    None.

--*/

void KiStartupSystem(uint32_t* mbi) {
    KdComPortInitialize();
    MmInit(mbi);
    ExInitSystem();
    CmInitSystem();
    SetupFramebuffer(mbi);
    InitFont();
    InitMouse();
    HalInitInterrupts();
    SnakeInit();
    EvryFsInit();
    IconInit();
    RegeditInit();

    DrawBootScreen();
    FlipBuffers();
    {
        uint32_t BootEnd = KernelGetTickCount() + 2000;
        while (KernelGetTickCount() < BootEnd) {
            __asm__ __volatile__("pause");
        }
    }

    while (1) {
        uint32_t FrameStart = KernelGetTickCount();
        uint32_t FrameElapsed;

        ExpProcessWorkItems();

        last_scancode = 0;
        char ch = KbdClassReadInput();

        /* Route keyboard input to the active window */
        HandleKeyboardInput(ch);

        UpdateMouse();
        SnakeStep();

        HandleWindowMouse(&ShellWin, 0);
        HandleWindowMouse(&NotesWin, 1);
        HandleWindowMouse(&SnakeWin, 2);
        HandleWindowMouse(&FilesWin, 3);
        HandleWindowMouse(&RegeditWin, 4);

        /* Forward left-click-down events into the Files content area. */
        if ((mouse_buttons & 1) && !(mouse_prev_buttons & 1)) {
            FilesHandleClick(mouse_x, mouse_y);
            RegeditHandleClick(mouse_x, mouse_y);
        }

        HandleTaskbarClick();

        UpdateWindowPhysics(&ShellWin);
        UpdateWindowPhysics(&NotesWin);
        UpdateWindowPhysics(&SnakeWin);
        UpdateWindowPhysics(&FilesWin);
        UpdateWindowPhysics(&RegeditWin);

        DrawDesktop();

        WINDOW* wins[5] = { &ShellWin, &NotesWin, &SnakeWin, &FilesWin, &RegeditWin };

        WINDOW* active_p = wins[active_window];
        int dest = 0;
        for (int i = 0; i < 5; i++) {
            if (i != active_window) {
                wins[dest++] = wins[i];
            }
        }
        wins[4] = active_p;

        for (int i = 0; i < 5; i++) {
            DrawWindowFrame(wins[i]);

            if (wins[i] == &ShellWin) ShellDraw();
            else if (wins[i] == &NotesWin) NotesDraw();
            else if (wins[i] == &SnakeWin) SnakeDraw();
            else if (wins[i] == &FilesWin) FilesDraw();
            else if (wins[i] == &RegeditWin) RegeditDraw();
        }

        DrawTaskbar();
        DrawMouseCursor();
        FlipBuffers();

        //
        // Cap the frame rate at ~100 fps (10 ms per frame).  Compute
        // elapsed time since FrameStart and sleep only the remainder so
        // the loop does not busy-spin if all work finished quickly.
        //
        FrameElapsed = KernelGetTickCount() - FrameStart;
        if (FrameElapsed < 10) {
            HalStallExecution(10 - FrameElapsed);
        }
    }
}
