#include <kos.h>
#include "debug_dc.h"
#include "dc_vmu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#include "doomdef.h"
#include "doomstat.h"
#include "m_misc.h"
#include "i_system.h"
#include "i_video.h"
#include "i_sound.h"
#include "d_items.h"
#include "d_main.h"
#include "d_net.h"
#include "d_player.h"
#include "g_game.h"
#include "m_argv.h"

#include <dc/maple.h>
#include <dc/maple/controller.h>
#include <dc/maple/purupuru.h>
#include <dc/maple/mouse.h>
#include <dc/maple/keyboard.h>

#include <arch/timer.h>

//typedef unsigned char      uint8_t;
//typedef signed   char      sint8_t;
//typedef unsigned short     uint16_t;
//typedef signed   short     sint16_t;
//typedef unsigned int       uint32_t;
typedef signed   int       sint32_t;

extern int guiVMode;
extern byte *vid_mem;
extern unsigned int total_frames;

int quit_requested = 0;

int dc_cheat_select = 0;


#define MIN_ZONESIZE  (2*1024*1024)
#define MAX_ZONESIZE  (6*1024*1024)

static void dc_handle_ff(void);
static void dc_getevents (void);
static void update_vmu_stat(void);

static int stick_disabled = 0;
static int stick_cx = 128;
static int stick_cy = 128;
static int stick_minx = 0;
static int stick_miny = 0;
static int stick_maxx = 255;
static int stick_maxy = 255;
static int ctrl_cheat1= 0;
static int ctrl_cheat2= 0;
static int ctrl_cheat3= 0;

static int ff_enable = 1;
static int ff_timeout = 0;
static int ff_frequency = 0;
static int ff_intensity = 0;
static int ff_select = 0x10;

static maple_device_t *cont, *kbd, *mky;
//static maple_device_t * jump;
static cont_state_t *cstate;
static kbd_state_t *kstate;
static mouse_state_t *mstate;

static uint8 bitmap[48*32/8];

extern int guiVMode32;

/* Defines to convert colors */
#define MAKE_RGB555(c) (((c & 0xF80000)>>9)|((c & 0x00F800)>>6)|((c & 0x0000F8)>>3))
#define MAKE_RGB565(c) (((c & 0xF80000)>>8)|((c & 0x00FC00)>>5)|((c & 0x0000F8)>>3))

/**********************************************************************/
/*
 * Licensed under the BSD license
 *
 * MSX Font
 *
 * Copyright (c) 2005 Marcus R. Brown <mrbrown@ocgnet.org>
 * Copyright (c) 2005 James Forshaw <tyranid@gmail.com>
 * Copyright (c) 2005 John Kelley <ps2dev@kelley.ca>
 */
