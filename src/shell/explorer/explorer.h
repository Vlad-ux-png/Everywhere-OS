/*++

Copyright (c) 2026  Everywhere Computing, Inc. All Rights Reserved.

Module Name:

    explorer.h

Abstract:

    Shell and explorer declarations: desktop, taskbar, shell command
    processor, notes, and snake game.

Author:

    Noah Juopperi <nipfswd@gmail.com>
    Clay Sanders (made the first version of the kernel) <claylikepython@yahoo.com>
    Vlad Lymar <ggcc98765432110@gmail.com>
    
Environment:

    User-mode

--*/

#ifndef _EXPLORER_H_
#define _EXPLORER_H_

#include "ke.h"
#include "io.h"

/* ********** Window instances (defined in base/ntos/ke/window.c) ********** */

extern WINDOW ShellWin;
extern WINDOW NotesWin;
extern WINDOW SnakeWin;
extern WINDOW FilesWin;

/* ********** Desktop ********** */

void DrawDesktop(void);
void DrawBootScreen(void);

/* ********** Taskbar ********** */

void DrawTaskbar(void);
void HandleTaskbarClick(void);

/* ********** Shell ********** */

#define SHELL_INPUT_SIZE 64

extern char shell_input[];
extern int  shell_len;
extern char shell_output[];
extern int  shell_has_output;

void ShellClear(void);
void ShellDraw(void);
void ShellExec(void);

/* ********** Notes ********** */

#define NOTES_BUF_SIZE 256

extern char notes_buf[];
extern int  notes_len;

void NotesDraw(void);

/* ********** Files ********** */

void FilesDraw(void);
void FilesHandleClick(int mx, int my);
void FilesHandleKey(int scancode);

/* ********** Icons ********** */

/*
 * IconInit  -- Load C:\Everywhere\Res\F\folder.ico from EVRYFS and decode
 *              it into a 16x16 VGA-palette pixel array.
 */
void IconInit(void);

/*
 * IconDrawFolder -- Blit the decoded 16x16 folder icon to the back buffer
 *                   with top-left corner at xy
 */
void IconDrawFolder(int x, int y);

/* ********** Snake ********** */

#define SNAKE_MAX 100

extern int snake_x[];
extern int snake_y[];
extern int snake_len;
extern int snake_dx;
extern int snake_dy;
extern int food_x;
extern int food_y;

void SnakeInit(void);
void SnakeStep(void);
void SnakeDraw(void);

#endif /* _EXPLORER_H_ */
