/*++

Copyright (c) 2026  Everywhere Computing, Inc. All Rights Reserved.

Module Name:

    files.c

Abstract:

    Files window: interactive EVRYFS directory listing with scrolling,
    keyboard navigation, mouse selection, column headers, and a status bar.

    Layout (window h = FILES_WIN_H):
        Title bar      10 px   (drawn by DrawWindowFrame)
        Header row     11 px   column labels + separator
        Row area       variable (FILES_ROW_H px per entry)
        Status bar     10 px   file count + selected entry info

Author:

    Noah Juopperi <nipfswd@gmail.com>
    Vlad Lymar <ggcc98765432110@gmail.com>

Environment:

    Userspace

--*/

#include "explorer.h"
#include "evryfs.h"

/* ---- Layout constants -------------------------------------------------- */

#define FILES_ROW_H     18   /* pixels per directory entry row              */
#define FILES_HEADER_H  11   /* header area height (below title bar)        */
#define FILES_STATUS_H  10   /* status bar height (at window bottom)        */

/* Number of visible rows given the current window height. */
#define FILES_ROWS_VIS  (((FilesWin.h) - FILES_HEADER_H - FILES_STATUS_H) / FILES_ROW_H)

/* ---- Module state ------------------------------------------------------ */

static int s_scroll   = 0;   /* index of the first visible row              */
static int s_selected = -1;  /* index of the selected entry (-1 = none)     */

/* Cached listing refreshed each frame so click/key handlers can use it. */
static char     s_names[EVRYFS_MAX_FILES][EVRYFS_NAME_LEN];
static uint32_t s_sizes[EVRYFS_MAX_FILES];
static int      s_count = 0;

/* ---- Internal helpers -------------------------------------------------- */

static int FilesStrLen(const char* s)
{
    int n = 0; while (s[n]) n++; return n;
}

static void FilesStrCpy(char* dst, const char* src, int n)
{
    int i = 0;
    while (i < n - 1 && src[i]) { dst[i] = src[i]; i++; }
    dst[i] = 0;
}

/*++

Routine Description:

    Formats a byte count as a human-readable string:
        < 1 KB  -> "NNN B"
        < 1 MB  -> "N.N KB"
        >= 1 MB -> "N.N MB"

Arguments:

    n   - Byte count.
    buf - Destination buffer (at least 12 bytes).

Return Value:

    None.

--*/
static void FilesFormatSize(uint32_t n, char* buf)
{
    /* Helper: append decimal integer to buf[*pos]. */
    char tmp[12];
    int  i = 0, j;

    if (n == 0) { buf[0] = '0'; buf[1] = ' '; buf[2] = 'B'; buf[3] = 0; return; }

    /* Choose unit. */
    if (n >= 1048576u) {
        uint32_t whole = n / 1048576u;
        uint32_t frac  = (n % 1048576u) * 10u / 1048576u;
        /* whole */
        uint32_t w2 = whole;
        if (w2 == 0) { tmp[i++] = '0'; } else { while (w2) { tmp[i++] = (char)('0' + w2 % 10); w2 /= 10; } }
        for (j = 0; j < i / 2; j++) { char t = tmp[j]; tmp[j] = tmp[i-1-j]; tmp[i-1-j] = t; }
        int p = 0;
        for (j = 0; j < i; j++) buf[p++] = tmp[j];
        buf[p++] = '.'; buf[p++] = (char)('0' + frac);
        buf[p++] = ' '; buf[p++] = 'M'; buf[p++] = 'B'; buf[p] = 0;
    } else if (n >= 1024u) {
        uint32_t whole = n / 1024u;
        uint32_t frac  = (n % 1024u) * 10u / 1024u;
        uint32_t w2 = whole;
        if (w2 == 0) { tmp[i++] = '0'; } else { while (w2) { tmp[i++] = (char)('0' + w2 % 10); w2 /= 10; } }
        for (j = 0; j < i / 2; j++) { char t = tmp[j]; tmp[j] = tmp[i-1-j]; tmp[i-1-j] = t; }
        int p = 0;
        for (j = 0; j < i; j++) buf[p++] = tmp[j];
        buf[p++] = '.'; buf[p++] = (char)('0' + frac);
        buf[p++] = ' '; buf[p++] = 'K'; buf[p++] = 'B'; buf[p] = 0;
    } else {
        uint32_t w2 = n;
        while (w2) { tmp[i++] = (char)('0' + w2 % 10); w2 /= 10; }
        for (j = 0; j < i / 2; j++) { char t = tmp[j]; tmp[j] = tmp[i-1-j]; tmp[i-1-j] = t; }
        int p = 0;
        for (j = 0; j < i; j++) buf[p++] = tmp[j];
        buf[p++] = ' '; buf[p++] = 'B'; buf[p] = 0;
    }
}