static unsigned char msx[]=
"\x00\x00\x00\x00\x00\x00\x00\x00\x3c\x42\xa5\x81\xa5\x99\x42\x3c"
"\x3c\x7e\xdb\xff\xff\xdb\x66\x3c\x6c\xfe\xfe\xfe\x7c\x38\x10\x00"
"\x10\x38\x7c\xfe\x7c\x38\x10\x00\x10\x38\x54\xfe\x54\x10\x38\x00"
"\x10\x38\x7c\xfe\xfe\x10\x38\x00\x00\x00\x00\x30\x30\x00\x00\x00"
"\xff\xff\xff\xe7\xe7\xff\xff\xff\x38\x44\x82\x82\x82\x44\x38\x00"
"\xc7\xbb\x7d\x7d\x7d\xbb\xc7\xff\x0f\x03\x05\x79\x88\x88\x88\x70"
"\x38\x44\x44\x44\x38\x10\x7c\x10\x30\x28\x24\x24\x28\x20\xe0\xc0"
"\x3c\x24\x3c\x24\x24\xe4\xdc\x18\x10\x54\x38\xee\x38\x54\x10\x00"
"\x10\x10\x10\x7c\x10\x10\x10\x10\x10\x10\x10\xff\x00\x00\x00\x00"
"\x00\x00\x00\xff\x10\x10\x10\x10\x10\x10\x10\xf0\x10\x10\x10\x10"
"\x10\x10\x10\x1f\x10\x10\x10\x10\x10\x10\x10\xff\x10\x10\x10\x10"
"\x10\x10\x10\x10\x10\x10\x10\x10\x00\x00\x00\xff\x00\x00\x00\x00"
"\x00\x00\x00\x1f\x10\x10\x10\x10\x00\x00\x00\xf0\x10\x10\x10\x10"
"\x10\x10\x10\x1f\x00\x00\x00\x00\x10\x10\x10\xf0\x00\x00\x00\x00"
"\x81\x42\x24\x18\x18\x24\x42\x81\x01\x02\x04\x08\x10\x20\x40\x80"
"\x80\x40\x20\x10\x08\x04\x02\x01\x00\x10\x10\xff\x10\x10\x00\x00"
"\x00\x00\x00\x00\x00\x00\x00\x00\x20\x20\x20\x20\x00\x00\x20\x00"
"\x50\x50\x50\x00\x00\x00\x00\x00\x50\x50\xf8\x50\xf8\x50\x50\x00"
"\x20\x78\xa0\x70\x28\xf0\x20\x00\xc0\xc8\x10\x20\x40\x98\x18\x00"
"\x40\xa0\x40\xa8\x90\x98\x60\x00\x10\x20\x40\x00\x00\x00\x00\x00"
"\x10\x20\x40\x40\x40\x20\x10\x00\x40\x20\x10\x10\x10\x20\x40\x00"
"\x20\xa8\x70\x20\x70\xa8\x20\x00\x00\x20\x20\xf8\x20\x20\x00\x00"
"\x00\x00\x00\x00\x00\x20\x20\x40\x00\x00\x00\x78\x00\x00\x00\x00"
"\x00\x00\x00\x00\x00\x60\x60\x00\x00\x00\x08\x10\x20\x40\x80\x00"
"\x70\x88\x98\xa8\xc8\x88\x70\x00\x20\x60\xa0\x20\x20\x20\xf8\x00"
"\x70\x88\x08\x10\x60\x80\xf8\x00\x70\x88\x08\x30\x08\x88\x70\x00"
"\x10\x30\x50\x90\xf8\x10\x10\x00\xf8\x80\xe0\x10\x08\x10\xe0\x00"
"\x30\x40\x80\xf0\x88\x88\x70\x00\xf8\x88\x10\x20\x20\x20\x20\x00"
"\x70\x88\x88\x70\x88\x88\x70\x00\x70\x88\x88\x78\x08\x10\x60\x00"
"\x00\x00\x20\x00\x00\x20\x00\x00\x00\x00\x20\x00\x00\x20\x20\x40"
"\x18\x30\x60\xc0\x60\x30\x18\x00\x00\x00\xf8\x00\xf8\x00\x00\x00"
"\xc0\x60\x30\x18\x30\x60\xc0\x00\x70\x88\x08\x10\x20\x00\x20\x00"
"\x70\x88\x08\x68\xa8\xa8\x70\x00\x20\x50\x88\x88\xf8\x88\x88\x00"
"\xf0\x48\x48\x70\x48\x48\xf0\x00\x30\x48\x80\x80\x80\x48\x30\x00"
"\xe0\x50\x48\x48\x48\x50\xe0\x00\xf8\x80\x80\xf0\x80\x80\xf8\x00"
"\xf8\x80\x80\xf0\x80\x80\x80\x00\x70\x88\x80\xb8\x88\x88\x70\x00"
"\x88\x88\x88\xf8\x88\x88\x88\x00\x70\x20\x20\x20\x20\x20\x70\x00"
"\x38\x10\x10\x10\x90\x90\x60\x00\x88\x90\xa0\xc0\xa0\x90\x88\x00"
"\x80\x80\x80\x80\x80\x80\xf8\x00\x88\xd8\xa8\xa8\x88\x88\x88\x00"
"\x88\xc8\xc8\xa8\x98\x98\x88\x00\x70\x88\x88\x88\x88\x88\x70\x00"
"\xf0\x88\x88\xf0\x80\x80\x80\x00\x70\x88\x88\x88\xa8\x90\x68\x00"
"\xf0\x88\x88\xf0\xa0\x90\x88\x00\x70\x88\x80\x70\x08\x88\x70\x00"
"\xf8\x20\x20\x20\x20\x20\x20\x00\x88\x88\x88\x88\x88\x88\x70\x00"
"\x88\x88\x88\x88\x50\x50\x20\x00\x88\x88\x88\xa8\xa8\xd8\x88\x00"
"\x88\x88\x50\x20\x50\x88\x88\x00\x88\x88\x88\x70\x20\x20\x20\x00"
"\xf8\x08\x10\x20\x40\x80\xf8\x00\x70\x40\x40\x40\x40\x40\x70\x00"
"\x00\x00\x80\x40\x20\x10\x08\x00\x70\x10\x10\x10\x10\x10\x70\x00"
"\x20\x50\x88\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xf8\x00"
"\x40\x20\x10\x00\x00\x00\x00\x00\x00\x00\x70\x08\x78\x88\x78\x00"
"\x80\x80\xb0\xc8\x88\xc8\xb0\x00\x00\x00\x70\x88\x80\x88\x70\x00"
"\x08\x08\x68\x98\x88\x98\x68\x00\x00\x00\x70\x88\xf8\x80\x70\x00"
"\x10\x28\x20\xf8\x20\x20\x20\x00\x00\x00\x68\x98\x98\x68\x08\x70"
"\x80\x80\xf0\x88\x88\x88\x88\x00\x20\x00\x60\x20\x20\x20\x70\x00"
"\x10\x00\x30\x10\x10\x10\x90\x60\x40\x40\x48\x50\x60\x50\x48\x00"
"\x60\x20\x20\x20\x20\x20\x70\x00\x00\x00\xd0\xa8\xa8\xa8\xa8\x00"
"\x00\x00\xb0\xc8\x88\x88\x88\x00\x00\x00\x70\x88\x88\x88\x70\x00"
"\x00\x00\xb0\xc8\xc8\xb0\x80\x80\x00\x00\x68\x98\x98\x68\x08\x08"
"\x00\x00\xb0\xc8\x80\x80\x80\x00\x00\x00\x78\x80\xf0\x08\xf0\x00"
"\x40\x40\xf0\x40\x40\x48\x30\x00\x00\x00\x90\x90\x90\x90\x68\x00"
"\x00\x00\x88\x88\x88\x50\x20\x00\x00\x00\x88\xa8\xa8\xa8\x50\x00"
"\x00\x00\x88\x50\x20\x50\x88\x00\x00\x00\x88\x88\x98\x68\x08\x70"
"\x00\x00\xf8\x10\x20\x40\xf8\x00\x18\x20\x20\x40\x20\x20\x18\x00"
"\x20\x20\x20\x00\x20\x20\x20\x00\xc0\x20\x20\x10\x20\x20\xc0\x00"
"\x40\xa8\x10\x00\x00\x00\x00\x00\x00\x00\x20\x50\xf8\x00\x00\x00"
"\x70\x88\x80\x80\x88\x70\x20\x60\x90\x00\x00\x90\x90\x90\x68\x00"
"\x10\x20\x70\x88\xf8\x80\x70\x00\x20\x50\x70\x08\x78\x88\x78\x00"
"\x48\x00\x70\x08\x78\x88\x78\x00\x20\x10\x70\x08\x78\x88\x78\x00"
"\x20\x00\x70\x08\x78\x88\x78\x00\x00\x70\x80\x80\x80\x70\x10\x60"
"\x20\x50\x70\x88\xf8\x80\x70\x00\x50\x00\x70\x88\xf8\x80\x70\x00"
"\x20\x10\x70\x88\xf8\x80\x70\x00\x50\x00\x00\x60\x20\x20\x70\x00"
"\x20\x50\x00\x60\x20\x20\x70\x00\x40\x20\x00\x60\x20\x20\x70\x00"
"\x50\x00\x20\x50\x88\xf8\x88\x00\x20\x00\x20\x50\x88\xf8\x88\x00"
"\x10\x20\xf8\x80\xf0\x80\xf8\x00\x00\x00\x6c\x12\x7e\x90\x6e\x00"
"\x3e\x50\x90\x9c\xf0\x90\x9e\x00\x60\x90\x00\x60\x90\x90\x60\x00"
"\x90\x00\x00\x60\x90\x90\x60\x00\x40\x20\x00\x60\x90\x90\x60\x00"
"\x40\xa0\x00\xa0\xa0\xa0\x50\x00\x40\x20\x00\xa0\xa0\xa0\x50\x00"
"\x90\x00\x90\x90\xb0\x50\x10\xe0\x50\x00\x70\x88\x88\x88\x70\x00"
"\x50\x00\x88\x88\x88\x88\x70\x00\x20\x20\x78\x80\x80\x78\x20\x20"
"\x18\x24\x20\xf8\x20\xe2\x5c\x00\x88\x50\x20\xf8\x20\xf8\x20\x00"
"\xc0\xa0\xa0\xc8\x9c\x88\x88\x8c\x18\x20\x20\xf8\x20\x20\x20\x40"
"\x10\x20\x70\x08\x78\x88\x78\x00\x10\x20\x00\x60\x20\x20\x70\x00"
"\x20\x40\x00\x60\x90\x90\x60\x00\x20\x40\x00\x90\x90\x90\x68\x00"
"\x50\xa0\x00\xa0\xd0\x90\x90\x00\x28\x50\x00\xc8\xa8\x98\x88\x00"
"\x00\x70\x08\x78\x88\x78\x00\xf8\x00\x60\x90\x90\x90\x60\x00\xf0"
"\x20\x00\x20\x40\x80\x88\x70\x00\x00\x00\x00\xf8\x80\x80\x00\x00"
"\x00\x00\x00\xf8\x08\x08\x00\x00\x84\x88\x90\xa8\x54\x84\x08\x1c"
"\x84\x88\x90\xa8\x58\xa8\x3c\x08\x20\x00\x00\x20\x20\x20\x20\x00"
"\x00\x00\x24\x48\x90\x48\x24\x00\x00\x00\x90\x48\x24\x48\x90\x00"
"\x28\x50\x20\x50\x88\xf8\x88\x00\x28\x50\x70\x08\x78\x88\x78\x00"
"\x28\x50\x00\x70\x20\x20\x70\x00\x28\x50\x00\x20\x20\x20\x70\x00"
"\x28\x50\x00\x70\x88\x88\x70\x00\x50\xa0\x00\x60\x90\x90\x60\x00"
"\x28\x50\x00\x88\x88\x88\x70\x00\x50\xa0\x00\xa0\xa0\xa0\x50\x00"
"\xfc\x48\x48\x48\xe8\x08\x50\x20\x00\x50\x00\x50\x50\x50\x10\x20"
"\xc0\x44\xc8\x54\xec\x54\x9e\x04\x10\xa8\x40\x00\x00\x00\x00\x00"
"\x00\x20\x50\x88\x50\x20\x00\x00\x88\x10\x20\x40\x80\x28\x00\x00"
"\x7c\xa8\xa8\x68\x28\x28\x28\x00\x38\x40\x30\x48\x48\x30\x08\x70"
"\x00\x00\x00\x00\x00\x00\xff\xff\xf0\xf0\xf0\xf0\x0f\x0f\x0f\x0f"
"\x00\x00\xff\xff\xff\xff\xff\xff\xff\xff\x00\x00\x00\x00\x00\x00"
"\x00\x00\x00\x3c\x3c\x00\x00\x00\xff\xff\xff\xff\xff\xff\x00\x00"
"\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\x0f\x0f\x0f\x0f\xf0\xf0\xf0\xf0"
"\xfc\xfc\xfc\xfc\xfc\xfc\xfc\xfc\x03\x03\x03\x03\x03\x03\x03\x03"
"\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x11\x22\x44\x88\x11\x22\x44\x88"
"\x88\x44\x22\x11\x88\x44\x22\x11\xfe\x7c\x38\x10\x00\x00\x00\x00"
"\x00\x00\x00\x00\x10\x38\x7c\xfe\x80\xc0\xe0\xf0\xe0\xc0\x80\x00"
"\x01\x03\x07\x0f\x07\x03\x01\x00\xff\x7e\x3c\x18\x18\x3c\x7e\xff"
"\x81\xc3\xe7\xff\xff\xe7\xc3\x81\xf0\xf0\xf0\xf0\x00\x00\x00\x00"
"\x00\x00\x00\x00\x0f\x0f\x0f\x0f\x0f\x0f\x0f\x0f\x00\x00\x00\x00"
"\x00\x00\x00\x00\xf0\xf0\xf0\xf0\x33\x33\xcc\xcc\x33\x33\xcc\xcc"
"\x00\x20\x20\x50\x50\x88\xf8\x00\x20\x20\x70\x20\x70\x20\x20\x00"
"\x00\x00\x00\x50\x88\xa8\x50\x00\xff\xff\xff\xff\xff\xff\xff\xff"
"\x00\x00\x00\x00\xff\xff\xff\xff\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0"
"\x0f\x0f\x0f\x0f\x0f\x0f\x0f\x0f\xff\xff\xff\xff\x00\x00\x00\x00"
"\x00\x00\x68\x90\x90\x90\x68\x00\x30\x48\x48\x70\x48\x48\x70\xc0"
"\xf8\x88\x80\x80\x80\x80\x80\x00\xf8\x50\x50\x50\x50\x50\x98\x00"
"\xf8\x88\x40\x20\x40\x88\xf8\x00\x00\x00\x78\x90\x90\x90\x60\x00"
"\x00\x50\x50\x50\x50\x68\x80\x80\x00\x50\xa0\x20\x20\x20\x20\x00"
"\xf8\x20\x70\xa8\xa8\x70\x20\xf8\x20\x50\x88\xf8\x88\x50\x20\x00"
"\x70\x88\x88\x88\x50\x50\xd8\x00\x30\x40\x40\x20\x50\x50\x50\x20"
"\x00\x00\x00\x50\xa8\xa8\x50\x00\x08\x70\xa8\xa8\xa8\x70\x80\x00"
"\x38\x40\x80\xf8\x80\x40\x38\x00\x70\x88\x88\x88\x88\x88\x88\x00"
"\x00\xf8\x00\xf8\x00\xf8\x00\x00\x20\x20\xf8\x20\x20\x00\xf8\x00"
"\xc0\x30\x08\x30\xc0\x00\xf8\x00\x18\x60\x80\x60\x18\x00\xf8\x00"
"\x10\x28\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\xa0\x40"
"\x00\x20\x00\xf8\x00\x20\x00\x00\x00\x50\xa0\x00\x50\xa0\x00\x00"
"\x00\x18\x24\x24\x18\x00\x00\x00\x00\x30\x78\x78\x30\x00\x00\x00"
"\x00\x00\x00\x00\x30\x00\x00\x00\x3e\x20\x20\x20\xa0\x60\x20\x00"
"\xa0\x50\x50\x50\x00\x00\x00\x00\x40\xa0\x20\x40\xe0\x00\x00\x00"
"\x00\x38\x38\x38\x38\x38\x38\x00\x00\x00\x00\x00\x00\x00\x00";

