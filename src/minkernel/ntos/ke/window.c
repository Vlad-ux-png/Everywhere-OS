/*++

Copyright (c) 2026  Everywhere Computing, Inc. All Rights Reserved.

Module Name:

    window.c

Abstract:

    Window management: WINDOW instances, frame drawing, physics,
    mouse interaction, and focus tracking.

Author:

    Noah Juopperi <nipfswd@gmail.com>
    Clay Sanders (made the first version of the kernel) <claylikepython@yahoo.com>
    Vlad Lymar <ggcc98765432110@gmail.com>

Environment:

    Kernel-mode only

--*/

#include "ke.h"

WINDOW ShellWin  = { 10, 10, 300, 70, 0, 0, "Shell", 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
WINDOW NotesWin  = { 10, 85, 300, 70, 0, 0, "Notes", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
WINDOW SnakeWin  = { 60, 40, 200, 120, 0, 0, "Snake", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
WINDOW FilesWin  = { 320, 10, 300, 160, 0, 0, "Files", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

/* 0 = Shell, 1 = Notes, 2 = Snake */
int active_window = 0;

/*++

Routine Description:

    Tests whether a point lies within a rectangle.

Arguments:

    x  - Point X coordinate.
    y  - Point Y coordinate.
    rx - Rectangle left edge.
    ry - Rectangle top edge.
    rw - Rectangle width.
    rh - Rectangle height.

Return Value:

    Non-zero if the point is inside the rectangle, zero otherwise.

--*/

int PointInRect(int x, int y, int rx, int ry, int rw, int rh) {
    return (x >= rx && x < rx + rw && y >= ry && y < ry + rh);
}

/*++

Routine Description:

    Draws the window frame including border, body, title bar, close button,
    and minimize button.

Arguments:

    w - Pointer to the WINDOW structure.

Return Value:

    None.

--*/

void DrawWindowFrame(WINDOW* w) {
    if (!w->visible || w->minimized) return;

    /* Border */
    FillRect(w->x - 1, w->y - 1, w->w + 2, w->h + 2, 0x03);
    /* Body */
    FillRect(w->x, w->y, w->w, w->h, 0x00);
    /* Title bar */
    FillRect(w->x, w->y, w->w, 10, 0x0D);
    DrawString(w->x + 3, w->y + 1, w->title, 0x0F);

    /* Close button [X] */
    FillRect(w->x + w->w - 10, w->y + 1, 8, 8, 0x0E);
    DrawChar(w->x + w->w - 9, w->y + 1, 'X', 0x00);

    /* Minimize button [_] */
    FillRect(w->x + w->w - 20, w->y + 1, 8, 8, 0x0E);
    DrawChar(w->x + w->w - 19, w->y + 1, '_', 0x00);

    /* Fullscreen button */
    FillRect(w->x + w->w - 30, w->y + 1, 8, 8, 0x0E);
    DrawChar(w->x + w->w - 29, w->y + 1, w->fullscreen ? 'r' : 'F', 0x00);
}

/*++

Routine Description:

    Applies velocity, friction, and edge-bounce physics to a window.

Arguments:

    w - Pointer to the WINDOW structure.

Return Value:

    None.

--*/

void UpdateWindowPhysics(WINDOW* w) {
    if (!w->visible || w->minimized || w->fullscreen || w->dragging) return;

    w->x += w->vx;
    w->y += w->vy;

    /* friction */
    w->vx = (w->vx * 9) / 10;
    w->vy = (w->vy * 9) / 10;

    /* bounce on edges */
    if (w->x < 0) { w->x = 0; w->vx = -w->vx / 2; }
    if (w->y < 0) { w->y = 0; w->vy = -w->vy / 2; }
    if (w->x + w->w > SCR_W) { w->x = SCR_W - w->w; w->vx = -w->vx / 2; }
    if (w->y + w->h > SCR_H - 12) { w->y = SCR_H - 12 - w->h; w->vy = -w->vy / 2; }
}

/*++

Routine Description:

    Handles mouse interaction with a window: click-to-focus, title bar
    dragging, close button, and minimize button.

Arguments:

    w      - Pointer to the WINDOW structure.
    win_id - Window identifier for focus tracking.

Return Value:

    None.

--*/

void HandleWindowMouse(WINDOW* w, int win_id) {
    if (!w->visible) return;

    int left_down  = mouse_buttons & 1;
    int left_prev  = mouse_prev_buttons & 1;

    /* Click anywhere in window to focus */
    if (left_down && !left_prev && !w->minimized &&
        PointInRect(mouse_x, mouse_y, w->x, w->y, w->w, w->h)) {
        active_window = win_id;
    }

    /* Fullscreen button */
    if (left_down && !left_prev &&
        PointInRect(mouse_x, mouse_y, w->x + w->w - 30, w->y + 1, 8, 8)) {
        if (!w->fullscreen) {
            w->prev_x = w->x;
            w->prev_y = w->y;
            w->prev_w = w->w;
            w->prev_h = w->h;
            w->x = 0;
            w->y = 0;
            w->w = SCR_W;
            w->h = SCR_H - 12;
            w->vx = 0;
            w->vy = 0;
            w->fullscreen = 1;
        } else {
            w->x = w->prev_x;
            w->y = w->prev_y;
            w->w = w->prev_w;
            w->h = w->prev_h;
            w->fullscreen = 0;
        }
    }

    /* Click on title bar to drag */
    if (!w->dragging && !w->fullscreen && left_down && !left_prev &&
        PointInRect(mouse_x, mouse_y, w->x, w->y, w->w, 10)) {
        w->dragging = 1;
        w->drag_off_x = mouse_x - w->x;
        w->drag_off_y = mouse_y - w->y;
        w->vx = 0;
        w->vy = 0;
    }

    if (w->dragging && left_down) {
        int new_x = mouse_x - w->drag_off_x;
        int new_y = mouse_y - w->drag_off_y;
        w->vx = new_x - w->x;
        w->vy = new_y - w->y;
        w->x = new_x;
        w->y = new_y;
    }

    if (w->dragging && !left_down && left_prev) {
        w->dragging = 0;
    }

    /* Close button */
    if (left_down && !left_prev &&
        PointInRect(mouse_x, mouse_y, w->x + w->w - 10, w->y + 1, 8, 8)) {
        w->visible = 0;
    }

    /* Minimize button */
    if (left_down && !left_prev &&
        PointInRect(mouse_x, mouse_y, w->x + w->w - 20, w->y + 1, 8, 8)) {
        w->minimized = 1;
    }
}

/*++

Routine Description:

    Compares two null-terminated strings for equality.

Arguments:

    a - First string.
    b - Second string.

Return Value:

    Non-zero if the strings are equal, zero otherwise.

--*/

int StrEq(const char* a, const char* b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return (*a == 0 && *b == 0);
}
