#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <kos.h>
#include <dc/sq.h>
#include <dc/video.h>
#include "debug_dc.h"

#include "doomtype.h"
#include "doomdef.h"
#include "doomstat.h"
#include "i_system.h"
#include "i_video.h"
#include "v_video.h"
#include "m_argv.h"
#include "m_bbox.h"
#include "d_main.h"
#include "r_draw.h"
#include "w_wad.h"
#include "z_zone.h"

#define QACR0 (*(volatile uint32_t *)(void *)0xff000038)

/** \brief   Store Queue 1 access register 
    \ingroup store_queues  
*/
#define QACR1 (*(volatile uint32_t *)(void *)0xff00003c)

/**********************************************************************/

int SCREENWIDTH;
int SCREENHEIGHT;
int weirdaspect;

#define NUMPALETTES 14

static uint32 video_colourtable[NUMPALETTES][256];
static int video_palette_index = 0;
static int video_palette_changed = 0;

static int video_doing_fps = 0;
static int video_cable = CT_COMPOSITE;
static int video_is_tv = 1;
static int video_res = 0;
static int video_vsync = 0;

static int lineBytes, lineWidth;

static int vramoffset;

unsigned int total_frames = 0;

#ifdef PROFILE
unsigned int profile[32][4];
static void calc_time (uint32 time, char *msg);
#endif

extern int guiVMode;
extern int guiVMode32;

/**********************************************************************/
static void video_do_fps (uint32 *base, int yoffset)
{
    static unsigned int last_frames = 0;
    static int secs = 0;
    static int ifps = 0;
    char fps[8];
    int curr_ticks = I_GetTime();

    if (curr_ticks / 35 != secs)
    {
        ifps = (int)(total_frames - last_frames);
        last_frames = total_frames;
        secs = curr_ticks / 35;
    }

    sprintf(fps, "%d", ifps);
    bfont_draw_str((uint16 *)((uint32)vram_l + 48 + 24 * lineBytes), lineWidth, 0, fps);
}

/**********************************************************************/
void video_set_vmode(void)
{
    int disp_mode = DM_320x240;
    if (!video_is_tv)
    {
        switch(video_res)
        {
            case 0:
            disp_mode = DM_320x240;
            break;
            case 1:
            disp_mode = DM_640x480;
            break;
            case 2:
            disp_mode = DM_768x480;
            break;
        }
    }
    else
    {
        switch(video_res)
        {
            case 0:
            disp_mode = (guiVMode == DM_640x480_PAL_IL) ? DM_320x240_PAL : DM_320x240_NTSC;
            break;
            case 1:
            disp_mode = (guiVMode == DM_640x480_PAL_IL) ? DM_640x480_PAL_IL : DM_640x480_NTSC_IL;
            break;
            case 2:
            disp_mode = (guiVMode == DM_640x480_PAL_IL) ? DM_768x480_PAL_IL : DM_768x480_NTSC_IL;
            break;
        }
    }
    vid_set_mode(disp_mode | DM_MULTIBUFFER, PM_RGB888);
    vid_empty();
    bfont_set_32bit_mode(1);
    bfont_set_foreground_color(0xFFFFFFFF);
    guiVMode32 = 1;
    DebugDC_Init(-1, -1); // disable debug output to screen
}