/**********************************************************************/
// Called by DoomMain.
void I_Init (void)
{
  int p;

  I_InitSound ();
  I_InitMusic ();
  //I_InitGraphics ();

  // Init DC controller stuff
  cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
  kbd = maple_enum_type(0, MAPLE_FUNC_KEYBOARD);
  if (kbd)
    kbd_set_queue(0); // turn off keyboard queue
  mky = maple_enum_type(0, MAPLE_FUNC_MOUSE);
  //jump = maple_enum_type(0, MAPLE_FUNC_PURUPURU);

  ff_enable = M_CheckParm ("-rumble");

  p = M_CheckParm ("-noanalog");
  if (p)
    stick_disabled = 1;
  else
  {
    p = M_CheckParm ("-analogcx");
    if (p && p < myargc - 1)
      stick_cx = atoi (myargv[p+1]);
    p = M_CheckParm ("-analogcy");
    if (p && p < myargc - 1)
      stick_cy = atoi (myargv[p+1]);
    p = M_CheckParm ("-analogminx");
    if (p && p < myargc - 1)
      stick_minx = atoi (myargv[p+1]);
    p = M_CheckParm ("-analogminy");
    if (p && p < myargc - 1)
      stick_miny = atoi (myargv[p+1]);
    p = M_CheckParm ("-analogmaxx");
    if (p && p < myargc - 1)
      stick_maxx = atoi (myargv[p+1]);
    p = M_CheckParm ("-analogmaxy");
    if (p && p < myargc - 1)
      stick_maxy = atoi (myargv[p+1]);
  }

  p = M_CheckParm ("-cheat1");
  if (p && p < myargc - 1)
    ctrl_cheat1 = atoi (myargv[p+1]);
  p = M_CheckParm ("-cheat2");
  if (p && p < myargc - 1)
    ctrl_cheat2 = atoi (myargv[p+1]);
  p = M_CheckParm ("-cheat3");
  if (p && p < myargc - 1)
    ctrl_cheat3 = atoi (myargv[p+1]);
}