/* ---- Public routines --------------------------------------------------- */

/*++

Routine Description:

    Handles a left-click at (mx, my) inside the Files window content area.
    Clicking a row selects that entry; clicking the same row again does nothing
    extra (future: open file viewer).

Arguments:

    mx - Mouse X coordinate.
    my - Mouse Y coordinate.

Return Value:

    None.

--*/
void FilesHandleClick(int mx, int my)
{
    if (!FilesWin.visible || FilesWin.minimized) return;

    int content_top = FilesWin.y + 10 + FILES_HEADER_H;
    int content_bot = FilesWin.y + FilesWin.h - FILES_STATUS_H;

    if (mx < FilesWin.x || mx >= FilesWin.x + FilesWin.w) return;
    if (my < content_top || my >= content_bot) return;

    int row = (my - content_top) / FILES_ROW_H;
    int idx = s_scroll + row;
    if (idx >= 0 && idx < s_count) {
        s_selected = idx;
    }
}

/*++

Routine Description:

    Handles a keyboard scancode for the Files window.
    Up arrow moves the selection up; down arrow moves it down.
    The scroll offset follows the selection to keep it visible.

Arguments:

    scancode - Raw keyboard scancode.

Return Value:

    None.

--*/
void FilesHandleKey(int scancode)
{
    if (s_count == 0) return;

    int vis = FILES_ROWS_VIS;

    if (scancode == 0x48) { /* up arrow */
        if (s_selected < 0) s_selected = 0;
        else if (s_selected > 0) s_selected--;
        if (s_selected < s_scroll) s_scroll = s_selected;

    } else if (scancode == 0x50) { /* down arrow */
        if (s_selected < 0) s_selected = 0;
        else if (s_selected < s_count - 1) s_selected++;
        if (s_selected >= s_scroll + vis) s_scroll = s_selected - vis + 1;
    }
}

