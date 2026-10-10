/*++

Copyright (c) 2026  Everywhere Computing, Inc. All Rights Reserved.

Module Name:

    snake.c

Abstract:

    Snake game logic: initialization, movement, food, and rendering.

Author:

    Noah Juopperi <nipfswd@gmail.com>
    Clay Sanders (made the first version of the kernel) <claylikepython@yahoo.com>
    Vlad Lymar <ggcc98765432110@gmail.com>

Environment:

    Userspace

--*/

#include "rtl.h"
#include "explorer.h"

int snake_x[SNAKE_MAX];
int snake_y[SNAKE_MAX];
int snake_len = 25;
int snake_dx = 1;
int snake_dy = 0;
int food_x = 20;
int food_y = 10;

static ULONG rand_seed = 0; 

/*++

Routine Description:

    Initializes the snake body positions.

Arguments:

    None.

Return Value:

    None.

--*/

void SnakeInit(void) {
    rand_seed = KernelGetTickCount();
    for (int i = 0; i < snake_len; i++) {
        snake_x[i] = 30 - i;
        snake_y[i] = 30;
    }
}

/*++

Routine Description:

    Advances the snake by one step, handles boundary clamping and
    food collision.

Arguments:

    None.

Return Value:

    None.

--*/

void SnakeStep(void) {
    static uint32_t LastStepMs = 0;
    uint32_t        Now;

    if (!SnakeWin.visible || SnakeWin.minimized) return;

    //
    // Advance the snake at a fixed 150 ms interval regardless of the
    // render frame rate.  Without this gate the snake would move at
    // 100 Hz (every 10 ms frame), which is unplayably fast.
    //
    Now = KernelGetTickCount();
    if (Now - LastStepMs < 150) return;
    LastStepMs = Now;

    for (int i = snake_len - 1; i > 0; i--) {
        snake_x[i] = snake_x[i - 1];
        snake_y[i] = snake_y[i - 1];
    }
    snake_x[0] += snake_dx;
    snake_y[0] += snake_dy;

    if (snake_x[0] < 0) snake_x[0] = 0;
    if (snake_y[0] < 0) snake_y[0] = 0;
    if (snake_x[0] > SnakeWin.w - 10) snake_x[0] = SnakeWin.w - 10;
    if (snake_y[0] > SnakeWin.h - 20) snake_y[0] = SnakeWin.h - 20;

    for (int i = 0; i < 4; i++) {
        if (snake_x[0] == food_x && snake_y[0] == food_y) {
            if (snake_len < SNAKE_MAX) snake_len++;
    
            food_x = RtlRandom(&rand_seed) % (SnakeWin.w - 10);
            food_y = RtlRandom(&rand_seed) % (SnakeWin.h - 20);
        }
    }
}

/*++

Routine Description:

    Draws the snake body and food inside the snake window.

Arguments:

    None.

Return Value:

    None.

--*/

void SnakeDraw(void) {
    if (!SnakeWin.visible || SnakeWin.minimized) return;

    FillRect(SnakeWin.x + 2, SnakeWin.y + 12, SnakeWin.w - 4, SnakeWin.h - 14, 0x00);

    for (int i = 0; i < snake_len; i++) {
        PutPixel(SnakeWin.x + 5 + snake_x[i],
                 SnakeWin.y + 15 + snake_y[i], 0x0A);
    }

    for (int i = 0; i < 4; i++) {
        PutPixel(SnakeWin.x + 5 + food_x + (i % 2),
                 SnakeWin.y + 15 + food_y + (i / 2), 0x04);
    }
}