/**********************************************************************/
// Called by startup code
// to get the ammount of memory to malloc
// for the zone management.
byte*  I_ZoneBase (int *size)
{
  byte *zone;
  int p;

  p = M_CheckParm ("-heapsize");
  if (p && p < myargc - 1)
    *size = 1024 * atoi (myargv[p+1]);
  else
    *size = MAX_ZONESIZE;

  if ((zone = (byte *)malloc(*size)) == NULL)
  {
    printf("Couldn't allocate %d bytes for zone\n", *size);
    *size = MIN_ZONESIZE;
    if ((zone = (byte *)malloc(*size)) == NULL)
      I_Error ("malloc() %d bytes for zone management failed", *size);
  }

  printf ("I_ZoneBase(): Allocated %d bytes for zone management\n", *size);

  return zone;
}

/**********************************************************************/
// Called by D_DoomLoop,
// returns current time in tics.

int I_GetTime (void)
{
  uint64 curr_ms;

  thd_pass();

  curr_ms = timer_ms_gettime64();
  return curr_ms * 2294 >> 16;
}

/**********************************************************************/
//
// Called by D_DoomLoop,
// called before processing any tics in a frame
// (just after displaying a frame).
// Time consuming syncronous operations
// are performed here (joystick reading).
// Can call D_PostEvent.
//
void I_StartFrame (void)
{
  if (quit_requested)
    I_Error ("User forced quit\n");

  thd_pass();
  dc_getevents();
  dc_handle_ff();
  if (!(total_frames & 7))
    update_vmu_stat(); // update every 8 frames
}

/**********************************************************************/
//
// Called by D_DoomLoop,
// called before processing each tic in a frame.
// Quick syncronous operations are performed here.
// Can call D_PostEvent.
void I_StartTic (void)
{
  thd_pass();
  if (quit_requested)
    I_Error ("User forced quit\n");
}

/**********************************************************************/
// Asynchronous interrupt functions should maintain private queues
// that are read by the synchronous functions
// to be converted into events.

// Either returns a null ticcmd,
// or calls a loadable driver to build it.
// This ticcmd will then be modified by the gameloop
// for normal input.
ticcmd_t  emptycmd;
ticcmd_t* I_BaseTiccmd (void)
{
  return &emptycmd;
}

/**********************************************************************/
// Called by M_Responder when quit is selected.
// Clean exit, displays sell blurb.
void I_Quit (void)
{
  // clear VMU LCD display
  memset(bitmap, 0, 48*32/8);
  StatusDrawLCD(bitmap);

  vid_shutdown();
  bfont_set_32bit_mode(0);
  bfont_set_background_color(MAKE_RGB565(0xFF000000));
  bfont_set_foreground_color(MAKE_RGB565(0xFFFFFFFF));
  DebugDC_Init(DM_640x480, PM_RGB565); // enable debug output to screen

  D_QuitNetGame ();
  I_ShutdownSound();
  I_ShutdownMusic();
  printf (" M_SaveDefaults: Save in-game defaults.\n");
  M_SaveDefaults ();
  I_ShutdownGraphics();

  thd_sleep(5*1000);
  exit(0);
}