/**********************************************************************/
// Called by D_DoomMain,
// determines the hardware configuration
// and sets up the video mode
void I_InitGraphics (void)
{
    int p;

    video_cable = vid_check_cable();
    video_is_tv = M_CheckParm ("-tv");
    if (video_is_tv && video_cable == CT_VGA)
    {
        printf("Cable doesn't match TV/VGA setting!");
        video_is_tv = 0;
    }
    else if (!video_is_tv && (video_cable == CT_COMPOSITE || video_cable == CT_RGB))
    {
        printf("Cable doesn't match TV/VGA setting!");
        video_is_tv = 1;
    }

    video_doing_fps = M_CheckParm ("-fps");
    video_vsync = M_CheckParm ("-vsync");

    switch(SCREENWIDTH)
    {
        case 304:
        case 320:
        lineWidth = 320;
        video_res = 0;
        break;
        case 608:
        case 640:
        lineWidth = 640;
        video_res = 1;
        break;
        case 576:
        lineWidth = 768;
        video_res = 2;
        break;
        case 800:
        lineWidth = 800;
        video_res = 2;
        break;
    }
    lineBytes = lineWidth * 4;

    int i = 0, j = 0;
    p = M_CheckParm ("-tvcx");
    if (p)
        sscanf(myargv[p+1], "%d", &i);
    p = M_CheckParm ("-tvcy");
    if (p)
        sscanf(myargv[p+1], "%d", &j);

    vramoffset = video_is_tv ? i*4 + lineBytes * j : 0;

    I_RecalcPalettes();

    video_set_vmode();
}

/**********************************************************************/
void I_ShutdownGraphics (void)
{
    printf (" I_ShutdownGraphics: ");

    if (total_frames > 0) {
        printf("Total number of frames = %u\n", total_frames);
#ifdef PROFILE
        {
            int i;

            for (i=0; i<32; i++)
                calc_time (profile[n][2], "Profile Time ");
        }
#endif
    }

    printf (" Graphics module closed.\n");
}

/**********************************************************************/
// recalculate colourtable[][] after changing usegamma
void I_RecalcPalettes (void)
{
  int p, i;
  byte *palette;
  static int lu_palette;

  lu_palette = W_GetNumForName ("PLAYPAL");
  for (p = 0; p < NUMPALETTES; p++) {
    palette = (byte *) W_CacheLumpNum (lu_palette, PU_STATIC)+p*768;
    for (i=0; i<256; i++) {
        // Better to define c locally here instead of for the whole function:
        uint32 r = gammatable[usegamma][palette[i*3]];
        uint32 g = gammatable[usegamma][palette[i*3+1]];
        uint32 b = gammatable[usegamma][palette[i*3+2]];
        video_colourtable[p][i] = r<<16 | g<<8 | b;
    }
  }
}

/**********************************************************************/
// Takes full 8 bit values.
void I_SetPalette (byte *palette, int palette_index)
{
  video_palette_changed = 1;
  video_palette_index = palette_index;
}

/**********************************************************************/
// Called by anything that renders to screens[0] (except 3D view)
void I_MarkRect (int left, int top, int width, int height)
{
  M_AddToBox (dirtybox, left, top);
  M_AddToBox (dirtybox, left + width - 1, top + height - 1);
}

/**********************************************************************/
void I_StartUpdate (void)
{
    thd_pass();
}

/**********************************************************************/
void I_UpdateNoBlit (void)
{
}

