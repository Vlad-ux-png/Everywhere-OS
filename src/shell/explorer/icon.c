/*++

Copyright (c) 2026  Everywhere Computing, Inc. All Rights Reserved.

Module Name:

    icon.c

Abstract:

    Folder icon loader and renderer.

Author:

    Noah Juopperi <nipfswd@gmail.com>
    Vlad Lymar <ggcc98765432110@gmail.com>

Environment:

    Kernel-mode

--*/

#include "explorer.h"

//## Constants ##

#define FOLDER_ICO_FS_NAME  "Everywhere\\Res\\F\\folder.ico"

#define FOLDER_ICO_MAXSIZE  65536

#define ICON_W  16
#define ICON_H  16

//## Module private state ##

static uint8_t s_ico_buf[FOLDER_ICO_MAXSIZE];

static uint8_t s_folder_pixels[ICON_H][ICON_W];

static uint8_t s_folder_alpha[ICON_H][ICON_W];

static int s_icon_loaded = 0;

static uint8_t s_vga_pal[256 * 3];

//## Internal Helpers ##

/*++

Routine Description:

    Reads the current VGA DAC (Digital-to-Analog Converter) palette into
    s_vga_pal.

--*/
static void IcoReadVgaPalette(void)
{
    outb(0x3C7, 0);
    for (int i = 0; i < 256 * 3; i++) {
        s_vga_pal[i] = inb(0x3C9);
    }
}

/*++

Routine Description:

    Returns the VGA palette index whose (R,G,B) value is closest to the
    supplied (r,g,b) tuple (all in the 0-255 range) by minimum squared
    Euclidean distance.

--*/
static uint8_t IcoNearestVga(uint8_t r, uint8_t g, uint8_t b)
{
    uint32_t best_dist = 0xFFFFFFFFU;
    uint8_t  best_idx  = 0;

    for (int i = 0; i < 256; i++) {
        int dr = (int)r - (int)(s_vga_pal[i * 3    ] * 4);
        int dg = (int)g - (int)(s_vga_pal[i * 3 + 1] * 4);
        int db = (int)b - (int)(s_vga_pal[i * 3 + 2] * 4);
        uint32_t dist = (uint32_t)(dr * dr + dg * dg + db * db);
        if (dist < best_dist) {
            best_dist = dist;
            best_idx  = (uint8_t)i;
            if (dist == 0) break;
        }
    }

    return best_idx;
}

static uint16_t IcoU16(const uint8_t* b, int off)
{
    return (uint16_t)(b[off] | ((uint16_t)b[off + 1] << 8));
}

static uint32_t IcoU32(const uint8_t* b, int off)
{
    return (uint32_t)b[off]
         | ((uint32_t)b[off + 1] <<  8)
         | ((uint32_t)b[off + 2] << 16)
         | ((uint32_t)b[off + 3] << 24);
}

//## Public API ##

