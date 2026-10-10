/*++

Copyright (c) 2026  Everywhere Computing, Inc. All Rights Reserved.

Module Name:

    shell.c

Abstract:

    Shell command input and execution (clear, credits, snake, notes).

Author:

    Noah Juopperi <nipfswd@gmail.com>
    Clay Sanders (made the first version of the kernel) <claylikepython@yahoo.com>
    Vlad Lymar <ggcc98765432110@gmail.com>

Environment:

    Userspace

--*/

#include "explorer.h"

char shell_input[SHELL_INPUT_SIZE];
int  shell_len = 0;
char shell_output[256];
int  shell_has_output = 0;

/*++

Routine Description:

    Clears the shell window content area.

Arguments:

    None.

Return Value:

    None.

--*/

void ShellClear(void) {
    FillRect(ShellWin.x + 2, ShellWin.y + 12, ShellWin.w - 4, ShellWin.h - 14, 0x00);
}

/*++

Routine Description:

    Draws the current shell input text inside the shell window.

Arguments:

    None.

Return Value:

    None.

--*/

void ShellDraw(void) {
    if (!ShellWin.visible || ShellWin.minimized) return;
    DrawString(ShellWin.x + 4, ShellWin.y + 14, shell_input, 0x0F);
    if (shell_has_output) {
        DrawString(ShellWin.x + 4, ShellWin.y + 24, shell_output, 0x0A);
        ShellExec();
    }
}

/*++

Routine Description:

    Executes the current shell command. Recognizes "clear", "credits",
    "snake", and "notes".

Arguments:

    None.

Return Value:

    None.

--*/

void ShellExec(void) {
    if (shell_len == 0) return;

    shell_has_output = 0;

    if (StrEq(shell_input, "help")) {
        const char* msg;
        int i = 0;
        msg = "Commands: clear, credits, snake,\nnotes, files, mktest\n";
        while (msg[i] && i < 63) { shell_output[i] = msg[i]; i++; }
        shell_output[i] = 0;
        shell_has_output = 1;
    } else if (StrEq(shell_input, "clear")) {
        ShellClear();
    } else if (StrEq(shell_input, "credits")) {
        const char* msg;
        int i = 0;
        msg = "Clay Sanders, Noah Juopperi\nVlad Lymar\n";
        while (msg[i] && i < 63) { shell_output[i] = msg[i]; i++; }
        shell_output[i] = 0;
        shell_has_output = 1;
    } else if (StrEq(shell_input, "snake")) {
        SnakeWin.visible = 1;
        SnakeWin.minimized = 0;
    } else if (StrEq(shell_input, "notes")) {
        NotesWin.visible = 1;
        NotesWin.minimized = 0;
    } else if (StrEq(shell_input, "files")) {
        FilesWin.visible = 1;
        FilesWin.minimized = 0;
    } else if (StrEq(shell_input, "mktest")) {
        static const uint8_t hello[] = "Hello, World!\n";
        const char* msg;
        int i = 0;
        if (EvryFsWriteFile("hello.txt", hello, 14) == 0) {
            msg = "Created hello.txt";
        } else {
            msg = "No disk found";
        }
        while (msg[i] && i < 63) { shell_output[i] = msg[i]; i++; }
        shell_output[i] = 0;
        shell_has_output = 1;
    }

    shell_len = 0;
    shell_input[0] = 0;
}
