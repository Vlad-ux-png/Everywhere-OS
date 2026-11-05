/*++

Copyright (c) 2026  Everywhere Computing, Inc. All Rights Reserved.

Module Name:

    input.c

Abstract:

    Keyboard input routing to the active window (Shell, Notes, Snake).

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

    Routes a keyboard character to the currently active window.
    Also handles arrow key scancodes for the Snake game.

Arguments:

    ch - ASCII character from GetKeyChar, or 0 if no printable key.

Return Value:

    None.

--*/

void HandleKeyboardInput(char ch) {
    /* Snake arrow key controls (scancodes: up=0x48, down=0x50, left=0x4B, right=0x4D) */
    if (active_window == 2 && SnakeWin.visible && !SnakeWin.minimized) {
        if (last_scancode == 0x48 && snake_dy != 1)  { snake_dx = 0; snake_dy = -1; }
        if (last_scancode == 0x50 && snake_dy != -1) { snake_dx = 0; snake_dy = 1;  }
        if (last_scancode == 0x4B && snake_dx != 1)  { snake_dx = -1; snake_dy = 0; }
        if (last_scancode == 0x4D && snake_dx != -1) { snake_dx = 1;  snake_dy = 0; }
    }

    /* Files window: up/down arrow navigation */
    if (active_window == 3 && FilesWin.visible && !FilesWin.minimized) {
        if (last_scancode == 0x48 || last_scancode == 0x50)
            FilesHandleKey(last_scancode);
    }

    /* Regedit window: arrow navigation, Enter, and shortcuts */
    if (active_window == 4 && RegeditWin.visible && !RegeditWin.minimized) {
        if (last_scancode == 0x48 || last_scancode == 0x50 || last_scancode == 0x1C || last_scancode == 0x31)
            RegeditHandleKey(last_scancode);
    }

    if (!ch) return;

    if (active_window == 0) {
        /* Shell input */
        if (ch == '\n') {
            ShellExec();
        } else if (ch == 0x08) {
            if (shell_len > 0) shell_input[--shell_len] = 0;
        } else if (shell_len < SHELL_INPUT_SIZE - 1) {
            shell_input[shell_len++] = ch;
            shell_input[shell_len] = 0;
        }
    } else if (active_window == 1) {
        /* Notes input */
        if (ch == 0x08) {
            if (notes_len > 0) notes_buf[--notes_len] = 0;
        } else if (notes_len < NOTES_BUF_SIZE - 1) {
            notes_buf[notes_len++] = ch;
            notes_buf[notes_len] = 0;
        }
    } else if (active_window == 4) {
        /* Regedit character text input (for names and values) */
        RegeditHandleChar(ch);
    }
}
