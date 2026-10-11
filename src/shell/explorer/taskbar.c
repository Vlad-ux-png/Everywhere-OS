/*++

Copyright (c) 2026  Everywhere Computing, Inc. All Rights Reserved.

Module Name:

    taskbar.c

Abstract:

    Taskbar drawing and click handling for minimized window restoration.

Author:

    Noah Juopperi <nipfswd@gmail.com>
    Clay Sanders (made the first version of the kernel) <claylikepython@yahoo.com>
    Vlad Lymar <ggcc98765432110@gmail.com>
    
Environment:

    Userspace

--*/

#include "explorer.h"

/*++

Routine Description:

    Handles mouse clicks on taskbar items to restore minimized windows.

Arguments:

    None.

Return Value:

    None.

--*/

void HandleTaskbarClick(void) {
    int left_down = mouse_buttons & 1;
    int left_prev = mouse_prev_buttons & 1;
    if (!(left_down && !left_prev)) return;
    if (!PointInRect(mouse_x, mouse_y, 0, SCR_H - 12, SCR_W, 12)) return;

    int x = 120;
    if (ShellWin.visible && ShellWin.minimized) {
        if (PointInRect(mouse_x, mouse_y, x, SCR_H - 11, 42, 10)) {
            ShellWin.minimized = 0;
            active_window = 0;
            return;
        }
        x += 44;
    }
    if (NotesWin.visible && NotesWin.minimized) {
        if (PointInRect(mouse_x, mouse_y, x, SCR_H - 11, 42, 10)) {
            NotesWin.minimized = 0;
            active_window = 1;
            return;
        }
        x += 44;
    }
    if (SnakeWin.visible && SnakeWin.minimized) {
        if (PointInRect(mouse_x, mouse_y, x, SCR_H - 11, 42, 10)) {
            SnakeWin.minimized = 0;
            active_window = 2;
            return;
        }
        x += 44;
    }
    if (FilesWin.visible && FilesWin.minimized) {
        if (PointInRect(mouse_x, mouse_y, x, SCR_H - 11, 42, 10)) {
            FilesWin.minimized = 0;
            active_window = 3;
            return;
        }
        x += 44;
    }
}

/*++

Routine Description:

    Draws the taskbar with the OS name and buttons for minimized windows.

Arguments:

    None.

Return Value:

    None.

--*/

void DrawTaskbar(void) {
    FillRect(0, SCR_H - 12, SCR_W, 12, 0x08);
    DrawString(4, SCR_H - 10, "Everywhere OS", 0x0F);

    int x = 120;
    if (ShellWin.visible && ShellWin.minimized) {
        FillRect(x, SCR_H - 11, 42, 10, 0x03);
        DrawString(x + 2, SCR_H - 10, "Shell", 0x0F);
        x += 44;
    }
    if (NotesWin.visible && NotesWin.minimized) {
        FillRect(x, SCR_H - 11, 42, 10, 0x03);
        DrawString(x + 2, SCR_H - 10, "Notes", 0x0F);
        x += 44;
    }
    if (SnakeWin.visible && SnakeWin.minimized) {
        FillRect(x, SCR_H - 11, 42, 10, 0x03);
        DrawString(x + 2, SCR_H - 10, "Snake", 0x0F);
        x += 44;
    }
    if (FilesWin.visible && FilesWin.minimized) {
        FillRect(x, SCR_H - 11, 42, 10, 0x03);
        DrawString(x + 2, SCR_H - 10, "Files", 0x0F);
        x += 44;
    }
}