/**********************************************************************/
// Allocates from low memory under dos,
// just mallocs under unix
byte* I_AllocLow (int length)
{
  byte*  mem;

  if ((mem = (byte *)malloc (length)) == NULL)
    I_Error ("Out of memory allocating %d bytes", length);
  memset (mem,0,length);
  return mem;
}

/**********************************************************************/
// Force Feedback
void I_Tactile (int freq, int intensity, int select)
{
    ff_frequency = freq;
    ff_intensity = intensity;
    ff_select = select;
    ff_timeout = I_GetTime() + 35;

    // FF will be handled at the same time as the events
}

/**********************************************************************/
void *I_malloc (size_t size)
{
  void *b;

  if ((b = malloc (size)) == NULL)
    I_Error ("Out of memory allocating %d bytes", size);
  return b;
}

/**********************************************************************/
void *I_calloc (size_t nelt, size_t esize)
{
  void *b;

  if ((b = calloc (nelt, esize)) == NULL)
    I_Error ("Out of memory allocating %d bytes", nelt * esize);
  return b;
}

/**********************************************************************/
void I_Error (char *error, ...)
{
  va_list  argptr;
  char msg[256];

  vid_shutdown();
  bfont_set_32bit_mode(0);
  bfont_set_background_color(MAKE_RGB565(0xFF000000));
  bfont_set_foreground_color(MAKE_RGB565(0xFFFFFFFF));
  DebugDC_Init(DM_640x480, PM_RGB565); // enable debug output to screen

  // Message first.
  va_start (argptr, error);
  vsprintf (msg, error, argptr);
  va_end (argptr);
  printf("\n\n Error: %s\n", msg);
  thd_sleep(5*1000);

  // Shutdown. Here might be other errors.
  if (demorecording)
    G_CheckDemoStatus ();

  I_Quit();
}

/**********************************************************************/
// misc extra functions needed by DOOM

int access(const char *path, int mode)
{
  FILE *lock;

  if ((lock = fopen(path, "rb"))) {
    fclose(lock);
    return 0;
  }

  return -1;
}

sint32_t _atoi(uint8_t *s)
{
#define ISNUM(c) ((c) >= '0' && (c) <= '9')

  register uint32_t i = 0;
  register uint32_t sign = 0;
  register uint8_t *p = s;

  /* Conversion starts at the first numeric character or sign. */
  while(*p && !ISNUM(*p) && *p != '-') p++;

  /*
     If we got a sign, set a flag.
     This will negate the value before return.
   */
  if(*p == '-')
    {
      sign++;
      p++;
    }

  /* Don't care when 'u' overflows (Bug?) */
  while(ISNUM(*p))
    {
      i *= 10;
      i += *p++ - '0';
    }

  /* Return according to sign */
  if(sign)
    return - i;
  else
    return i;

#undef ISNUM
}

#if 0
int islower(int c)
{
  if (c < 'a')
    return 0;

  if (c > 'z')
    return 0;

  // passed both criteria, so it
  // is a lower case alpha char
  return 1;
}
#endif

#if 0
int _toupper(int c)
{
  if ( islower( c ) ){
    c -= 32;
  }
  return c;
}
#endif

/**********************************************************************/
static void dc_handle_ff(void)
{
    maple_device_t * jump;

    if (!ff_enable)
        return; // early out

    jump = maple_enum_type(0, MAPLE_FUNC_PURUPURU);

    if (ff_timeout && jump)
    {
        if (I_GetTime() >= ff_timeout)
        {
            static purupuru_effect_t effect;
            effect.duration = 0x00;
            effect.effect2 = 0x00;
            effect.effect1 = 0x00;
            effect.special = PURUPURU_SPECIAL_MOTOR1;
            purupuru_rumble(jump, &effect);
            ff_timeout = 0;
            ff_intensity = 0;
        }
    }

    if (ff_intensity && jump)
    {
        static purupuru_effect_t effect;
        effect.duration = 0x11;
        effect.effect2 = ff_frequency;
        effect.effect1 = PURUPURU_EFFECT1_INTENSITY(ff_intensity);
        effect.special = PURUPURU_SPECIAL_MOTOR1 | ff_select;
        purupuru_rumble(jump, &effect);
        ff_intensity = 0;
    }
}

/**********************************************************************/
static void lcd_put_char(int x, int y, unsigned char ch)
{
    int j;
    unsigned char *font;

    font = &msx[(int)ch * 8];

    for (j=0; j<8; j++, font++)
        bitmap[(y*8 + j)*6 + x] = *font;
}

static void update_vmu_stat(void)
{
    player_t *plyr = &players[consoleplayer];
    int ammo = plyr->ammo[weaponinfo[plyr->readyweapon].ammo];
    int health = plyr->health;
    int armor = plyr->armorpoints;
    int i, f;

    memset(bitmap, 0, 48*32/8);
    lcd_put_char(0, 0, 'A');
    lcd_put_char(1, 0, ':');
    i = 2;
    f = 0;
    if (ammo / 100)
    {
        lcd_put_char(i, 0, '0' + ammo / 100);
        i++;
        ammo -= (ammo / 100) * 100;
        f = 1;
    }
    if (ammo / 10 || f)
    {
        lcd_put_char(i, 0, '0' + ammo / 10);
        i++;
        ammo -= (ammo / 10) * 10;
    }
    lcd_put_char(i, 0, '0' + ammo);

    lcd_put_char(0, 1, 'H');
    lcd_put_char(1, 1, ':');
    i = 2;
    f = 0;
    if (health / 100)
    {
        lcd_put_char(i, 1, '0' + health / 100);
        i++;
        health -= (health / 100) * 100;
        f = 1;
    }
    if (health / 10 || f)
    {
        lcd_put_char(i, 1, '0' + health / 10);
        i++;
        health -= (health / 10) * 10;
    }
    lcd_put_char(i, 1, '0' + health);

    lcd_put_char(0, 2, 'R');
    lcd_put_char(1, 2, ':');
    i = 2;
    f = 0;
    if (armor / 100)
    {
        lcd_put_char(i, 2, '0' + armor / 100);
        i++;
        armor -= (armor / 100) * 100;
        f = 1;
    }
    if (armor / 10 || f)
    {
        lcd_put_char(i, 2, '0' + armor / 10);
        i++;
        armor -= (armor / 10) * 10;
    }
    lcd_put_char(i, 2, '0' + armor);

    if (plyr->cards[0])
    {
        lcd_put_char(0, 3, 'B');
        lcd_put_char(1, 3, 'C');
    }
    if (plyr->cards[1])
    {
        lcd_put_char(2, 3, 'Y');
        lcd_put_char(3, 3, 'C');
    }
    if (plyr->cards[2])
    {
        lcd_put_char(4, 3, 'R');
        lcd_put_char(5, 3, 'C');
    }
    if (plyr->cards[3])
    {
        lcd_put_char(0, 3, 'B');
        lcd_put_char(1, 3, 'S');
    }
    if (plyr->cards[4])
    {
        lcd_put_char(2, 3, 'Y');
        lcd_put_char(3, 3, 'S');
    }
    if (plyr->cards[5])
    {
        lcd_put_char(4, 3, 'R');
        lcd_put_char(5, 3, 'S');
    }

    StatusDrawLCD(bitmap);
}