/*++

Routine Description:

    Draws the EVRYFS directory listing inside the Files window.

    Column layout:
        x+2          folder icon (16x16)
        x+20         filename, truncated to fit the column
        x+w-48       right-aligned size string

    Special messages:
        "Empty"      -- volume mounted but no files
        "No disk"    -- no ATA disk detected

Arguments:

    None.

Return Value:

    None.

--*/
void FilesDraw(void)
{
    if (!FilesWin.visible || FilesWin.minimized) return;

    /* Buffer for directory entries fetched via NtQueryDirectory */
    EVRYFS_DIR_ENTRY_INFO entries[EVRYFS_MAX_FILES];
    ULONG s_count = 0;

    /* Refresh directory listing using the new NT-style service */
    if (!NT_SUCCESS(NtQueryDirectory(NULL, entries, sizeof(entries), &s_count))) {
        s_count = 0;
    }

    /* Clamp scroll so it never leaves the list. */
    int vis = FILES_ROWS_VIS;
    if (vis < 1) vis = 1;
    if (s_scroll > (int)s_count - vis) s_scroll = (int)s_count - vis;
    if (s_scroll < 0) s_scroll = 0;
    if (s_selected >= (int)s_count) s_selected = (int)s_count - 1;

    int wx = FilesWin.x;
    int wy = FilesWin.y;
    int ww = FilesWin.w;
    int wh = FilesWin.h;

    /* ---- Header row ---- */
    int hdr_y = wy + 10;
    FillRect(wx, hdr_y, ww, FILES_HEADER_H, 0x08);  /* dark grey header bg */
    DrawString(wx + 20, hdr_y + 1, "Name", 0x07);

    /* Right-align "Size" label */
    {
        const char* slbl = "Size";
        int lx = wx + ww - (4 * 8) - 4; /* 4 chars * 8px wide */
        DrawString(lx, hdr_y + 1, slbl, 0x07);
    }

    /* Separator line */
    FillRect(wx, hdr_y + FILES_HEADER_H - 1, ww, 1, 0x07);

    /* ---- Directory rows ---- */
    int row_top = hdr_y + FILES_HEADER_H;

    if (s_count == 0) {
        DrawString(wx + 4, row_top + 4, "Empty", 0x07);
        goto draw_status;
    }

    for (int i = 0; i < vis; i++) {
        int idx = s_scroll + i;
        if (idx >= (int)s_count) break;

        int ry = row_top + (i * FILES_ROW_H);
        uint8_t row_bg  = (idx == s_selected) ? 0x01 : 0x00; /* blue / black */
        uint8_t txt_col = (idx == s_selected) ? 0x0F : 0x07;

        FillRect(wx, ry, ww, FILES_ROW_H, row_bg);

        /* Folder icon */
        IconDrawFolder(wx + 2, ry + 1);

        /* Filename -- truncate to 18 visible characters */
        char name_buf[20];
        FilesStrCpy(name_buf, entries[idx].FileName, 19);
        if (FilesStrLen(entries[idx].FileName) > 18) {
            name_buf[16] = '.'; name_buf[17] = '.'; name_buf[18] = 0;
        }
        DrawString(wx + 20, ry + 5, name_buf, txt_col);

        /* Size string, right-aligned in the last 48 pixels of the row */
        char sz[12];
        FilesFormatSize(entries[idx].FileSize, sz);
        int slen = FilesStrLen(sz);
        int sx   = wx + ww - (slen * 8) - 4;
        DrawString(sx, ry + 5, sz, txt_col);
    }

    /* Scroll indicator: small tick marks on the right edge */
    if (s_count > (ULONG)vis) {
        int track_top = row_top;
        int track_h   = vis * FILES_ROW_H;
        int thumb_y   = track_top + (s_scroll * track_h) / s_count;
        int thumb_h   = (vis * track_h) / s_count;
        if (thumb_h < 2) thumb_h = 2;
        FillRect(wx + ww - 3, track_top, 3, track_h, 0x08);
        FillRect(wx + ww - 3, thumb_y,   3, thumb_h, 0x07);
    }

draw_status:;
    /* ---- Status bar ---- */
    int sb_y = wy + wh - FILES_STATUS_H;
    FillRect(wx, sb_y, ww, FILES_STATUS_H, 0x08);
    FillRect(wx, sb_y, ww, 1, 0x07);   /* top border */

    /* Left: file count */
    {
        char cnt[32];
        int  p = 0;
        char dig[4]; int di = 0;
        ULONG nn = s_count;
        if (nn == 0) { dig[di++] = '0'; }
        else { while (nn) { dig[di++] = (char)('0' + nn % 10); nn /= 10; } }
        for (int k = di - 1; k >= 0; k--) cnt[p++] = dig[k];
        const char* lbl = s_count == 1 ? " file" : " files";
        for (int k = 0; lbl[k]; k++) cnt[p++] = lbl[k];
        cnt[p] = 0;
        DrawString(wx + 4, sb_y + 1, cnt, 0x07);
    }

    /* Right: selected filename if any */
    if (s_selected >= 0 && s_selected < (int)s_count) {
        char sel[EVRYFS_NAME_LEN + 2];
        FilesStrCpy(sel, entries[s_selected].FileName, EVRYFS_NAME_LEN);
        int slen = FilesStrLen(sel);
        int sx   = wx + ww - (slen * 8) - 4;
        if (sx < wx + ww / 2) sx = wx + ww / 2;
        DrawString(sx, sb_y + 1, sel, 0x0F);
    }
}
