/*++

Copyright (c) 2026  O&V Soft. All Rights Reserved.

Module Name:

    regedit.c

Abstract:

    Registry Editor window: clean, straightforward in-memory registry viewer
    and value creator without flat index abstractions.

Author(s):

    Vlad Lymar <ggcc98765432110@gmail.com>

Environment:

    Userspace

--*/

#include "explorer.h"
#include "cm.h"

/* ---- Module state ------------------------------------------------------ */

static int s_selected = -1;  /* index of the selected row */
static PCM_KEY_NODE s_current_key = NULL;

/* Interactive input state for creating values */
char reg_input_name[32] = {0};
int  reg_name_len = 0;

char reg_input_data[32] = {0};
int  reg_data_len = 0;

static int reg_input_mode = 0;  /* 0 = normal, 1 = entering name, 2 = entering data */

/* ---- Internal helpers -------------------------------------------------- */

static void RegeditStrCpy(char* dst, const char* src, int n)
{
    int i = 0;
    while (i < n - 1 && src && src[i]) { dst[i] = src[i]; i++; }
    dst[i] = 0;
}

/* Helper to count total rows to display (Parent '..' + Subkeys + Values) */
static int RegeditGetTotalRows(PCM_KEY_NODE key)
{
    if (!key) return 0;
    int count = 0;

    if (key->Parent) count++;

    PCM_KEY_NODE child = key->Child;
    while (child) {
        count++;
        child = child->Sibling;
    }

    PCM_VALUE_ENTRY val = key->Values;
    while (val) {
        count++;
        val = val->Next;
    }

    return count;
}

/* ---- Public routines --------------------------------------------------- */

void RegeditInit(void)
{
    s_current_key = CmRootKey;
    s_selected = -1;
    reg_input_mode = 0;
}

void RegeditHandleClick(int mx, int my)
{
    if (!RegeditWin.visible || RegeditWin.minimized) return;
    if (reg_input_mode != 0) return;

    int content_top = RegeditWin.y + 20;
    if (mx < RegeditWin.x || mx >= RegeditWin.x + RegeditWin.w) return;
    if (my < content_top || my >= RegeditWin.y + RegeditWin.h - 10) return;

    int row = (my - content_top) / 18;
    int total = RegeditGetTotalRows(s_current_key);

    if (row >= 0 && row < total) {
        s_selected = row;
    }
}

void RegeditHandleKey(int scancode)
{
    if (reg_input_mode != 0) return;

    int total = RegeditGetTotalRows(s_current_key);
    if (total <= 0) return;

    if (scancode == 0x48) { /* Up arrow */
        if (s_selected > 0) s_selected--;
    } else if (scancode == 0x50) { /* Down arrow */
        if (s_selected < total - 1) s_selected++;
    } else if (scancode == 0x1C) { /* Enter key -- open selected subkey or go up */
        int current_index = 0;

        /* 1. Check parent '..' */
        if (s_current_key->Parent) {
            if (s_selected == current_index) {
                s_current_key = s_current_key->Parent;
                s_selected = 0;
                return;
            }
            current_index++;
        }

        /* 2. Check subkeys */
        PCM_KEY_NODE child = s_current_key->Child;
        while (child) {
            if (s_selected == current_index) {
                s_current_key = child;
                s_selected = 0;
                return;
            }
            current_index++;
            child = child->Sibling;
        }
        /* (Values do nothing on Enter in this simple view) */
    } else if (scancode == 0x31) { /* 'N' key -- create new value */
        reg_input_mode = 1;
        reg_name_len = 0;
        reg_data_len = 0;
        reg_input_name[0] = 0;
        reg_input_data[0] = 0;
    }
}