/**********************************************************************/

static void dc_do_cheat(int cheat)
{
  event_t event;
  char *str;
  int i;

  switch (cheat)
  {
    case 1: // God Mode
    str = "iddqd";
    break;
    case 2: // Fucking Arsenal
    str = "idfa";
    break;
    case 3: // Key Full Ammo
    str = "idkfa";
    break;
    case 4: // No Clipping
    str = "idclip";
    break;
    case 5: // Toggle Map
    str = "iddt";
    break;
    case 6: // Invincible with Chainsaw
    str = "idchoppers";
    break;
    case 7: // Berserker Strength Power-up
    str = "idbeholds";
    break;
    case 8: // Invincibility Power-up
    str = "idbeholdv";
    break;
    case 9: // Invisibility Power-Up
    str = "idbeholdi";
    break;
    case 10: // Automap Power-up
    str = "idbeholda";
    break;
    case 11: // Anti-Radiation Suit Power-up
    str = "idbeholdr";
    break;
    case 12: // Light-Amplification Visor Power-up
    str = "idbeholdl";
    break;
    default:
    return;
  }

  for (i=0; i<strlen(str); i++)
  {
    event.type = ev_keydown;
    event.data1 = str[i];
    D_PostEvent (&event);
    event.type = ev_keyup;
    event.data1 = str[i];
    D_PostEvent (&event);
  }
}

extern int get_text_osk(char *input, char *desc);
extern void video_set_vmode(void);

static void dc_send_string(void)
{
  event_t event;
  int ok, i;
  char str[32];
  char *desc = "Enter In-Game Text";

  str[0] = '\0';
  ok = get_text_osk(str, desc);

  video_set_vmode(); // restore game display mode

  if (ok)
  {
    strcat(str, " ");
    for (i=0; i<strlen(str); i++)
    {
      event.type = ev_keydown;
      event.data1 = str[i];
      D_PostEvent (&event);
      event.type = ev_keyup;
      event.data1 = str[i];
      D_PostEvent (&event);
    }
  }
}

// fake defines for triggers
#define CONT_LTRIGGER (1<<16)
#define CONT_RTRIGGER (1<<17)

int key_table[128] = {
 0, 0, 0, 0, 'a', 'b', 'c', 'd',                // 00-07
 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',        // 08-0F
 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',        // 10-17
 'u', 'v', 'w', 'x', 'y', 'z', '1', '2',        // 18-1F
 '3', '4', '5', '6', '7', '8', '9', '0',        // 20-27
 KEY_ENTER, KEY_ESCAPE, KEY_BACKSPACE, KEY_TAB, ' ', '-', '=', '[',        // 28-2F
 ']', '\\', 0, ';', '\'', '`', ',', '.',        // 30-37
 '/', 0, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6,        // 38-3F
 KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12, KEY_PRINT, KEY_SCRLOCK,        // 40-47
 KEY_PAUSE, KEY_INSERT, KEY_HOME, KEY_PGUP, KEY_DEL, KEY_END, KEY_PGDN, KEY_RIGHTARROW,        // 48-4F
 KEY_LEFTARROW, KEY_DOWNARROW, KEY_UPARROW, KEY_PAD_NUMLOCK, KEY_PAD_SLASH, KEY_PAD_ASTER, KEY_PAD_MINUS, KEY_PAD_PLUS,        // 50-57
 KEY_PAD_ENTER, KEY_PAD_1, KEY_PAD_2, KEY_PAD_3, KEY_PAD_4, KEY_PAD_5, KEY_PAD_6, KEY_PAD_7,        // 58-5F
 KEY_PAD_8, KEY_PAD_9, KEY_PAD_0, KEY_PAD_PERIOD, 0, 0, 0, 0,        // 60-67
 0, 0, 0, 0, 0, 0, 0, 0,        // 68-6F
 0, 0, 0, 0, 0, 0, 0, 0,        // 70-77
 0, 0, 0, 0, 0, 0, 0, 0         // 78-7F
};