/*++

Routine Description:

    Loads C:\Everywhere\Res\F\folder.ico from the EVRYFS volume, decodes
    its 16x16 (or nearest available) 32-bit RGBA image into s_folder_pixels
    and s_folder_alpha, and reads the live VGA DAC palette so that pixel
    colours can be mapped accurately.

    Must be called after EvryFsInit().  Safe to call multiple times
    (re-loads the icon each time).

Arguments:

    None.

Return Value:

    None.

--*/
void IconInit(void)
{
    s_icon_loaded = 0;

    int len = EvryFsReadFile(FOLDER_ICO_FS_NAME, s_ico_buf, FOLDER_ICO_MAXSIZE);
    if (len < 22) return;               // too short

    if (IcoU16(s_ico_buf, 2) != 1) return;//NOT TYPE_ICON
    int count = (int)IcoU16(s_ico_buf, 4);
    if (count <= 0 || 6 + count * 16 > len) return;

    int best    = -1;
    int best_w  = 0x7FFF;

    for (int i = 0; i < count; i++) {
        int eoff = 6 + i * 16;
        int w    = (int)s_ico_buf[eoff];//0=256
        if (w == 0) w = 256;
        int bc   = (int)IcoU16(s_ico_buf, eoff + 6);
        uint32_t img_off = IcoU32(s_ico_buf, eoff + 12);
        uint32_t img_sz  = IcoU32(s_ico_buf, eoff + 8);

        // This is a bit of a "paranoid" check but I like to be reassured.
        // image data MUST, yep ,MUST be within our buffer.
        if ((int)img_off + 40 > len) continue;
        if ((int)(img_off + img_sz) > len) continue;

        if (bc == 32 && w >= ICON_W && w <= best_w) {
            best_w = w;
            best   = i;
        }
    }

    // Mandatory fallback
    if (best < 0) {
        for (int i = 0; i < count; i++) {
            int eoff = 6 + i * 16;
            int bc   = (int)IcoU16(s_ico_buf, eoff + 6);
            uint32_t img_off = IcoU32(s_ico_buf, eoff + 12);
            if ((int)img_off + 40 <= len && bc == 32) { best = i; break; }
        }
    }

    if (best < 0) return;

    int      eoff   = 6 + best * 16;
    uint32_t img_off = IcoU32(s_ico_buf, eoff + 12);

    const uint8_t* bmp = s_ico_buf + img_off;

    int bmp_w   = (int)IcoU32(bmp, 4);
    int bmp_dh  = (int)IcoU32(bmp, 8);
    int bmp_bpp = (int)IcoU16(bmp, 14);

    if (bmp_bpp != 32) return;
    if (bmp_w < 1 || bmp_dh < 2) return;

    int bmp_h = bmp_dh / 2;

    int stride  = bmp_w * 4;

    const uint8_t* pixels = bmp + 40;

    // Another paranoid check :)
    if ((int)img_off + 40 + bmp_h * stride > len) return;

    int draw_w = bmp_w  < ICON_W ? bmp_w  : ICON_W;
    int draw_h = bmp_h  < ICON_H ? bmp_h  : ICON_H;

    // Let's get ready to rumble!
    IcoReadVgaPalette();

    for (int row = 0; row < ICON_H; row++) {
        if (row < draw_h) {
            int rb         = bmp_h - 1 - row;
            const uint8_t* src = pixels + rb * stride;

            for (int col = 0; col < ICON_W; col++) {
                if (col < draw_w) {
                    uint8_t b_ch = src[col * 4 + 0];
                    uint8_t g_ch = src[col * 4 + 1];
                    uint8_t r_ch = src[col * 4 + 2];
                    uint8_t a_ch = src[col * 4 + 3];

                    if (a_ch >= 128) {
                        s_folder_pixels[row][col] = IcoNearestVga(r_ch, g_ch, b_ch);
                        s_folder_alpha [row][col] = 1;
                    } else {
                        s_folder_pixels[row][col] = 0;
                        s_folder_alpha [row][col] = 0;
                    }
                } else {
                    s_folder_pixels[row][col] = 0;
                    s_folder_alpha [row][col] = 0;
                }
            }
        } else {
            for (int col = 0; col < ICON_W; col++) {
                s_folder_pixels[row][col] = 0;
                s_folder_alpha [row][col] = 0;
            }
        }
    }

    s_icon_loaded = 1;
}

/*++

Routine Description:

    Blits the decoded 16x16 folder icon to the back buffer at the given
    screen coordinate.  Transparent pixels (alpha = 0) are skipped so the
    window background shows through.  Aint do shit if IconInit() has not
    succeeded.

Arguments:

    x - Left edge of the icon in screen pixels.
    y - Top edge of the icon in screen pixels.

    (Pretty self-explanatory)

Return Value:

    None.

--*/
void IconDrawFolder(int x, int y)
{
    if (!s_icon_loaded) return;

    for (int row = 0; row < ICON_H; row++) {
        for (int col = 0; col < ICON_W; col++) {
            if (s_folder_alpha[row][col]) {
                PutPixel(x + col, y + row, s_folder_pixels[row][col]);
            }
        }
    }
}