void RegeditHandleChar(char ch)
{
    if (reg_input_mode == 1) {
        if (ch == 0x08) {
            if (reg_name_len > 0) reg_input_name[--reg_name_len] = 0;
        } else if (ch == '\n' || ch == '\r') {
            if (reg_name_len > 0) reg_input_mode = 2;
        } else if (reg_name_len < 31 && ch >= 32 && ch < 127) {
            reg_input_name[reg_name_len++] = ch;
            reg_input_name[reg_name_len] = 0;
        }
    } else if (reg_input_mode == 2) {
        if (ch == 0x08) {
            if (reg_data_len > 0) reg_input_data[--reg_data_len] = 0;
        } else if (ch == '\n' || ch == '\r') {
            if (s_current_key && reg_name_len > 0) {
                CmSetValueKey(s_current_key, reg_input_name, REG_SZ, reg_input_data, reg_data_len + 1);
            }
            reg_input_mode = 0;
            reg_name_len = 0;
            reg_data_len = 0;
        } else if (reg_data_len < 31 && ch >= 32 && ch < 127) {
            reg_input_data[reg_data_len++] = ch;
            reg_input_data[reg_data_len] = 0;
        }
    }
}

void RegeditDraw(void)
{
    if (!RegeditWin.visible || RegeditWin.minimized) return;
    if (!s_current_key) s_current_key = CmRootKey;

    int wx = RegeditWin.x;
    int wy = RegeditWin.y;
    int ww = RegeditWin.w;
    int wh = RegeditWin.h;

    /* Header */
    FillRect(wx, wy + 10, ww, 11, 0x08);
    DrawString(wx + 10, wy + 11, "Name", 0x07);
    DrawString(wx + ww - 72, wy + 11, "Value", 0x07);

    int ry = wy + 21;
    int row_idx = 0;

    /* --- Draw Parent Link '..' if available --- */
    if (s_current_key->Parent) {
        int selected = (row_idx == s_selected);
        FillRect(wx, ry, ww, 18, selected ? 0x01 : 0x00);
        DrawString(wx + 10, ry + 5, "..", selected ? 0x0F : 0x07);
        DrawString(wx + ww - 73, ry + 5, "<Parent>", 0x08);
        ry += 18;
        row_idx++;
    }

    /* --- Draw Subkeys --- */
    PCM_KEY_NODE child = s_current_key->Child;
    while (child && row_idx < 10) {
        int selected = (row_idx == s_selected);
        FillRect(wx, ry, ww, 18, selected ? 0x01 : 0x00);

        char name_buf[20];
        RegeditStrCpy(name_buf, child->Name, 19);
        DrawString(wx + 10, ry + 5, name_buf, selected ? 0x0F : 0x07);
        DrawString(wx + ww - 73, ry + 5, "<Key>", 0x08);

        ry += 18;
        row_idx++;
        child = child->Sibling;
    }

    /* --- Draw Values --- */
    PCM_VALUE_ENTRY val = s_current_key->Values;
    while (val && row_idx < 10) {
        int selected = (row_idx == s_selected);
        FillRect(wx, ry, ww, 18, selected ? 0x01 : 0x00);

        char name_buf[20];
        RegeditStrCpy(name_buf, val->Name, 19);
        DrawString(wx + 10, ry + 5, name_buf, selected ? 0x0F : 0x07);

        if (val->Type == REG_SZ && val->Data) {
            char val_buf[16];
            RegeditStrCpy(val_buf, (char*)val->Data, 15);
            DrawString(wx + ww - 72, ry + 5, val_buf, selected ? 0x0F : 0x07);
        }

        ry += 18;
        row_idx++;
        val = val->Next;
    }

    /* Overlay input prompts if active */
    if (reg_input_mode == 1) {
        FillRect(wx + 10, wy + wh - (wh / 2), ww - 20, 20, 0x0F);
        DrawString(wx + 15, wy + wh - (wh / 2) + 6, "Name: ", 0x00);
        DrawString(wx + 65, wy + wh - (wh / 2) + 6, reg_input_name, 0x00);
    } else if (reg_input_mode == 2) {
        FillRect(wx + 10, wy + wh - (wh / 2), ww - 20, 20, 0x0F);
        DrawString(wx + 15, wy + wh - (wh / 2) + 6, "Value: ", 0x00);
        DrawString(wx + 65, wy + wh - (wh / 2) + 6, reg_input_data, 0x00);
    }

    /* Status bar showing current path */
    int sb_y = wy + wh - 10;
    FillRect(wx, sb_y, ww, 10, 0x08);
    DrawString(wx + 4, sb_y + 1, s_current_key->Name, 0x0F);
}
