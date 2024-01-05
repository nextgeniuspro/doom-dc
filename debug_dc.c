/*
 * Licensed under the BSD license
 *
 * debug_32x.c - Debug screen functions.
 *
 * Copyright (c) 2005 Marcus R. Brown <mrbrown@ocgnet.org>
 * Copyright (c) 2005 James Forshaw <tyranid@gmail.com>
 * Copyright (c) 2005 John Kelley <ps2dev@kelley.ca>
 *
 * Altered for DC by Chilly Willy
 */

#include <kos.h>
#include <dc/video.h>
#include <dc/biosfont.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static int X = 0, Y = 0;
static int MX = 53, MY = 20;
static int SW = 640, SH = 480;
static int vmode = DM_640x480, pmode = PM_RGB565;
static unsigned int fgc = 0xFFFFFF, bgc = 0x000000;

/* Defines to convert colors */
#define MAKE_RGB555(c) (((c & 0xF80000)>>9)|((c & 0x00F800)>>6)|((c & 0x0000F8)>>3))
#define MAKE_RGB565(c) (((c & 0xF80000)>>8)|((c & 0x00FC00)>>5)|((c & 0x0000F8)>>3))


// pass a generic mode and the pixel mode - it doesn't set the video mode,
//   it merely assumes the values describe the mode already set
void DebugDC_Init(int vidmode, int pixmode)
{
    if (vmode == vidmode && pmode == pixmode)
        return;

    if (vidmode == -1)
    {
        vmode = pmode = -1;
        return;
    }

    switch(vidmode)
    {
        case DM_320x240:
        SW = 320;
        SH = 240;
        break;
        case DM_640x480:
        SW = 640;
        SH = 480;
        break;
        case DM_256x256:
        SW = 256;
        SH = 256;
        break;
        case DM_768x480:
        SW = 768;
        SH = 480;
        break;
        case DM_768x576:
        SW = 768;
        SH = 576;
        break;
    }
    MX = SW / 12;
    MY = SH / 24;

    fgc = 0xFFFFFF;
    bgc = 0x000000;
    X = Y = 0;

    vmode = vidmode;
    pmode = pixmode;
}

void DebugDC_SetFGColor(int r, int g, int b)
{
    if (vmode == -1)
        return;

    fgc = 0xFF<<24 | r<<16 | g<<8 | b;
    if (pmode == PM_RGB888)
        bfont_set_foreground_color(fgc);
    else if (pmode == PM_RGB565)
        bfont_set_foreground_color(MAKE_RGB565(fgc));
    else
        bfont_set_foreground_color(MAKE_RGB555(fgc));
}

void DebugDC_SetBGColor(int r, int g, int b)
{
    if (vmode == -1)
        return;

    bgc = 0xFF<<24 | r<<16 | g<<8 | b;
    if (pmode == PM_RGB888)
        bfont_set_background_color(bgc);
    else if (pmode == PM_RGB565)
        bfont_set_background_color(MAKE_RGB565(bgc));
    else
        bfont_set_background_color(MAKE_RGB555(bgc));
}

int DebugDC_ScreenGetX()
{
    return X;
}

int DebugDC_ScreenGetY()
{
    return Y;
}

void DebugDC_ScreenClear()
{
    if (vmode == -1)
        return;

    vid_clear(bgc>>16, (bgc>>8) & 0xFF, bgc & 0xFF);
}

void DebugDC_ScreenSetXY(int x, int y)
{
    if(x<MX && x>=0) X = x;
    if(y<MY && y>=0) Y = y;
}

static void debug_put_char_16(int x, int y, unsigned char ch)
{
    if (vmode == -1)
        return;

    bfont_draw_thin((uint16 *)((uint32)vram_s + (x * 12 + y * 24 * SW) * 2), SW, 1, ch, 0);
}

static void debug_put_char_32(int x, int y, unsigned char ch)
{
    if (vmode == -1)
        return;

    bfont_draw_thin((uint16 *)((uint32)vram_l + (x * 12 + y * 24 * SW) * 4), SW, 1, ch, 0);
}

void DebugDC_ScreenPutChar(int x, int y, unsigned char ch)
{
    if (pmode != PM_RGB888)
        debug_put_char_16(x, y, ch);
    else
        debug_put_char_32(x, y, ch);
}

void DebugDC_ScreenClearLine(int Y)
{
    int i;

    for (i=0; i < MX; i++)
        DebugDC_ScreenPutChar(i, Y, ' ');
}

/* Print non-nul terminated strings */
int DebugDC_ScreenPrintData(const char *buff, int size)
{
    int i;
    int j;
    char c;

    for (i=0; i<size; i++)
    {
        c = buff[i];
        switch (c)
        {
            case '\r':
                X = 0;
                break;
            case '\n':
                X = 0;
                Y++;
                if (Y == MY)
                Y = 0;
                DebugDC_ScreenClearLine(Y);
                break;
            case '\t':
                for (j=0; j<4; j++)
                {
                    DebugDC_ScreenPutChar(X, Y, ' ');
                            X++;
                }
                break;
            default:
                DebugDC_ScreenPutChar(X, Y, c);
                X++;
                if (X == MX)
                {
                    X = 0;
                    Y++;
                    if (Y == MY)
                        Y = 0;
                    DebugDC_ScreenClearLine(Y);
                }
        }
    }

    return i;
}

int DebugDC_ScreenPuts(const char *str)
{
    return DebugDC_ScreenPrintData(str, strlen(str));
}

void DebugDC_Delay(int ticks)
{
    thd_sleep(ticks*20);
}