/**********************************************************************/
void I_FinishUpdate (void)
{
    int top, left, width, height;
    int i, j;
    uint32 *base_address;
    static uint32 *palette = video_colourtable[0];

    if (total_frames == 0)
        vid_empty();
    total_frames++;

    if (video_palette_changed != 0) {
      palette = video_colourtable[video_palette_index];
      video_palette_changed = 0;
    }

#if 0
    /* update only the viewwindow and dirtybox when gamestate == GS_LEVEL */
    if (gamestate == GS_LEVEL) {
        if (dirtybox[BOXLEFT] < viewwindowx)
            left = dirtybox[BOXLEFT];
        else
            left = viewwindowx;
        if (dirtybox[BOXRIGHT] + 1 > viewwindowx + scaledviewwidth)
            width = dirtybox[BOXRIGHT] + 1 - left;
        else
            width = viewwindowx + scaledviewwidth - left;
        if (dirtybox[BOXBOTTOM] < viewwindowy) /* BOXBOTTOM is really the top! */
            top = dirtybox[BOXBOTTOM];
        else
            top = viewwindowy;
        if (dirtybox[BOXTOP] + 1 > viewwindowy + viewheight)
            height = dirtybox[BOXTOP] + 1 - top;
        else
            height = viewwindowy + viewheight - top;
        M_ClearBox (dirtybox);
#ifdef RANGECHECK
        if (left < 0 || left + width > SCREENWIDTH || top < 0 || top + height > SCREENHEIGHT)
            I_Error ("I_FinishUpdate: Box out of range: %d %d %d %d", left, top, width, height);
#endif
    } else {
        left = 0;
        top = 0;
        width = SCREENWIDTH;
        height = SCREENHEIGHT;
    }
#else
    left = 0;
    top = 0;
    width = SCREENWIDTH;
    height = SCREENHEIGHT;
#endif

    base_address = (uint32 *)((uint32)vram_l + vramoffset);

    //start_timer ();
#if 0
    for (j=top; j<height; j++)
        for (i=left; i<width; i++)
            base_address[i + j*lineWidth] = palette[screens[0][i + j*SCREENWIDTH]];
#elif 0
    for (j=top; j<height; j++)
        for (i=left; i<width; i+=4)
        {
            uint32 fp = *(uint32 *)&screens[0][i + j*SCREENWIDTH];
            base_address[i + j*lineWidth] = palette[fp&0xff];
            base_address[i + 1 + j*lineWidth] = palette[(fp>>8)&0xff];
            base_address[i + 2 + j*lineWidth] = palette[(fp>>16)&0xff];
            base_address[i + 3 + j*lineWidth] = palette[fp>>24];
        }
#else
    {
        uint32 dest = (uint32)&base_address[0];
        /* Set store queue memory area as desired */
        QACR0 = (((dest)>>26)<<2)&0x1c;
        QACR1 = (((dest)>>26)<<2)&0x1c;
    }
    for (j=top; j<height; j++)
    {
        uint32 *s = (uint32 *)&screens[0][j*SCREENWIDTH];
        uint32 *d = (uint32 *)(0xe0000000 | (((uint32)&base_address[j*lineWidth]) & 0x03ffffe0));
        for (i=left; i<width; i+=8)
        {
            // copy to vram using store queues
            uint32 fp;
// screens[] is uncached, so prefetch doesn't make a difference
//            if ((i&7) == 0)
//                asm("pref @%0" : : "r" (s + 8)); /* prefetch 32 bytes for next loop */
            fp = *s++;
            d[0] = palette[fp&0xff];
            d[1] = palette[(fp>>8)&0xff];
            d[2] = palette[(fp>>16)&0xff];
            d[3] = palette[fp>>24];
            fp = *s++;
            d[4] = palette[fp&0xff];
            d[5] = palette[(fp>>8)&0xff];
            d[6] = palette[(fp>>16)&0xff];
            d[7] = palette[fp>>24];
            asm("pref @%0" : : "r" (d));
            d += 8;
        }
    }
    {
        /* Wait for both store queues to complete */
        uint32 *d = (uint32 *)0xe0000000;
        d[0] = d[8] = 0;
    }
#endif
    //lock_time += end_timer ();

    thd_pass();

    if (video_doing_fps)
      video_do_fps (base_address, 0);

    if (video_vsync)
        vid_waitvbl();

    vid_flip(-1);
}

/**********************************************************************/
// Wait for vertical retrace or pause a bit.  Use when quit game.
void I_WaitVBL(int count)
{
  for ( ; count > 0; count--)
    vid_waitvbl();
}

/**********************************************************************/
void I_ReadScreen (byte* scr)
{
  memcpy (scr, screens[0], SCREENWIDTH * SCREENHEIGHT);
}

/**********************************************************************/
void I_BeginRead (void)
{
}

/**********************************************************************/
void I_EndRead (void)
{
}

/**********************************************************************/
#ifdef PROFILE
static void calc_time (uint32 time, char *msg)
{
    printf ("Total %s = %ld us  (%ld us/frame)\n", msg, time, time / total_frames);
}
#endif

/**********************************************************************/