static void dc_getevents (void)
{
  event_t event;
  event_t joyevent;
  event_t mouseevent;
  short mousex, mousey;
  int mouseb;
  char weapons[8] = { '1', '2', '3', '3', '4', '5', '6', '7' };
  static uint32 buttons, previous = -1;
  static int pad_weapon = 2;
  static int rx, ry;

  //if (!cont)
    cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
  //if (!kbd)
  {
    kbd = maple_enum_type(0, MAPLE_FUNC_KEYBOARD);
    if (kbd)
      kbd_set_queue(0); // turn off keyboard queue
  }
  //if (!mky)
    mky = maple_enum_type(0, MAPLE_FUNC_MOUSE);

  if (cont)
    cstate = (cont_state_t *)maple_dev_status(cont);
  else
    cstate = NULL;
  if (kbd)
    kstate = (kbd_state_t *)maple_dev_status(kbd);
  else
    kstate = NULL;
  if (mky)
    mstate = (mouse_state_t *)maple_dev_status(mky);
  else
    mstate = NULL;

  if (cstate)
  {
    buttons = cstate->buttons;
    buttons |= cstate->ltrig > 16 ? CONT_LTRIGGER : 0;
    buttons |= cstate->rtrig > 16 ? CONT_RTRIGGER : 0;
  }
  else
    buttons = previous;

  if (previous == -1)
  {
    previous = buttons;
    rx = ry = stick_cx > 16 || stick_cy > 16 ? 32 : 24;
  }

  // use analog stick as mouse unless -noanalog specified or no controller found
  if (!stick_disabled && cstate)
  {
    // we don't use the min/max yet, and center just affects the comparison
    mousex = mousey = 0;
    if (abs(cstate->joyx) > rx)
      mousex = cstate->joyx << 1;
    if (abs(cstate->joyy) > ry)
      mousey = -cstate->joyy << 1;

    if (mousex || mousey)
    {
      mouseevent.type = ev_mouse;
      mouseevent.data1 = 0; // no mouse buttons
      mouseevent.data2 = mousex;
      mouseevent.data3 = mousey;
      D_PostEvent (&mouseevent);
    }
  }

  // use mouse as mouse if found
  if (mstate)
  {
    static int prev_mkyb = -1;

    mouseb = (mstate->buttons & MOUSE_LEFTBUTTON ? 1 : 0) | (mstate->buttons & MOUSE_RIGHTBUTTON ? 2 : 0) | (mstate->buttons & MOUSE_SIDEBUTTON ? 4 : 0);
    mousex = mstate->dx << 2;
    mousey = -mstate->dy << 2;

    if (mousex || mousey || (mouseb != prev_mkyb))
    {
      mouseevent.type = ev_mouse;
      mouseevent.data1 = mouseb;
      mouseevent.data2 = mousex;
      mouseevent.data3 = mousey;
      D_PostEvent (&mouseevent);
      prev_mkyb = mouseb;
    }
  }

  // use keyboard if found
  if (kstate)
  {
    int i;
    static int prev_shift = -1;
    static uint8 prev_matrix[128];

    if (prev_shift == -1)
    {
        prev_shift = kstate->shift_keys;
        memset(prev_matrix, 0, 128);
    }
    switch(kstate->shift_keys ^ prev_shift)
    {
        case KBD_MOD_LALT:
        case KBD_MOD_RALT:
        if (kstate->shift_keys & (KBD_MOD_LALT | KBD_MOD_RALT))
        {
            event.type = ev_keydown;
            event.data1 = KEY_RALT;
            D_PostEvent (&event);
        }
        else
        {
            event.type = ev_keyup;
            event.data1 = KEY_RALT;
            D_PostEvent (&event);
        }
        break;

        case KBD_MOD_LCTRL:
        case KBD_MOD_RCTRL:
        if (kstate->shift_keys & (KBD_MOD_LCTRL | KBD_MOD_RCTRL))
        {
            event.type = ev_keydown;
            event.data1 = KEY_RCTRL;
            D_PostEvent (&event);
        }
        else
        {
            event.type = ev_keyup;
            event.data1 = KEY_RCTRL;
            D_PostEvent (&event);
        }
        break;

        case KBD_MOD_LSHIFT:
        case KBD_MOD_RSHIFT:
        if (kstate->shift_keys & (KBD_MOD_LSHIFT | KBD_MOD_RSHIFT))
        {
            event.type = ev_keydown;
            event.data1 = KEY_RSHIFT;
            D_PostEvent (&event);
        }
        else
        {
            event.type = ev_keyup;
            event.data1 = KEY_RSHIFT;
            D_PostEvent (&event);
        }
        break;
    }
    prev_shift = kstate->shift_keys;

    // handle regular keys
    for (i=4; i<0x65; i++)
    {
        if (kstate->matrix[i] != 0 && prev_matrix[i] == 0 && key_table[i] != 0)
        {
            event.type = ev_keydown;
            event.data1 = key_table[i];
            D_PostEvent (&event);
        }
        else if (kstate->matrix[i] == 0 && prev_matrix[i] != 0 && key_table[i] != 0)
        {
            event.type = ev_keyup;
            event.data1 = key_table[i];
            D_PostEvent (&event);
        }
    }
    memcpy(prev_matrix, kstate->matrix, 128);
  }

  // now handle controller
  if (buttons == previous)
    return;

    joyevent.type = ev_joystick;
    joyevent.data1 = joyevent.data2 = joyevent.data3 = 0;

    if (!(buttons & CONT_START))
    {
        // A = Ctrl (Fire)
        if (!(buttons & CONT_Y) && buttons & CONT_A)
            joyevent.data1 |= 1;

        // X = SHIFT (Run)
        if (!(buttons & CONT_Y) && buttons & CONT_X)
            joyevent.data1 |= 4;

        // B = SPACE (Open/Operate)
        if (!(buttons & CONT_Y) && buttons & CONT_B)
            joyevent.data1 |= 8;

        // directionals
        if (!(buttons & CONT_Y) && buttons & CONT_DPAD_LEFT)
            joyevent.data2 = -1;
        else if (!(buttons & CONT_Y) && buttons & CONT_DPAD_RIGHT)
            joyevent.data2 = 1;
        if (!(buttons & CONT_Y) && buttons & CONT_DPAD_UP)
            joyevent.data3 = -1;
        else if (!(buttons & CONT_Y) && buttons & CONT_DPAD_DOWN)
            joyevent.data3 = 1;

        // LTRIGGER = ',' (Strafe Left)
        if (!(buttons & CONT_Y) && buttons & CONT_LTRIGGER && !(previous & CONT_LTRIGGER))
        {
            event.type = ev_keydown;
            event.data1 = ',';
            D_PostEvent (&event);
        }
        else if (!(buttons & CONT_Y) && !(buttons & CONT_LTRIGGER) && previous & CONT_LTRIGGER)
        {
            event.type = ev_keyup;
            event.data1 = ',';
            D_PostEvent (&event);
        }
        // RTRIGGER = '.' (Strafe Right)
        if (!(buttons & CONT_Y) && buttons & CONT_RTRIGGER && !(previous & CONT_RTRIGGER))
        {
            event.type = ev_keydown;
            event.data1 = '.';
            D_PostEvent (&event);
        }
        else if (!(buttons & CONT_Y) && !(buttons & CONT_RTRIGGER) && previous & CONT_RTRIGGER)
        {
            event.type = ev_keyup;
            event.data1 = '.';
            D_PostEvent (&event);
        }

        // LTRIGGER + Y =  (Prev Weapon)
        if (buttons & CONT_Y && buttons & CONT_LTRIGGER && !(previous & CONT_LTRIGGER))
        {
            pad_weapon -= 1;
            if (pad_weapon<0) pad_weapon = 0;
            event.type = ev_keydown;
            event.data1 = weapons[pad_weapon];
            D_PostEvent (&event);
        }
        else if (buttons & CONT_Y && !(buttons & CONT_LTRIGGER) && previous & CONT_LTRIGGER)
        {
            event.type = ev_keyup;
            event.data1 = weapons[pad_weapon];
            D_PostEvent (&event);
        }
        // RTRIGGER + Y =  (Next Weapon)
        if (buttons & CONT_Y && buttons & CONT_RTRIGGER && !(previous & CONT_RTRIGGER))
        {
            pad_weapon += 1;
            if (pad_weapon>7) pad_weapon = 7;
            event.type = ev_keydown;
            event.data1 = weapons[pad_weapon];
            D_PostEvent (&event);
        }
        else if (buttons & CONT_Y && !(buttons & CONT_RTRIGGER) && previous & CONT_RTRIGGER)
        {
            event.type = ev_keyup;
            event.data1 = weapons[pad_weapon];
            D_PostEvent (&event);
        }

        // UP + Y =  F11 (Gamma)
        if (buttons & CONT_Y && buttons & CONT_DPAD_UP && !(previous & CONT_DPAD_UP))
        {
            pad_weapon -= 1;
            if (pad_weapon<1) pad_weapon = 1;
            event.type = ev_keydown;
            event.data1 = KEY_F11;
            D_PostEvent (&event);
        }
        else if (buttons & CONT_Y && !(buttons & CONT_DPAD_UP) && previous & CONT_DPAD_UP)
        {
            event.type = ev_keyup;
            event.data1 = KEY_F11;
            D_PostEvent (&event);
        }

        // DOWN + Y =  F5 (Detail)
        if (buttons & CONT_Y && buttons & CONT_DPAD_DOWN && !(previous & CONT_DPAD_DOWN))
        {
            pad_weapon -= 1;
            if (pad_weapon<1) pad_weapon = 1;
            event.type = ev_keydown;
            event.data1 = KEY_F5;
            D_PostEvent (&event);
        }
        else if (buttons & CONT_Y && !(buttons & CONT_DPAD_DOWN) && previous & CONT_DPAD_DOWN)
        {
            event.type = ev_keyup;
            event.data1 = KEY_F5;
            D_PostEvent (&event);
        }

        // B + Y =  (Cheat1)
        if (buttons & CONT_Y && buttons & CONT_B && !(previous & CONT_B))
            dc_do_cheat(ctrl_cheat1);

        // A + Y =  (Cheat2)
        if (buttons & CONT_Y && buttons & CONT_A && !(previous & CONT_A))
            dc_do_cheat(ctrl_cheat2);

        // X + Y =  (Cheat1)
        if (buttons & CONT_Y && buttons & CONT_X && !(previous & CONT_X))
            dc_do_cheat(ctrl_cheat3);

        // LEFT + Y = (Prev Cheat)
        if (buttons & CONT_Y && buttons & CONT_DPAD_LEFT && !(previous & CONT_DPAD_LEFT))
            dc_cheat_select = dc_cheat_select > 0 ? dc_cheat_select - 1 : 12;

        // RIGHT + Y = (Next Cheat)
        if (buttons & CONT_Y && buttons & CONT_DPAD_RIGHT && !(previous & CONT_DPAD_RIGHT))
            dc_cheat_select = dc_cheat_select < 12 ? dc_cheat_select + 1 : 0;

        // Y release = y / BACKSPACE (Exit, Backspace, or do cheat when done selecting)
        if (!(buttons & CONT_Y) && previous & CONT_Y)
        {
            if (dc_cheat_select)
            {
                dc_do_cheat(dc_cheat_select);
                dc_cheat_select = 0;
            }

            event.type = ev_keydown;
            event.data1 = 'y';
            D_PostEvent (&event);
            event.type = ev_keyup;
            event.data1 = 'y';
            D_PostEvent (&event);

            event.type = ev_keydown;
            event.data1 = KEY_BACKSPACE;
            D_PostEvent (&event);
            event.type = ev_keyup;
            event.data1 = KEY_BACKSPACE;
            D_PostEvent (&event);

            event.type = ev_keydown;
            event.data1 = KEY_BACKSPACE;
            D_PostEvent (&event);
            event.type = ev_keyup;
            event.data1 = KEY_BACKSPACE;
            D_PostEvent (&event);
        }
    }
    else
    {
        // START + A = (Pause)
        if (buttons & CONT_A && !(previous & CONT_A))
        {
            event.type = ev_keydown;
            event.data1 = KEY_PAUSE;
            D_PostEvent (&event);
        }
        else if (!(buttons & CONT_A) && previous & CONT_A)
        {
            event.type = ev_keyup;
            event.data1 = KEY_PAUSE;
            D_PostEvent (&event);
        }
        // START + B = ESC (menu)
        if (buttons & CONT_B && !(previous & CONT_B))
        {
            event.type = ev_keydown;
            event.data1 = KEY_ESCAPE;
            D_PostEvent (&event);
        }
        else if (!(buttons & CONT_B) && previous & CONT_B)
        {
            event.type = ev_keyup;
            event.data1 = KEY_ESCAPE;
            D_PostEvent (&event);
        }
        // START + X = TAB (map)
        if (buttons & CONT_X && !(previous & CONT_X))
        {
            event.type = ev_keydown;
            event.data1 = 9;
            D_PostEvent (&event);
        }
        else if (!(buttons & CONT_X) && previous & CONT_X)
        {
            event.type = ev_keyup;
            event.data1 = 9;
            D_PostEvent (&event);
        }
        // START + Y = (get text string from OSK)
        if (buttons & CONT_Y && !(previous & CONT_Y))
            dc_send_string();
    }

    D_PostEvent (&joyevent);

    previous = buttons;
}
