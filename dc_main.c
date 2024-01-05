// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// $Log:$
//
// DESCRIPTION:
//  Main program, simply calls D_DoomMain high level loop.
//
//-----------------------------------------------------------------------------

#include <kos.h>
#include <kos/net.h>

#include <dc/maple.h>
#include <dc/maple/controller.h>
#include <dc/maple/purupuru.h>
#include <dc/maple/keyboard.h>
#include <dc/video.h>
#include <dc/biosfont.h>

#include "dc_vmu.h"
#include "danzeff/danzeff.h"
#include "debug_dc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

// port version/revision
#define VERS     1
#define REVS     5


#include "doomdef.h"

#include "m_argv.h"
#include "d_main.h"
#include "i_system.h"
#include "m_fixed.h"

/* Defines to convert colors */
#define MAKE_RGB555(c) (((c & 0xF80000)>>9)|((c & 0x00F800)>>6)|((c & 0x0000F8)>>3))
#define MAKE_RGB565(c) (((c & 0xF80000)>>8)|((c & 0x00FC00)>>5)|((c & 0x0000F8)>>3))


KOS_INIT_FLAGS(INIT_DEFAULT | INIT_NET);


// base code version
int VERSION = 110;

char dc_home[8];

int dc_net_available = 0;
char *dc_net_ipaddr;
char dc_ipaddr[20];
struct sockaddr_in dc_net_dnssrv;

static int video_cable = CT_COMPOSITE;

extern int quit_requested;

int guiVMode;
int guiVMode32;

// variables used by danzeff
int vid_width = 320;

// return a max of 26 characters
int get_text_osk(char *input, char *desc)
{
    static maple_device_t *cont = NULL, *kbd = NULL;

    int disp_mode = (guiVMode == DM_640x480_PAL_IL) ? DM_320x240_PAL : (guiVMode == DM_640x480_NTSC_IL) ? DM_320x240_NTSC : DM_320x240;
    vid_set_mode(disp_mode | DM_MULTIBUFFER, PM_RGB565);
    vid_empty();
    bfont_set_32bit_mode(0);
    guiVMode32 = 0;

    if (!danzeff_isinitialized())
        danzeff_load();

    if (!danzeff_isinitialized())
        return 0; // couldn't init danzeff

    while (1)
    {
        unsigned int key;

        vid_clear(0, 0, 0); // clear draw buffer

        danzeff_moveTo(85, 68);
        danzeff_render();

        bfont_set_foreground_color(MAKE_RGB565(0xFFCCCCCC));
        bfont_set_background_color(MAKE_RGB565(0xFF1020FF));
        bfont_draw_str((uint16 *)((uint32)vram_s + (320 - strlen(desc) * 12) + 12 * 320 * 2), 320, 1, desc);

        bfont_set_foreground_color(MAKE_RGB565(0xFFFFFFFF));
        bfont_set_background_color(MAKE_RGB565(0xFF000000));
        bfont_draw_str((uint16 *)((uint32)vram_s + 4 * 2 + 40 * 320 * 2), 320, 1, input);

        if (!cont)
            cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
        if (!kbd)
            kbd = maple_enum_type(0, MAPLE_FUNC_KEYBOARD);

        key = danzeff_readInput(getCtrlFromJoystick(cont));
        switch (key)
        {
            case 0: // no input
            if (kbd)
            {
                while ((key = kbd_get_key()) != -1)
                {
                    if (key == 13)
                        return 1; // enter
                    if (key == 27)
                        return 0; // escape
                    if (key == 8 && strlen(input) > 0)
                    {
                        input[strlen(input) - 1] = '\0'; // backspace
                    }
                    else if (key >= 32 && key <= 126 && strlen(input) < 26)
                    {
                        input[strlen(input) + 1] = '\0';
                        input[strlen(input)] = (char)key;
                    }
                }
            }
            break;
            case 1: // move left
            break;
            case 2: // move right
            break;
            case 3: // select
            break;
            case 4: // start
            return 0;
            break;
            case 8: // backspace
            if (strlen(input) > 0)
                input[strlen(input) - 1] = '\0';
            break;
            case 10: // enter
            return 1;
            break;
            default: // ascii character
            if (strlen(input) < 26)
            {
                input[strlen(input) + 1] = '\0';
                input[strlen(input)] = (char)key;
            }
            break;
        }
        vid_waitvbl();
        vid_flip(-1);
    }
    return 0;
}

void dc_font_init(void)
{
    /* use ISO8859-1 encoding */
    bfont_set_encoding(BFONT_CODE_ISO8859_1);
}


/*
 * GUI support code
 *
 */

extern char *RequestFile (char *initialPath);

void gui_PrePrint(void)
{
    vid_clear(0, 0, 0); // clear draw buffer
}

void gui_PostPrint(void)
{
    vid_waitvbl();
    vid_flip(-1);
}

int gui_PrintWidth(char *text)
{
    return strlen(text)*12;
}

void gui_Print(char *text, uint32 fc, uint32 bc, int x, int y)
{
    if (guiVMode32)
    {
        bfont_set_foreground_color(fc);
        bfont_set_background_color(bc);
    }
    else
    {
        bfont_set_foreground_color(MAKE_RGB565(fc));
        bfont_set_background_color(MAKE_RGB565(bc));
    }
    bfont_draw_str((uint16 *)((uint32)vram_s + (x + y * 640) * 2), 640, (bc & 0xFF000000) ? 1 : 0, text);
}

char *RequestString (char *initialStr)
{
    int ok;
    static char str[32];
    char *desc  = "Enter Menu Text";

    strncpy(str, initialStr, 26);
    str[26] = '\0';
    ok = get_text_osk(str, desc);

    // restore gui display mode
    vid_set_mode(guiVMode | DM_MULTIBUFFER, PM_RGB565);
    vid_empty();
    bfont_set_32bit_mode(0);
    guiVMode32 = 0;

    if (ok)
        return str;

    return 0;
}


struct gui_menu {
    char *text;
    unsigned int flags;
    void *field1;
    void *field2;
    unsigned int enable;
};

struct gui_list {
    char *text;
    int index;
};

// gui_menu.flags
#define GUI_DIVIDER   0
#define GUI_TEXT      1
#define GUI_MENU      2
#define GUI_FUNCTION  3
#define GUI_SELECT    4
#define GUI_TOGGLE    5
#define GUI_INTEGER   6
#define GUI_FILE      7
#define GUI_STRING    8

#define GUI_CENTER 0x10000000
#define GUI_LEFT   0x20000000
#define GUI_RIGHT  0x30000000

// gui_menu.enable
#define GUI_DISABLED 0
#define GUI_ENABLED  1
#define GUI_SET_ME   0xFFFFFFFF

// misc
#define GUI_END_OF_MENU 0xFFFFFFFF
#define GUI_END_OF_LIST 0xFFFFFFFF

int dc_use_tv = 0;
int dc_vid_fps = 0;

int dc_mon_res = 0;
int dc_mon_sync = 0;
int dc_mon_detail = 0;

int dc_tv_cable = -1;
int dc_tv_res = 0;
int dc_tv_sync = 0;
int dc_tv_detail = 0;
int dc_tv_cx = 0;
int dc_tv_cy = 0;

int dc_sfx_enabled = 1;
int dc_music_enabled = 1;

int dc_jump_enable = 0;
int dc_stick_disabled = 0;
int dc_stick_cx = 128;
int dc_stick_cy = 128;
int dc_stick_minx = 0;
int dc_stick_miny = 0;
int dc_stick_maxx = 255;
int dc_stick_maxy = 255;
int dc_ctrl_cheat1= 0;
int dc_ctrl_cheat2= 0;
int dc_ctrl_cheat3= 0;

char *dc_iwad_file = 0;
char *dc_pwad_file1 = 0;
char *dc_pwad_file2 = 0;
char *dc_pwad_file3 = 0;
char *dc_pwad_file4 = 0;
char *dc_deh_file1 = 0;
char *dc_deh_file2 = 0;
char *dc_deh_file3 = 0;
char *dc_deh_file4 = 0;

int dc_game_nomonsters = 0;
int dc_game_respawn = 0;
int dc_game_fast = 0;
int dc_game_turbo = 0;
int dc_game_maponhu = 0;
int dc_game_rotatemap = 0;
int dc_game_deathmatch = 0;
int dc_game_altdeath = 0;
int dc_game_record = 0;
int dc_game_playdemo = 0;
int dc_game_forcedemo = 0;
int dc_game_timedemo = 0;
int dc_game_timer = 0;
int dc_game_avg = 0;
int dc_game_statcopy = 0;
int dc_game_version = 110;
int dc_game_pcchksum = 1;
int dc_game_skill = 2; // Hurt Me Plenty (medium)
int dc_game_level = 1;

int dc_net_enabled = 0;
int dc_net_port = 5029; // 5000 + 0x1d
int dc_net_ticdup = 1;
int dc_net_extratic = 0;
int dc_net_player1 = 1;
char *dc_net_player2 = 0;
char *dc_net_player3 = 0;
char *dc_net_player4 = 0;
int dc_net_error = 0;


int gui_menu_len(struct gui_menu *menu)
{
    int c = 0;
    while (menu[c].flags != GUI_END_OF_MENU)
        c++;
    return c;
}

int gui_list_len(struct gui_list *list)
{
    int c = 0;
    while (list[c].index != GUI_END_OF_LIST)
        c++;
    return c;
}

void do_gui(struct gui_menu *menu, void *menufn, char *exit_msg)
{
    maple_device_t *cont = NULL;
    cont_state_t *cstate;
    int msy, mlen;
    int i, j, k, min, max;
    int csel = 0;
    uint32 fc, bc;
    int tx, ty;
    char line[54];

    mlen = gui_menu_len(menu);
    msy = 224 - mlen*24/2;

    vid_set_mode(guiVMode | DM_MULTIBUFFER, PM_RGB565);
    vid_empty();
    bfont_set_32bit_mode(0);
    guiVMode32 = 0;

    while (cont == NULL)
        cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
    cstate = (cont_state_t *)maple_dev_status(cont);
    while (!(cstate->buttons & CONT_B))
    {
        void (*fnptr)(struct gui_menu *);
        fnptr = menufn;
        if (fnptr)
            (*fnptr)(menu);
        if (cstate->buttons & CONT_A)
        {
            if (menu[csel].enable == GUI_ENABLED)
                switch (menu[csel].flags & 0x0FFFFFFF)
                {
                    void (*fnptr)(void *);
                    char temp[256];
                    char *req;

                    case GUI_MENU:
                    while (cstate->buttons & CONT_A)
                        cstate = (cont_state_t *)maple_dev_status(cont);
                    do_gui((struct gui_menu *)menu[csel].field1, menu[csel].field2, "Prev Menu");
                    break;
                    case GUI_FUNCTION:
                    while (cstate->buttons & CONT_A)
                        cstate = (cont_state_t *)maple_dev_status(cont);
                    fnptr = menu[csel].field1;
                    (*fnptr)(menu[csel].field2);
                    break;
                    case GUI_TOGGLE:
                    while (cstate->buttons & CONT_A)
                        cstate = (cont_state_t *)maple_dev_status(cont);
                    *(int *)menu[csel].field1 ^= 1;
                    break;
                    case GUI_SELECT:
                    if (cstate->buttons & (CONT_DPAD_RIGHT | CONT_DPAD_DOWN))
                    {
                        // next selection
                        while (cstate->buttons & (CONT_DPAD_RIGHT | CONT_DPAD_DOWN))
                            cstate = (cont_state_t *)maple_dev_status(cont);
                        j = gui_list_len((struct gui_list *)menu[csel].field1);
                        k = *(int *)menu[csel].field2;
                        k = (k == (j - 1)) ? 0 : k + 1;
                        *(int *)menu[csel].field2 = k;
                    }
                    if (cstate->buttons & (CONT_DPAD_LEFT | CONT_DPAD_UP))
                    {
                        // previous selection
                        while (cstate->buttons & (CONT_DPAD_LEFT | CONT_DPAD_UP))
                            cstate = (cont_state_t *)maple_dev_status(cont);
                        j = gui_list_len((struct gui_list *)menu[csel].field1);
                        k = *(int *)menu[csel].field2;
                        k = (k == 0) ? j - 1 : k - 1;
                        *(int *)menu[csel].field2 = k;
                    }
                    break;
                    case GUI_FILE:
                    while (cstate->buttons & CONT_A)
                        cstate = (cont_state_t *)maple_dev_status(cont);
                    if (*(int *)menu[csel].field1)
                        free(*(char **)menu[csel].field1);
                    strcpy(temp, dc_home);
                    strcat(temp, (char *)menu[csel].field2);
                    //strcat(temp, "/");
                    req = RequestFile(temp);
                    *(char **)menu[csel].field1 = req ? strdup(req) : 0;
                    break;
                    case GUI_STRING:
                    while (cstate->buttons & CONT_A)
                        cstate = (cont_state_t *)maple_dev_status(cont);
                    temp[0] = 0;
                    if (*(int *)menu[csel].field1)
                    {
                        strcpy(temp, *(char **)menu[csel].field1);
                        free(*(char **)menu[csel].field1);
                    }
                    req = RequestString(temp);
                    *(char **)menu[csel].field1 = req ? strdup(req) : 0;
                    break;
                    case GUI_INTEGER:
                    if (menu[csel].field2)
                    {
                        int *rng = (int *)menu[csel].field2;
                        min = rng[0];
                        max = rng[1];
                    }
                    else
                    {
                        min = (int)0x80000000;
                        max = 0x7FFFFFFF;
                    }
                    if (cstate->buttons & CONT_DPAD_RIGHT)
                    {
                        // +1
                        thd_sleep(200);
                        *(int *)menu[csel].field1 += 1;
                        if (*(int *)menu[csel].field1 > max)
                            *(int *)menu[csel].field1 = max;
                    }
                    if (cstate->buttons & CONT_DPAD_LEFT)
                    {
                        // -1
                        thd_sleep(200);
                        *(int *)menu[csel].field1 -= 1;
                        if (*(int *)menu[csel].field1 < min)
                            *(int *)menu[csel].field1 = min;
                    }
                    if (cstate->buttons & CONT_DPAD_DOWN)
                    {
                        // +10
                        thd_sleep(200);
                        *(int *)menu[csel].field1 += 10;
                        if (*(int *)menu[csel].field1 > max)
                            *(int *)menu[csel].field1 = max;
                    }
                    if (cstate->buttons & CONT_DPAD_UP)
                    {
                        // -10
                        thd_sleep(200);
                        *(int *)menu[csel].field1 -= 10;
                        if (*(int *)menu[csel].field1 < min)
                            *(int *)menu[csel].field1 = min;
                    }
                    break;
                }
        }
        else
        {
            if (cstate->buttons & CONT_DPAD_UP)
            {
                while (cstate->buttons & CONT_DPAD_UP)
                    cstate = (cont_state_t *)maple_dev_status(cont);
                csel = (csel == 0) ? mlen - 1 : csel - 1;
            }
            if (cstate->buttons & CONT_DPAD_DOWN)
            {
                while (cstate->buttons & CONT_DPAD_DOWN)
                    cstate = (cont_state_t *)maple_dev_status(cont);
                csel = (csel == (mlen - 1)) ? 0 : csel + 1;
            }
        }

        vid_waitvbl();
        gui_PrePrint();
        fc = 0xFFFFFFFF;
        bc = 0xFF000000;

        switch (menu[csel].flags & 0x0FFFFFFF)
        {
            case GUI_MENU:
            gui_Print("A = Enter Sub-Menu", fc, bc, 12, 424);
            break;
            case GUI_FUNCTION:
            gui_Print("A = Do Function", fc, bc, 12, 424);
            break;
            case GUI_SELECT:
            gui_Print("A = Hold to Change Selection", fc, bc, 12, 424);
            break;
            case GUI_TOGGLE:
            gui_Print("A = Toggle Flag", fc, bc, 12, 424);
            break;
            case GUI_INTEGER:
            gui_Print("A = Hold to Change Integer", fc, bc, 12, 424);
            break;
            case GUI_FILE:
            gui_Print("A = Select File", fc, bc, 12, 424);
            break;
            case GUI_STRING:
            gui_Print("A = Input String", fc, bc, 12, 424);
            break;
        }
        sprintf(line, "B = %s", exit_msg);
        gui_Print(line, fc, bc, 628 - gui_PrintWidth(line), 424);

        for (i=0; i<mlen; i++)
        {
            char temp[10];

            bc = 0xFF000000;
            if ((menu[i].flags & 0x0FFFFFFF) == GUI_DIVIDER)
                continue;
            if (menu[i].enable == GUI_ENABLED)
                fc = (i==csel) ? 0xFFFFFFFF : 0xFF40FF20;
            else
                fc = 0xFFFF4020;

            strcpy(line, menu[i].text);
            switch (menu[i].flags & 0x0FFFFFFF)
            {
                case GUI_SELECT:
                strcat(line, " : ");
                strcat(line, ((struct gui_list *)menu[i].field1)[*(int *)menu[i].field2].text);
                break;
                case GUI_TOGGLE:
                strcat(line, " : ");
                strcat(line, *(int *)menu[i].field1 ? "on" : "off");
                break;
                case GUI_INTEGER:
                strcat(line, " : ");
                sprintf(temp, "%d", *(int *)menu[i].field1);
                strcat(line, temp);
                break;
                case GUI_FILE:
                strcat(line, " : ");
                strcat(line, *(int *)menu[i].field1 ? *(char **)menu[i].field1 : "");
                break;
                case GUI_STRING:
                strcat(line, " : ");
                strcat(line, *(int *)menu[i].field1 ? *(char **)menu[i].field1 : "");
                break;
                case GUI_TEXT:
                if ((int)menu[i].field1)
                    fc = (uint32)menu[i].field1 | 0xFF000000;
                if ((int)menu[i].field2)
                    bc = (uint32)menu[i].field2 | 0xFF000000;
                break;
            }
            switch (menu[i].flags & 0xF0000000)
            {
                case GUI_LEFT:
                tx = 12;
                ty = msy + i * 24;
                break;
                case GUI_RIGHT:
                tx = 628 - gui_PrintWidth(line);
                ty = msy + i * 24;
                break;
                case GUI_CENTER:
                default:
                tx = 320 - gui_PrintWidth(line) / 2;
                ty = msy + i * 24;
            }
            gui_Print(line, fc, bc, tx, ty);
            bc = 0xFF000000;
        }
        gui_PostPrint();

        cstate = (cont_state_t *)maple_dev_status(cont);
        if (quit_requested)
        {
            thd_sleep(5*1000);
            exit(0);
        }
    }
    while (cstate->buttons & CONT_B)
        cstate = (cont_state_t *)maple_dev_status(cont);

    vid_empty();
}

void set_myargv(void);
void get_myargv(void);

void dc_load_defaults()
{
    int i;
    FILE *handle;
    char temp[256];

    if (DC_LoadFromVMU("doom.set"))
        return;
    handle = fopen ("doom.set", "r");
    if (handle == NULL)
        return;

    for (i = 0 ; i < MAXARGVS; i++)
    {
        temp[0] = 0;
        fgets(temp, 255, handle);
        printf(" %d : %s", i, temp);
        if (temp[0] == 0)
            break;
        temp[strlen(temp) - 1] = 0;
        myargv[i] = strdup(temp);
    }
    myargc = i;

    fclose (handle);

    get_myargv();
}

void dc_load_presets(void *arg)
{
    int i;
    char *req;
    char temp[64];

    strcpy(temp, dc_home);
    strcat(temp, "presets");
    req = RequestFile(temp);

    vid_set_mode(guiVMode, PM_RGB565);
    vid_empty();
    bfont_set_32bit_mode(0);
    guiVMode32 = 0;
    bfont_set_background_color(MAKE_RGB565(0xFF000000));
    bfont_set_foreground_color(MAKE_RGB565(0xFFFFFFFF));
    DebugDC_Init(DM_640x480, PM_RGB565); // enable debug output to screen

    // free old argv entries
    if (myargc)
        for (i = 0 ; i < myargc; i++)
        {
            free(myargv[i]);
            myargv[i] = 0;
        }
    myargc = 0;

    if (req)
    {
        FILE *handle;

        printf("Attempting to load settings from %s\n", req);

        handle = fopen (req, "r");
        if (handle == NULL)
        {
            printf("Error! Couldn't open file %s\n\n", req);
            thd_sleep(2*1000);
            return;
        }

        for (i = 0 ; i < MAXARGVS; i++)
        {
            char temp[256];
            temp[0] = 0;
            fgets(temp, 255, handle);
            printf(" %d : %s", i, temp);
            if (temp[0] == 0)
                break;
            temp[strlen(temp) - 1] = 0;
            myargv[i] = strdup(temp);
        }
        myargc = i;

        fclose (handle);

        get_myargv();

        printf("\nSettings loaded\n\n");
        thd_sleep(3*1000);
    }
    else
    {
        printf("You need to enter a filename to load!\n\n");
        thd_sleep(2*1000);
    }

    // restore gui display mode
    vid_set_mode(guiVMode | DM_MULTIBUFFER, PM_RGB565);
    vid_empty();
    bfont_set_32bit_mode(0);
    guiVMode32 = 0;
}

void dc_load_settings(void *arg)
{
    int ok, i;
    char filename[32] = { 'd', 'o', 'o', 'm', '.', 's', 'e', 't', 0 }; // text already in the edit box on start
    char *desc = "Enter Load Name";

    ok = get_text_osk(filename, desc);

    vid_set_mode(guiVMode, PM_RGB565);
    vid_empty();
    bfont_set_32bit_mode(0);
    guiVMode32 = 0;
    bfont_set_background_color(MAKE_RGB565(0xFF000000));
    bfont_set_foreground_color(MAKE_RGB565(0xFFFFFFFF));
    DebugDC_Init(DM_640x480, PM_RGB565); // enable debug output to screen

    // free old argv entries
    if (myargc)
        for (i = 0 ; i < myargc; i++)
        {
            free(myargv[i]);
            myargv[i] = 0;
        }
    myargc = 0;

    if (ok)
    {
        FILE *handle;

        printf("Attempting to load settings from %s\n\n", filename);

        if (DC_LoadFromVMU(filename))
        {
            printf("Error! Couldn't read file %s from VMU\n\n", filename);
            thd_sleep(2*1000);

            // restore gui display mode
            vid_set_mode(guiVMode | DM_MULTIBUFFER, PM_RGB565);
            vid_empty();
            bfont_set_32bit_mode(0);
            guiVMode32 = 0;
            return;
        }
        handle = fopen (filename, "r");
        if (handle == NULL)
        {
            printf("Error! Couldn't open file %s\n\n", filename);
            thd_sleep(2*1000);

            // restore gui display mode
            vid_set_mode(guiVMode | DM_MULTIBUFFER, PM_RGB565);
            vid_empty();
            bfont_set_32bit_mode(0);
            guiVMode32 = 0;
            return;
        }

        for (i = 0 ; i < MAXARGVS; i++)
        {
            char temp[256];
            temp[0] = 0;
            fgets(temp, 255, handle);
            printf(" %d : %s", i, temp);
            if (temp[0] == 0)
                break;
            temp[strlen(temp) - 1] = 0;
            myargv[i] = strdup(temp);
        }
        myargc = i;

        fclose (handle);

        get_myargv();

        printf("\nSettings loaded\n\n");
        thd_sleep(3*1000);
    }
    else
    {
        printf("You need to enter a filename to load!\n\n");
        thd_sleep(2*1000);
    }

    // restore gui display mode
    vid_set_mode(guiVMode | DM_MULTIBUFFER, PM_RGB565);
    vid_empty();
    bfont_set_32bit_mode(0);
    guiVMode32 = 0;
}

void dc_save_settings(void *arg)
{
    int ok, i;
    char filename[32] = { 'd', 'o', 'o', 'm', '.', 's', 'e', 't', 0 }; // text already in the edit box on start
    char *desc = "Enter Save Name";

    ok = get_text_osk(filename, desc);

    vid_set_mode(guiVMode, PM_RGB565);
    vid_empty();
    bfont_set_32bit_mode(0);
    guiVMode32 = 0;
    bfont_set_background_color(MAKE_RGB565(0xFF000000));
    bfont_set_foreground_color(MAKE_RGB565(0xFFFFFFFF));
    DebugDC_Init(DM_640x480, PM_RGB565); // enable debug output to screen

    if (ok)
    {
        FILE *handle;

        printf("Attempting to save settings to %s\n\n", filename);

        handle = fopen (filename, "wb");
        if (handle == NULL)
        {
            printf("Error! Couldn't open file %s\n\n", filename);
            thd_sleep(2*1000);

            // restore gui display mode
            vid_set_mode(guiVMode | DM_MULTIBUFFER, PM_RGB565);
            vid_empty();
            bfont_set_32bit_mode(0);
            guiVMode32 = 0;
            return;
        }

        set_myargv();
        for (i = 0 ; i < myargc; i++)
        {
            fwrite (myargv[i], 1, strlen(myargv[i]), handle);
            fwrite ("\n", 1, 1, handle);
        }

        fclose (handle);

        if (DC_SaveToVMU(filename, 2))
            printf("Error! Couldn't write file %s to VMU\n\n", filename);
        else
            printf("Settings saved to %s\n\n", filename);
        thd_sleep(2*1000);
    }
    else
    {
        printf("You need to enter a filename to save!\n\n");
        thd_sleep(2*1000);
    }

    // restore gui display mode
    vid_set_mode(guiVMode | DM_MULTIBUFFER, PM_RGB565);
    vid_empty();
    bfont_set_32bit_mode(0);
    guiVMode32 = 0;
}

static void drawLine(int inX0, int inY0, int inX1, int inY1, uint32 inColor, uint32* inDestination, int inWidth)
{
     int tempDY = inY1 - inY0;
     int tempDX = inX1 - inX0;
     int tempStepX, tempStepY;

     if(tempDY < 0) {
          tempDY = -tempDY;  tempStepY = -inWidth;
     } else {
          tempStepY = inWidth;
     }

     if(tempDX < 0) {
          tempDX = -tempDX;  tempStepX = -1;
     } else {
          tempStepX = 1;
     }

     tempDY <<= 1;
     tempDX <<= 1;

     inY0 *= inWidth;
     inY1 *= inWidth;
     inDestination[inX0 + inY0] = inColor;
     if(tempDX > tempDY) {
          int tempFraction = tempDY - (tempDX >> 1);
          while(inX0 != inX1) {
               if(tempFraction >= 0) {
                    inY0 += tempStepY;
                    tempFraction -= tempDX;
               }
               inX0 += tempStepX;
               tempFraction += tempDY;
               inDestination[inX0 + inY0] = inColor;
          }
     } else {
          int tempFraction = tempDX - (tempDY >> 1);
          while(inY0 != inY1) {
               if(tempFraction >= 0) {
                    inX0 += tempStepX;
                    tempFraction -= tempDY;
               }
               inY0 += tempStepY;
               tempFraction += tempDX;
               inDestination[inX0 + inY0] = inColor;
          }
     }
}

void dc_tv_center(void *arg)
{
    maple_device_t *cont = NULL;
    cont_state_t *cstate;
    int cx, cy, mx, my, h, w;

    while (cont == NULL)
        cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);

    if (video_cable != CT_VGA)
    {
        switch (dc_tv_res)
        {
            case 1:
            w = 608;
            h = 448;
            mx = 640 - w;
            my = 480 - h;
            vid_set_mode((guiVMode == DM_640x480_PAL_IL) ? DM_640x480_PAL_IL_MB : DM_640x480_NTSC_IL_MB, PM_RGB888);
            break;
            case 2:
            w = 576;
            h = 448;
            mx = 768 - w;
            my = 480 - h;
            vid_set_mode((guiVMode == DM_640x480_PAL_IL) ? DM_768x480_PAL_IL_MB : DM_768x480_NTSC_IL_MB, PM_RGB888);
            break;
            case 0:
            default:
            w = 304;
            h = 224;
            mx = 320 - w;
            my = 240 - h;
            vid_set_mode((guiVMode == DM_640x480_PAL_IL) ? DM_320x240_PAL_MB : DM_320x240_NTSC_MB, PM_RGB888);
            break;
        }
        cx = dc_tv_cx > mx ? mx : dc_tv_cx;
        cy = dc_tv_cy > my ? my : dc_tv_cy;

        cstate = (cont_state_t *)maple_dev_status(cont);
        while (!(cstate->buttons & (CONT_A | CONT_B)))
        {
            if (cstate->buttons & CONT_DPAD_UP)
                cy = cy > 0 ? cy-1 : 0;
            if (cstate->buttons & CONT_DPAD_DOWN)
                cy = cy < my ? cy+1 : my;
            if (cstate->buttons & CONT_DPAD_LEFT)
                cx = cx > 0 ? cx-1 : 0;
            if (cstate->buttons & CONT_DPAD_RIGHT)
                cx = cx < mx ? cx+1 : mx;

            vid_waitvbl();
            vid_clear(0, 0, 0); // clear draw buffer

            drawLine(cx, cy, cx+w-1, cy, 0xFFFFFF, vram_l, mx+w);
            drawLine(cx+w-1, cy, cx+w-1, cy+h-1, 0xFFFFFF, vram_l, mx+w);
            drawLine(cx+w-1, cy+h-1, cx, cy+h-1, 0xFFFFFF, vram_l, mx+w);
            drawLine(cx, cy+h-1, cx, cy, 0xFFFFFF, vram_l, mx+w);
            drawLine(cx, cy, cx+w-1, cy+h-1, 0xFFFFFF, vram_l, mx+w);
            drawLine(cx, cy+h-1, cx+w-1, cy, 0xFFFFFF, vram_l, mx+w);

            vid_waitvbl();
            vid_flip(-1);

            cstate = (cont_state_t *)maple_dev_status(cont);
        }
        while (cstate->buttons & (CONT_A | CONT_B))
            cstate = (cont_state_t *)maple_dev_status(cont);

        dc_tv_cx = cx;
        dc_tv_cy = cy;

        vid_set_mode(guiVMode | DM_MULTIBUFFER, PM_RGB565);
        vid_empty();
        bfont_set_32bit_mode(0);
        guiVMode32 = 0;
    }
}

void dc_stick_calibrate(void *arg)
{
    maple_device_t *cont = NULL;
    cont_state_t *cstate;
    int cx, cy, mx, my, Mx, My;
    uint32 *addr;
    char temp[8];

    while (cont == NULL)
        cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);

    cx = cy = mx = my = Mx = My = 128;

    vid_set_mode(guiVMode | DM_MULTIBUFFER, PM_RGB888);
    vid_empty();
    bfont_set_32bit_mode(1);
    guiVMode32 = 1;
    bfont_set_background_color(0xFF000000);
    bfont_set_foreground_color(0xFFFFFFFF);

    cstate = (cont_state_t *)maple_dev_status(cont);
    while (!(cstate->buttons & (CONT_A | CONT_B)))
    {
        cstate = (cont_state_t *)maple_dev_status(cont);
        cx = cstate->joyx + 128;
        cy = cstate->joyy + 128;
        if (cx<mx) mx = cx;
        if (cx>Mx) Mx = cx;
        if (cy<my) my = cy;
        if (cy>My) My = cy;

        vid_waitvbl();
        vid_clear(0, 0, 0); // clear draw buffer

        drawLine(192, 112, 192+255, 112, 0xFFFFFF, vram_l, 640);
        drawLine(192+255, 112, 192+255, 112+255, 0xFFFFFF, vram_l, 640);
        drawLine(192+255, 112+255, 192, 112+255, 0xFFFFFF, vram_l, 640);
        drawLine(192, 112+255, 192, 112, 0xFFFFFF, vram_l, 640);
        addr = (uint32 *)((int)vram_l + (192+cx)*4 +(112+cy)*640*4);
        *addr = (uint32)0xFFFFFF;

        bfont_draw_str((uint16 *)((uint32)vram_l + (12 + 4 * 24 * 640) * 4), 640, 1, "Current");
        sprintf(temp, " %02X %02X", cx, cy);
        bfont_draw_str((uint16 *)((uint32)vram_l + (12 + 5 * 24 * 640) * 4), 640, 1, temp);
        bfont_draw_str((uint16 *)((uint32)vram_l + (12 + 7 * 24 * 640) * 4), 640, 1, "Minimum");
        sprintf(temp, " %02X %02X", mx, my);
        bfont_draw_str((uint16 *)((uint32)vram_l + (12 + 8 * 24 * 640) * 4), 640, 1, temp);
        bfont_draw_str((uint16 *)((uint32)vram_l + (12 + 10 * 24 * 640) * 4), 640, 1, "Maximum");
        sprintf(temp, " %02X %02X", Mx, My);
        bfont_draw_str((uint16 *)((uint32)vram_l + (12 + 11 * 24 * 640) * 4), 640, 1, temp);

        vid_waitvbl();
        vid_flip(-1);
    }
    while (cstate->buttons & (CONT_A | CONT_B))
        cstate = (cont_state_t *)maple_dev_status(cont);

    // restore gui display mode
    vid_set_mode(guiVMode | DM_MULTIBUFFER, PM_RGB565);
    vid_empty();
    bfont_set_32bit_mode(0);
    guiVMode32 = 0;

    dc_stick_cx = cx;
    dc_stick_cy = cy;
    dc_stick_minx = mx;
    dc_stick_miny = my;
    dc_stick_maxx = Mx;
    dc_stick_maxy = My;
}

void InfraFunc(struct gui_menu *menu)
{

}

void dc_gui(void)
{
    struct gui_menu AboutLevel[] = {
        { "Doom for Dreamcast v1.4", GUI_CENTER | GUI_TEXT, (void *)0xFFFFFF, (void *)0x1020FF, GUI_DISABLED },
        { "by Chilly Willy", GUI_CENTER | GUI_TEXT, (void *)0xFFFFFF, (void *)0x1020FF, GUI_DISABLED },
        { "based on Doom for PSP v1.4 by Chilly Willy", GUI_CENTER | GUI_TEXT, (void *)0x88FF88, 0, GUI_DISABLED },
        { "based on ADoomPPC v1.7 by Jarmo Laakkonen", GUI_CENTER | GUI_TEXT, (void *)0x88FF88, 0, GUI_DISABLED },
        { "based on ADoomPPC v1.3 by Joseph Fenton", GUI_CENTER | GUI_TEXT, (void *)0x88FF88, 0, GUI_DISABLED },
        { "based on ADoom v1.2 by Peter McGavin", GUI_CENTER | GUI_TEXT, (void *)0x88FF88, 0, GUI_DISABLED },
        { "", GUI_CENTER | GUI_DIVIDER, 0, 0, GUI_DISABLED },
        { "Danzeff OSK by Danzel and Jeff Chen", GUI_CENTER | GUI_TEXT, (void *)0x8888FF, 0, GUI_DISABLED },
        { "KallistiOS version by Chilly Willy", GUI_CENTER | GUI_TEXT, (void *)0x8888FF, 0, GUI_DISABLED },
        { "", GUI_CENTER | GUI_DIVIDER, 0, 0, GUI_DISABLED },
        { "Mus2Midi by Ben Ryves", GUI_CENTER | GUI_TEXT, (void *)0xFF8888, 0, GUI_DISABLED },
        { "WildMidi by Chris Ison", GUI_CENTER | GUI_TEXT, (void *)0xFF8888, 0, GUI_DISABLED },
        { 0, GUI_END_OF_MENU, 0, 0, 0 } // end of menu
    };

    int net_ticdup_range[2] = { 1, 9 };
    int net_player1_range[2] = { 1, 4 };
    int net_port_range[2] = { 5000, 65535 };

    struct gui_menu InfraLevel[] = {
        { "Network Port", GUI_CENTER | GUI_INTEGER, &dc_net_port, &net_port_range, GUI_ENABLED },
        { "", GUI_CENTER | GUI_DIVIDER, 0, 0, GUI_DISABLED },
        { "Network Player #1 Addr", GUI_LEFT | GUI_STRING, &dc_net_player2, 0, GUI_ENABLED },
        { "Network Player #2 Addr", GUI_LEFT | GUI_STRING, &dc_net_player3, 0, GUI_ENABLED },
        { "Network Player #3 Addr", GUI_LEFT | GUI_STRING, &dc_net_player4, 0, GUI_ENABLED },
        { "Local Network Address ", GUI_LEFT | GUI_STRING, &dc_net_ipaddr, 0, GUI_DISABLED },
        { 0, GUI_END_OF_MENU, 0, 0, 0 } // end of menu
    };

    int game_level_range[2] = { 1, 36 };

    struct gui_list game_skill_list[] = {
        { "I'm Too Young To Die", 0 },
        { "Hey, Not Too Rough", 1 },
        { "Hurt Me Plenty", 2 },
        { "Ultra-Violence", 3 },
        { "Nightmare!", 4 },
        { 0, GUI_END_OF_LIST }
    };

    struct gui_menu NetLevel[] = {
        { "Play Network Game", GUI_CENTER | GUI_TOGGLE, &dc_net_enabled, 0, GUI_ENABLED },
        { "Player Number",  GUI_CENTER | GUI_INTEGER, &dc_net_player1, &net_player1_range, GUI_ENABLED },
        { "Deathmatch", GUI_CENTER | GUI_TOGGLE, &dc_game_deathmatch, 0, GUI_ENABLED },
        { "Alt Deathmatch", GUI_CENTER | GUI_TOGGLE, &dc_game_altdeath, 0, GUI_ENABLED },
        { "Starting Skill Level", GUI_CENTER | GUI_SELECT, &game_skill_list, &dc_game_skill, GUI_ENABLED },
        { "Starting Map Level", GUI_CENTER | GUI_INTEGER, &dc_game_level, &game_level_range, GUI_ENABLED },
        { "Timed Game", GUI_CENTER | GUI_INTEGER, &dc_game_timer, 0, GUI_ENABLED },
        { "A.V.G.", GUI_CENTER | GUI_TOGGLE, &dc_game_avg, 0, GUI_ENABLED },
        { "Copy Stats", GUI_CENTER | GUI_TOGGLE, &dc_game_statcopy, 0, GUI_ENABLED },
        { "Force Version", GUI_CENTER | GUI_INTEGER, &dc_game_version, 0, GUI_ENABLED },
        { "Use PC Checksum", GUI_CENTER | GUI_TOGGLE, &dc_game_pcchksum, 0, GUI_ENABLED },
        { "Network Type Settings", GUI_CENTER | GUI_MENU, &InfraLevel, &InfraFunc, GUI_ENABLED },
        { "Network Extra Tic", GUI_CENTER | GUI_TOGGLE, &dc_net_extratic, 0, GUI_ENABLED },
        { "Network Tic Dup", GUI_CENTER | GUI_INTEGER, &dc_net_ticdup, &net_ticdup_range, GUI_ENABLED },
        { 0, GUI_END_OF_MENU, 0, 0, 0 } // end of menu
    };

    struct gui_menu GameLevel[] = {
        { "No Monsters", GUI_CENTER | GUI_TOGGLE, &dc_game_nomonsters, 0, GUI_ENABLED },
        { "Respawn", GUI_CENTER | GUI_TOGGLE, &dc_game_respawn, 0, GUI_ENABLED },
        { "Fast", GUI_CENTER | GUI_TOGGLE, &dc_game_fast, 0, GUI_ENABLED },
        { "Turbo", GUI_CENTER | GUI_TOGGLE, &dc_game_turbo, 0, GUI_ENABLED },
        { "Map on HU", GUI_CENTER | GUI_TOGGLE, &dc_game_maponhu, 0, GUI_ENABLED },
        { "Rotate Map", GUI_CENTER | GUI_TOGGLE, &dc_game_rotatemap, 0, GUI_ENABLED },
        { "Force Demo", GUI_CENTER | GUI_TOGGLE, &dc_game_forcedemo, 0, GUI_ENABLED },
        { "Play Demo", GUI_CENTER | GUI_INTEGER, &dc_game_playdemo, 0, GUI_ENABLED },
        { "Time Demo", GUI_CENTER | GUI_INTEGER, &dc_game_timedemo, 0, GUI_ENABLED },
        { "Record Demo", GUI_CENTER | GUI_INTEGER, &dc_game_record, 0, GUI_ENABLED },
        //{ "Warp to Level", GUI_CENTER | GUI_SELECT, &game_warp_list, &dc_game_warp, GUI_ENABLED },
        { 0, GUI_END_OF_MENU, 0, 0, 0 } // end of menu
    };

    struct gui_menu FileLevel[] = {
        { "Main WAD", GUI_LEFT | GUI_FILE, &dc_iwad_file, "iwad", GUI_ENABLED },
        { "Patch WAD", GUI_LEFT | GUI_FILE, &dc_pwad_file1, "pwad", GUI_ENABLED },
        { "Patch WAD", GUI_LEFT | GUI_FILE, &dc_pwad_file2, "pwad", GUI_ENABLED },
        { "Patch WAD", GUI_LEFT | GUI_FILE, &dc_pwad_file3, "pwad", GUI_ENABLED },
        { "Patch WAD", GUI_LEFT | GUI_FILE, &dc_pwad_file4, "pwad", GUI_ENABLED },
        { "DEH File", GUI_LEFT | GUI_FILE, &dc_deh_file1, "deh", GUI_ENABLED },
        { "DEH File", GUI_LEFT | GUI_FILE, &dc_deh_file2, "deh", GUI_ENABLED },
        { "DEH File", GUI_LEFT | GUI_FILE, &dc_deh_file3, "deh", GUI_ENABLED },
        { "DEH File", GUI_LEFT | GUI_FILE, &dc_deh_file4, "deh", GUI_ENABLED },
        { 0, GUI_END_OF_MENU, 0, 0, 0 } // end of menu
    };

    struct gui_list ctrl_cheat_list[] = {
        { "none", 0 },
        { "God Mode", 1 },
        { "Fucking Arsenal", 2 },
        { "Key Full Ammo", 3 },
        { "No Clipping", 4 },
        { "Toggle Map", 5 },
        { "Invincible with Chainsaw", 6 },
        { "Berserker Strength Power-up", 7 },
        { "Invincibility Power-up", 8 },
        { "Invisibility Power-Up", 9 },
        { "Automap Power-up", 10 },
        { "Anti-Radiation Suit Power-up", 11 },
        { "Light-Amplification Visor Power-up", 12 },
        { 0, GUI_END_OF_LIST }
    };

    struct gui_menu ControlLevel[] = {
        { "Jump Pack", GUI_CENTER | GUI_TOGGLE, &dc_jump_enable, 0, GUI_SET_ME },
        { "Disable Analog Stick", GUI_CENTER | GUI_TOGGLE, &dc_stick_disabled, 0, GUI_ENABLED },
        { "Calibrate Analog Stick", GUI_CENTER | GUI_FUNCTION, &dc_stick_calibrate, 0, GUI_ENABLED },
        { "Y + B Cheat", GUI_CENTER | GUI_SELECT, &ctrl_cheat_list, &dc_ctrl_cheat1, GUI_ENABLED },
        { "Y + A Cheat", GUI_CENTER | GUI_SELECT, &ctrl_cheat_list, &dc_ctrl_cheat2, GUI_ENABLED },
        { "Y + X Cheat", GUI_CENTER | GUI_SELECT, &ctrl_cheat_list, &dc_ctrl_cheat3, GUI_ENABLED },
        { 0, GUI_END_OF_MENU, 0, 0, 0 } // end of menu
    };

    struct gui_menu SoundLevel[] = {
        { "Sound Effects", GUI_CENTER | GUI_TOGGLE, &dc_sfx_enabled, 0, GUI_ENABLED },
        { "Music", GUI_CENTER | GUI_TOGGLE, &dc_music_enabled, 0, GUI_SET_ME },
        { 0, GUI_END_OF_MENU, 0, 0, 0 } // end of menu
    };

    struct gui_list tv_res_list[] = {
        { "304x224", 0 },
        { "608x448", 1 },
        { "576x448 (16:9)", 2 },
        { 0, GUI_END_OF_LIST }
    };

    struct gui_list dc_cable_list[] = {
        { "VGA", 0 },
        { "Unknown", 1 },
        { "RGB", 2 },
        { "Composite", 3 },
        { 0, GUI_END_OF_LIST }
    };

    struct gui_list dc_detail_list[] = {
        { "High", 0 },
        { "Low", 1 },
        { 0, GUI_END_OF_LIST }
    };

    struct gui_menu TvLevel[] = {
        { "Cable", GUI_CENTER | GUI_SELECT, &dc_cable_list, &video_cable, GUI_DISABLED },
        { "Resolution", GUI_CENTER | GUI_SELECT, &tv_res_list, &dc_tv_res, GUI_ENABLED },
        { "Sync to VBlank", GUI_CENTER | GUI_TOGGLE, &dc_tv_sync, 0, GUI_ENABLED },
        { "Show FPS", GUI_CENTER | GUI_TOGGLE, &dc_vid_fps, 0, GUI_ENABLED },
        { "Detail", GUI_CENTER | GUI_SELECT, &dc_detail_list, &dc_tv_detail, GUI_ENABLED },
        { "Center Screen", GUI_CENTER | GUI_FUNCTION, &dc_tv_center, 0, GUI_ENABLED },
        { 0, GUI_END_OF_MENU, 0, 0, 0 } // end of menu
    };

    struct gui_list mon_res_list[] = {
        { "320x240", 0 },
        { "640x480", 1 },
        //{ "800x608", 2 },
        { 0, GUI_END_OF_LIST }
    };

    struct gui_menu MonLevel[] = {
        { "Cable", GUI_CENTER | GUI_SELECT, &dc_cable_list, &video_cable, GUI_DISABLED },
        { "Resolution", GUI_CENTER | GUI_SELECT, &mon_res_list, &dc_mon_res, GUI_ENABLED },
        { "Sync to VBlank", GUI_CENTER | GUI_TOGGLE, &dc_mon_sync, 0, GUI_ENABLED },
        { "Show FPS", GUI_CENTER | GUI_TOGGLE, &dc_vid_fps, 0, GUI_ENABLED },
        { "Detail", GUI_CENTER | GUI_SELECT, &dc_detail_list, &dc_mon_detail, GUI_ENABLED },
        { 0, GUI_END_OF_MENU, 0, 0, 0 } // end of menu
    };

    struct gui_list dc_display_list[] = {
        { "Use Monitor", 0 },
        { "Use TV", 1 },
        { 0, GUI_END_OF_LIST }
    };

    struct gui_menu VideoLevel[] = {
        { "Monitor Settings", GUI_CENTER | GUI_MENU, MonLevel, 0, GUI_SET_ME },
        { "TV Settings", GUI_CENTER | GUI_MENU, TvLevel, 0, GUI_SET_ME },
        { "Display", GUI_CENTER | GUI_SELECT, &dc_display_list, &dc_use_tv, GUI_DISABLED },
        { 0, GUI_END_OF_MENU, 0, 0, 0 } // end of menu
    };

    struct gui_menu TopLevel[] = {
        { "Load Settings", GUI_CENTER | GUI_FUNCTION, &dc_load_settings, 0, GUI_ENABLED },
        { "Save Settings", GUI_CENTER | GUI_FUNCTION, &dc_save_settings, 0, GUI_ENABLED },
        { "Load Game Presets", GUI_CENTER | GUI_FUNCTION, &dc_load_presets, 0, GUI_ENABLED },
        { "Video Settings", GUI_CENTER | GUI_MENU, VideoLevel, 0, GUI_ENABLED },
        { "Sound Settings", GUI_CENTER | GUI_MENU, SoundLevel, 0, GUI_ENABLED },
        { "Controller Settings", GUI_CENTER | GUI_MENU, ControlLevel, 0, GUI_ENABLED },
        { "File Settings", GUI_CENTER | GUI_MENU, FileLevel, 0, GUI_ENABLED },
        { "Game Settings", GUI_CENTER | GUI_MENU, GameLevel, 0, GUI_ENABLED },
        { "Network Settings", GUI_CENTER | GUI_MENU, NetLevel, 0, GUI_SET_ME },
        { "About", GUI_CENTER | GUI_MENU, AboutLevel, 0, GUI_ENABLED },
        { 0, GUI_END_OF_MENU, 0, 0, 0 } // end of menu
    };

    FILE *temp;
    char str[256];
    maple_device_t * jump;

    // start by setting some of the enables that depend on certain variables
    VideoLevel[0].enable = (video_cable == CT_VGA) ? GUI_ENABLED : GUI_DISABLED;
    VideoLevel[1].enable = (video_cable != CT_VGA) ? GUI_ENABLED : GUI_DISABLED;
    dc_use_tv = (video_cable != CT_VGA) ? 1 : 0;
    sprintf(str,"%s%s",dc_home,"timidity/timidity.cfg");
    temp = fopen(str, "rb");
    if (temp)
    {
        SoundLevel[1].enable = GUI_ENABLED;
        fclose(temp);
    }
    else
    {
        dc_music_enabled = 0;
        SoundLevel[1].enable = GUI_DISABLED;
    }
    TopLevel[8].enable = dc_net_available ? GUI_ENABLED : GUI_DISABLED;
    jump = maple_enum_type(0, MAPLE_FUNC_PURUPURU);
    ControlLevel[0].enable = jump ? GUI_ENABLED : GUI_DISABLED;
    if (!jump)
        dc_jump_enable = 0;

    do_gui(TopLevel, (void *)0, "Start DOOM");

    // now set the arg list
    set_myargv();
}

// Network support code

void dc_net_init (void)
{
    netif_t *netdev = net_default_dev;

    if (netdev && netdev->ip_addr[0]) {
        dc_net_available = 1;
        dc_net_error = 0;
        sprintf(dc_ipaddr, "%u.%u.%u.%u", netdev->ip_addr[0], netdev->ip_addr[1], netdev->ip_addr[2], netdev->ip_addr[3]);
        dc_net_ipaddr = dc_ipaddr;
        dc_net_dnssrv.sin_family = AF_INET;
        dc_net_dnssrv.sin_port = htons(53);
        dc_net_dnssrv.sin_addr.s_addr = htonl(netdev->dns[0]|(netdev->dns[1]<<8)|(netdev->dns[2]<<16)|(netdev->dns[3]<<24));
    } else {
        dc_net_available = 0;
        dc_net_error = 1;
    }
}

#if 0
extern unsigned long end;

static uint32 memFreeSpace(void) {
    uint32 base = (int)(&end);
    base = (base/4)*4 + 4;      /* longword align */
    uint32 space = (0x8d000000 - 65536) - base;
    return space;
}
#endif

int pal_menu(void)
{
    maple_device_t *cont1;
    cont_state_t *state;

    vid_set_mode(DM_640x480_PAL_IL, PM_RGB565);

    bfont_draw_str(vram_s + 640 * 200 + 64, 640, 1, "Press A to run at 60Hz");
    bfont_draw_str(vram_s + 640 * 240 + 64, 640, 1, "or B to run at 50Hz");

    cont1 = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);

    while(1)
    {
        if(cont1)
        {
            state = (cont_state_t *)maple_dev_status(cont1);
            if(state)
            {
                if(state->buttons & CONT_A)
                    return 1;
                else if(state->buttons & CONT_B)
                    return 0;
            }
        }
        else
            cont1 = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
    }
}

int main (int argc, char **argv)
{
    int i, p, dc_region;

    dc_font_init();

    dc_region = flashrom_get_region();
    video_cable = vid_check_cable();
    /* Prompt the user for whether to run in PAL50 or PAL60 if the flashrom says
       the Dreamcast is European and a VGA Box is not hooked up. */
    if(dc_region == FLASHROM_REGION_EUROPE && video_cable != CT_VGA)
    {
        if(pal_menu())
            guiVMode = DM_640x480_NTSC_IL;
        else
            guiVMode = DM_640x480_PAL_IL;
    }
    else
        guiVMode = DM_640x480;

#if 0
    {
        vid_set_mode(guiVMode, PM_RGB565);
        vid_empty();
        bfont_set_32bit_mode(0);
        guiVMode32 = 0;
        DebugDC_Init(DM_640x480, PM_RGB565);
        // let's see how much memory we have...
        printf(temp, "Available MEM = %d KB\n\n", memFreeSpace()>>10);
    }
#endif

    vid_set_mode(guiVMode | DM_MULTIBUFFER, PM_RGB565);
    vid_empty();
    bfont_set_32bit_mode(0);
    guiVMode32 = 0;
    DebugDC_Init(-1, -1); // disable debug output to screen

    // use cd for our home
    strcpy(dc_home, "/cd/");

    // set current working directory to the ram disk (needed for VMU and music file handling)
    fs_chdir("/ram");

    dc_net_init();

    if ((myargv = malloc(sizeof(char *) * MAXARGVS)) == NULL)
    {
        printf("malloc(%d) failed", sizeof(char *) * MAXARGVS);
        thd_sleep(5*1000);
        return 0;
    }
    memset (myargv, 0, sizeof(char *)*MAXARGVS);
    myargc = 0;

    dc_load_defaults();

    // check if shareware WAD file is available and set as default
    if (!dc_iwad_file)
    {
        char temp[256];
        FILE *hnd;

        strcpy(temp, dc_home);
        strcat(temp, "iwad/doom1.wad");
        hnd = fopen(temp, "rb");
        if (hnd)
        {
            fclose(hnd);
            dc_iwad_file = strdup(temp);
        }
        else
        {
            strcpy(temp, dc_home);
            strcat(temp, "iwad/DOOM1.WAD");
            hnd = fopen(temp, "rb");
            if (hnd)
            {
                fclose(hnd);
                dc_iwad_file = strdup(temp);
            }
        }
    }

    dc_gui();

    vid_set_mode(guiVMode, PM_RGB565);
    vid_empty();
    bfont_set_32bit_mode(0);
    guiVMode32 = 0;
    bfont_set_background_color(MAKE_RGB565(0xFF000000));
    bfont_set_foreground_color(MAKE_RGB565(0xFFFFFFFF));
    DebugDC_Init(DM_640x480, PM_RGB565); // enable debug output to screen

    printf ("\n\n DOOM v%d.%d for Dreamcast\n\n", VERS, REVS);
    printf (" Args passed to D_DoomMain() are:\n");
    for (i = 1 ; i < myargc; i++)
        printf (" %s", myargv[i]);
    printf ("\n\n");
    thd_sleep(2*1000);

    p = M_CheckParm ("-forceversion");
    if (p && p < myargc - 1)
        VERSION = atoi (myargv[p+1]);

    D_DoomMain ();

    thd_sleep(5*1000);
    return 0;
}

int isIWADDoom2 (void)
{
    int         p;
    char wadname[256];

    if (!dc_iwad_file)
        return 1;

    for (p=strlen(dc_iwad_file)-1; p>0; p--)
        if (dc_iwad_file[p] == '/') break;
    strcpy(wadname, &dc_iwad_file[p+1]);

    if (!strcasecmp(wadname, "doom2f.wad"))
        return 1;

    if (!strcasecmp(wadname, "doom2.wad"))
        return 1;

    if (!strcasecmp(wadname, "plutonia.wad"))
        return 1;

    if (!strcasecmp(wadname, "tnt.wad"))
        return 1;

    return 0;
}

void set_myargv(void)
{
    char temp[256];
    int i;

    // free old argv entries
    if (myargc)
        for (i = 0 ; i < myargc; i++)
        {
            free(myargv[i]);
            myargv[i] = 0;
        }

    myargv[0] = strdup("Doom");
    myargc = 1;

    if (dc_vid_fps)
    {
        myargv[myargc] = strdup("-fps");
        myargc++;
    }

    if (dc_use_tv)
    {
        myargv[myargc] = strdup("-tv");
        myargc++;

        switch (dc_tv_res)
        {
            case 0:
            myargv[myargc] = strdup("-width");
            myargc++;
            myargv[myargc] = strdup("304");
            myargc++;
            myargv[myargc] = strdup("-height");
            myargc++;
            myargv[myargc] = strdup("224");
            myargc++;
            break;
            case 1:
            myargv[myargc] = strdup("-width");
            myargc++;
            myargv[myargc] = strdup("608");
            myargc++;
            myargv[myargc] = strdup("-height");
            myargc++;
            myargv[myargc] = strdup("448");
            myargc++;
            break;
            case 2:
            myargv[myargc] = strdup("-width");
            myargc++;
            myargv[myargc] = strdup("576");
            myargc++;
            myargv[myargc] = strdup("-height");
            myargc++;
            myargv[myargc] = strdup("448");
            myargc++;
            break;
        }

        if (dc_tv_sync)
        {
            myargv[myargc] = strdup("-vsync");
            myargc++;
        }

        if (dc_tv_detail)
        {
            myargv[myargc] = strdup("-lowdetail");
            myargc++;
        }

        myargv[myargc] = strdup("-tvcx");
        myargc++;
        sprintf(temp, "%d", dc_tv_cx);
        myargv[myargc] = strdup(temp);
        myargc++;
        myargv[myargc] = strdup("-tvcy");
        myargc++;
        sprintf(temp, "%d", dc_tv_cy);
        myargv[myargc] = strdup(temp);
        myargc++;
    }
    else
    {
        switch (dc_mon_res)
        {
            case 0:
            myargv[myargc] = strdup("-width");
            myargc++;
            myargv[myargc] = strdup("320");
            myargc++;
            myargv[myargc] = strdup("-height");
            myargc++;
            myargv[myargc] = strdup("240");
            myargc++;
            break;
            case 1:
            myargv[myargc] = strdup("-width");
            myargc++;
            myargv[myargc] = strdup("640");
            myargc++;
            myargv[myargc] = strdup("-height");
            myargc++;
            myargv[myargc] = strdup("480");
            myargc++;
            break;
            case 2:
            myargv[myargc] = strdup("-width");
            myargc++;
            myargv[myargc] = strdup("800");
            myargc++;
            myargv[myargc] = strdup("-height");
            myargc++;
            myargv[myargc] = strdup("608");
            myargc++;
            break;
        }

        if (dc_mon_sync)
        {
            myargv[myargc] = strdup("-vsync");
            myargc++;
        }

        if (dc_mon_detail)
        {
            myargv[myargc] = strdup("-lowdetail");
            myargc++;
        }
    }

    if (!dc_sfx_enabled)
    {
        myargv[myargc] = strdup("-nosfx");
        myargc++;
    }

    if (dc_music_enabled)
    {
        myargv[myargc] = strdup("-music");
        myargc++;
    }

    if (dc_iwad_file)
    {
        myargv[myargc] = strdup("-iwad");
        myargc++;
        myargv[myargc] = strdup(dc_iwad_file);
        myargc++;
    }

    if (dc_pwad_file1 || dc_pwad_file2 || dc_pwad_file3 || dc_pwad_file4)
    {
        myargv[myargc] = strdup("-file");
        myargc++;
        if (dc_pwad_file1)
        {
            myargv[myargc] = strdup(dc_pwad_file1);
            myargc++;
        }
        if (dc_pwad_file2)
        {
            myargv[myargc] = strdup(dc_pwad_file2);
            myargc++;
        }
        if (dc_pwad_file3)
        {
            myargv[myargc] = strdup(dc_pwad_file3);
            myargc++;
        }
        if (dc_pwad_file4)
        {
            myargv[myargc] = strdup(dc_pwad_file4);
            myargc++;
        }
    }

    if (dc_deh_file1 || dc_deh_file2 || dc_deh_file3 || dc_deh_file4)
    {
        myargv[myargc] = strdup("-deh");
        myargc++;
        if (dc_deh_file1)
        {
            myargv[myargc] = strdup(dc_deh_file1);
            myargc++;
        }
        if (dc_deh_file2)
        {
            myargv[myargc] = strdup(dc_deh_file2);
            myargc++;
        }
        if (dc_deh_file3)
        {
            myargv[myargc] = strdup(dc_deh_file3);
            myargc++;
        }
        if (dc_deh_file4)
        {
            myargv[myargc] = strdup(dc_deh_file4);
            myargc++;
        }
    }

    if (dc_game_nomonsters)
    {
        myargv[myargc] = strdup("-nomonsters");
        myargc++;
    }

    if (dc_game_respawn)
    {
        myargv[myargc] = strdup("-respawn");
        myargc++;
    }

    if (dc_game_fast)
    {
        myargv[myargc] = strdup("-fast");
        myargc++;
    }

    if (dc_game_turbo)
    {
        myargv[myargc] = strdup("-turbo");
        myargc++;
    }

    if (dc_game_maponhu)
    {
        myargv[myargc] = strdup("-maponhu");
        myargc++;
    }

    if (dc_game_rotatemap)
    {
        myargv[myargc] = strdup("-rotatemap");
        myargc++;
    }

    if (dc_game_deathmatch)
    {
        myargv[myargc] = strdup("-deathmatch");
        myargc++;
    }

    if (dc_game_altdeath)
    {
        myargv[myargc] = strdup("-altdeath");
        myargc++;
    }

    if (dc_game_record)
    {
        myargv[myargc] = strdup("-record");
        myargc++;
        sprintf(temp, "demo%1d", dc_game_record);
        myargv[myargc] = strdup(temp);
        myargc++;
    }

    if (dc_game_playdemo)
    {
        myargv[myargc] = strdup("-playdemo");
        myargc++;
        sprintf(temp, "demo%1d", dc_game_playdemo);
        myargv[myargc] = strdup(temp);
        myargc++;
    }

    if (dc_game_forcedemo)
    {
        myargv[myargc] = strdup("-forcedemo");
        myargc++;
    }

    if (dc_game_timedemo)
    {
        myargv[myargc] = strdup("-timedemo");
        myargc++;
        sprintf(temp, "demo%1d", dc_game_timedemo);
        myargv[myargc] = strdup(temp);
        myargc++;
    }

    if (dc_game_timer)
    {
        myargv[myargc] = strdup("-timer");
        myargc++;
        sprintf(temp, "%2d", dc_game_timer);
        myargv[myargc] = strdup(temp);
        myargc++;
    }

    if (dc_game_avg)
    {
        myargv[myargc] = strdup("-avg");
        myargc++;
    }

    if (dc_game_statcopy)
    {
        myargv[myargc] = strdup("-statcopy");
        myargc++;
    }

    if (dc_game_version != 110)
    {
        myargv[myargc] = strdup("-forceversion");
        myargc++;
        sprintf(temp, "%3d", dc_game_version);
        myargv[myargc] = strdup(temp);
        myargc++;
    }

    if (dc_jump_enable)
    {
        myargv[myargc] = strdup("-rumble");
        myargc++;
    }

    if (dc_stick_disabled)
    {
        myargv[myargc] = strdup("-noanalog");
        myargc++;
    }
    else
    {
        myargv[myargc] = strdup("-analogcx");
        myargc++;
        sprintf(temp, "%d", dc_stick_cx);
        myargv[myargc] = strdup(temp);
        myargc++;
        myargv[myargc] = strdup("-analogcy");
        myargc++;
        sprintf(temp, "%d", dc_stick_cy);
        myargv[myargc] = strdup(temp);
        myargc++;
        myargv[myargc] = strdup("-analogminx");
        myargc++;
        sprintf(temp, "%d", dc_stick_minx);
        myargv[myargc] = strdup(temp);
        myargc++;
        myargv[myargc] = strdup("-analogminy");
        myargc++;
        sprintf(temp, "%d", dc_stick_miny);
        myargv[myargc] = strdup(temp);
        myargc++;
        myargv[myargc] = strdup("-analogmaxx");
        myargc++;
        sprintf(temp, "%d", dc_stick_maxx);
        myargv[myargc] = strdup(temp);
        myargc++;
        myargv[myargc] = strdup("-analogmaxy");
        myargc++;
        sprintf(temp, "%d", dc_stick_maxy);
        myargv[myargc] = strdup(temp);
        myargc++;
    }

    if (dc_ctrl_cheat1)
    {
        myargv[myargc] = strdup("-cheat1");
        myargc++;
        sprintf(temp, "%d", dc_ctrl_cheat1);
        myargv[myargc] = strdup(temp);
        myargc++;
    }
    if (dc_ctrl_cheat2)
    {
        myargv[myargc] = strdup("-cheat2");
        myargc++;
        sprintf(temp, "%d", dc_ctrl_cheat2);
        myargv[myargc] = strdup(temp);
        myargc++;
    }
    if (dc_ctrl_cheat3)
    {
        myargv[myargc] = strdup("-cheat3");
        myargc++;
        sprintf(temp, "%d", dc_ctrl_cheat3);
        myargv[myargc] = strdup(temp);
        myargc++;
    }

    if (dc_net_enabled && !dc_net_error)
    {
        if (dc_net_port != 5029)
        {
            myargv[myargc] = strdup("-port");
            myargc++;
            sprintf(temp, "%d", dc_net_port);
            myargv[myargc] = strdup(temp);
            myargc++;
        }

        if (dc_net_ticdup > 1)
        {
            myargv[myargc] = strdup("-dup");
            myargc++;
            sprintf(temp, "%d", dc_net_ticdup);
            myargv[myargc] = strdup(temp);
            myargc++;
        }

        if (dc_net_extratic)
        {
            myargv[myargc] = strdup("-extratic");
            myargc++;
        }

        if (dc_game_pcchksum)
        {
            myargv[myargc] = strdup("-pcchecksum");
            myargc++;
        }

        myargv[myargc] = strdup("-net");
        myargc++;
        sprintf(temp, "%d", dc_net_player1);
        myargv[myargc] = strdup(temp);
        myargc++;

        if (dc_net_player2)
        {
            if (dc_net_player2[0] >= '1' && dc_net_player2[0] <= '9')
            {
                strcpy(temp, ".");
                strcat(temp, dc_net_player2);
            }
            else
                strcpy(temp, dc_net_player2);
            myargv[myargc] = strdup(temp);
            myargc++;
        }

        if (dc_net_player3)
        {
            if (dc_net_player3[0] >= '1' && dc_net_player3[0] <= '9')
            {
                strcpy(temp, ".");
                strcat(temp, dc_net_player3);
            }
            else
                strcpy(temp, dc_net_player3);
            myargv[myargc] = strdup(temp);
            myargc++;
        }

        if (dc_net_player4)
        {
            if (dc_net_player4[0] >= '1' && dc_net_player4[0] <= '9')
            {
                strcpy(temp, ".");
                strcat(temp, dc_net_player4);
            }
            else
                strcpy(temp, dc_net_player4);
            myargv[myargc] = strdup(temp);
            myargc++;
        }
    }

    if (dc_game_skill != 2)
    {
        myargv[myargc] = strdup("-skill");
        myargc++;
        sprintf(temp, "%d", dc_game_skill+1);
        myargv[myargc] = strdup(temp);
        myargc++;
    }

    if (dc_game_level != 1)
    {
        myargv[myargc] = strdup("-warp");
        myargc++;
        if (isIWADDoom2())
        {
            sprintf(temp, "%d", dc_game_level);
            myargv[myargc] = strdup(temp);
            myargc++;
        }
        else
        {
            sprintf(temp, "%d", (dc_game_level-1) / 9 + 1);
            myargv[myargc] = strdup(temp);
            myargc++;
            sprintf(temp, "%d", (dc_game_level-1) % 9 + 1);
            myargv[myargc] = strdup(temp);
            myargc++;
        }
    }
}

int first_argv(char *check)
{
    int     i;

    for (i=1; i<myargc; i++)
        if ( !strcasecmp(check, myargv[i]) )
            return i;

    return 0;
}

void get_myargv(void)
{
    int i, j;

    dc_vid_fps = 0;
    if (first_argv("-fps"))
        dc_vid_fps = 1;

    dc_use_tv = 0;
    if (first_argv("-tv"))
    {
        dc_use_tv = 1;

        dc_tv_res = 0;
        i = first_argv("-width");
        if (i)
        {
            sscanf(myargv[i+1], "%d", &j);
            switch (j)
            {
                case 304:
                dc_tv_res = 0;
                break;
                case 608:
                dc_tv_res = 1;
                break;
                case 576:
                dc_tv_res = 2;
                break;
            }
        }

        dc_tv_sync = 0;
        if (first_argv("-vsync"))
            dc_tv_sync = 1;

        dc_tv_detail = 0;
        if (first_argv("-lowdetail"))
            dc_tv_detail = 1;

        dc_tv_cx = 0;
        i = first_argv("-tvcx");
        if (i)
            sscanf(myargv[i+1], "%d", &dc_tv_cx);

        dc_tv_cy = 0;
        i = first_argv("-tvcy");
        if (i)
            sscanf(myargv[i+1], "%d", &dc_tv_cy);
    }
    else
    {
        dc_mon_res = 0;
        i = first_argv("-width");
        if (i)
        {
            sscanf(myargv[i+1], "%d", &j);
            switch (j)
            {
                case 320:
                dc_mon_res = 0;
                break;
                case 640:
                dc_mon_res = 1;
                break;
                case 800:
                dc_mon_res = 2;
                break;
            }
        }

        dc_mon_sync = 0;
        if (first_argv("-vsync"))
            dc_mon_sync = 1;

        dc_mon_detail = 0;
        if (first_argv("-lowdetail"))
            dc_mon_detail = 1;
    }

    dc_sfx_enabled = 1;
    if (first_argv("-nosfx"))
        dc_sfx_enabled = 0;

    dc_music_enabled = 0;
    if (first_argv("-music"))
        dc_music_enabled = 1;

    i = first_argv("-iwad");
    if (i)
        dc_iwad_file = strdup(myargv[i+1]);

    i = first_argv("-file");
    if (i)
    {
        dc_pwad_file1 = strdup(myargv[i+1]);
        if ((i+2 < myargc) && myargv[i+2][0] != '-')
        {
            dc_pwad_file2 = strdup(myargv[i+2]);
            if ((i+3 < myargc) && myargv[i+3][0] != '-')
            {
                dc_pwad_file3 = strdup(myargv[i+3]);
                if ((i+4 < myargc) && myargv[i+4][0] != '-')
                    dc_pwad_file4 = strdup(myargv[i+4]);
            }
        }
    }

    i = first_argv("-deh");
    if (i)
    {
        dc_deh_file1 = strdup(myargv[i+1]);
        if ((i+2 < myargc) && myargv[i+2][0] != '-')
        {
            dc_deh_file2 = strdup(myargv[i+2]);
            if ((i+3 < myargc) && myargv[i+3][0] != '-')
            {
                dc_deh_file3 = strdup(myargv[i+3]);
                if ((i+4 < myargc) && myargv[i+4][0] != '-')
                    dc_deh_file4 = strdup(myargv[i+1]);
            }
        }
    }

    dc_game_nomonsters = 0;
    if (first_argv("-nomonsters"))
        dc_game_nomonsters = 1;

    dc_game_respawn = 0;
    if (first_argv("-respawn"))
        dc_game_respawn = 1;

    dc_game_fast = 0;
    if (first_argv("-fast"))
        dc_game_fast = 1;

    dc_game_turbo = 0;
    if (first_argv("-turbo"))
        dc_game_turbo = 1;

    dc_game_maponhu = 0;
    if (first_argv("-maponhu"))
        dc_game_maponhu = 1;

    dc_game_rotatemap = 0;
    if (first_argv("-rotatemap"))
        dc_game_rotatemap = 1;

    dc_game_deathmatch = 0;
    if (first_argv("-deathmatch"))
        dc_game_deathmatch = 1;

    dc_game_altdeath = 0;
    if (first_argv("-altdeath"))
        dc_game_altdeath = 1;

    dc_game_record = 0;
    i = first_argv("-record");
    if (i)
        dc_game_record = (int)(myargv[i+1][4]) - 0x30;

    dc_game_playdemo = 0;
    i = first_argv("-playdemo");
    if (i)
        dc_game_playdemo = (int)(myargv[i+1][4]) - 0x30;

    dc_game_forcedemo = 0;
    if (first_argv("-forcedemo"))
        dc_game_forcedemo = 1;

    dc_game_timedemo = 0;
    i = first_argv("-timedemo");
    if (i)
        dc_game_timedemo = (int)(myargv[i+1][4]) - 0x30;

    dc_game_timer = 0;
    i = first_argv("-timer");
    if (i)
        sscanf(myargv[i+1], "%d", &dc_game_timer);

    dc_game_avg = 0;
    if (first_argv("-avg"))
        dc_game_avg = 1;

    dc_game_statcopy = 0;
    if (first_argv("-statcopy"))
        dc_game_statcopy = 1;

    dc_game_version = 110;
    i = first_argv("-forceversion");
    if (i)
        sscanf(myargv[i+1], "%d", &dc_game_version);

    dc_jump_enable = 0;
    if (first_argv("-rumble"))
        dc_jump_enable = 1;

    dc_stick_disabled = 0;
    i = first_argv("-noanalog");
    if (i)
        dc_stick_disabled = 1;
    else
    {
        dc_stick_cx = 128;
        i = first_argv("-analogcx");
        if (i)
            sscanf(myargv[i+1], "%d", &dc_stick_cx);
        dc_stick_cy = 128;
        i = first_argv("-analogcy");
        if (i)
            sscanf(myargv[i+1], "%d", &dc_stick_cy);
        dc_stick_minx = 0;
        i = first_argv("-analogminx");
        if (i)
            sscanf(myargv[i+1], "%d", &dc_stick_minx);
        dc_stick_miny = 0;
        i = first_argv("-analogminy");
        if (i)
            sscanf(myargv[i+1], "%d", &dc_stick_miny);
        dc_stick_maxx = 255;
        i = first_argv("-analogmaxx");
        if (i)
            sscanf(myargv[i+1], "%d", &dc_stick_maxx);
        dc_stick_maxy = 255;
        i = first_argv("-analogmaxy");
        if (i)
            sscanf(myargv[i+1], "%d", &dc_stick_maxy);
    }

    dc_ctrl_cheat1 = 0;
    i = first_argv("-cheat1");
    if (i)
        sscanf(myargv[i+1], "%d", &dc_ctrl_cheat1);
    dc_ctrl_cheat2 = 0;
    i = first_argv("-cheat2");
    if (i)
        sscanf(myargv[i+1], "%d", &dc_ctrl_cheat2);
    dc_ctrl_cheat3 = 0;
    i = first_argv("-cheat3");
    if (i)
        sscanf(myargv[i+1], "%d", &dc_ctrl_cheat3);

    dc_net_enabled = 0;
    i = first_argv("-net");
    if (i && !dc_net_error)
    {
        dc_net_enabled = 1;
        sscanf(myargv[i+1], "%d", &dc_net_player1);
        if ((i+2 < myargc) && myargv[i+2][0] != '-')
        {
            if (myargv[i+2][0] == '.')
                dc_net_player2 = strdup(&myargv[i+2][1]);
            else
                dc_net_player2 = strdup(myargv[i+2]);
            if ((i+3 < myargc) && myargv[i+3][0] != '-')
            {
                if (myargv[i+3][0] == '.')
                    dc_net_player3 = strdup(&myargv[i+3][1]);
                else
                    dc_net_player3 = strdup(myargv[i+3]);
                if ((i+4 < myargc) && myargv[i+4][0] != '-')
                {
                    if (myargv[i+4][0] == '.')
                        dc_net_player4 = strdup(&myargv[i+4][1]);
                    else
                        dc_net_player4 = strdup(myargv[i+4]);
                }
            }
        }

        dc_net_port = 5029; // 5000 + 0x1d
        i = first_argv("-port");
        if (i)
            sscanf(myargv[i+1], "%d", &dc_net_port);

        dc_net_ticdup = 0;
        i = first_argv("-dup");
        if (i)
            sscanf(myargv[i+1], "%d", &dc_net_ticdup);

        dc_net_extratic = 0;
        if (first_argv("-extratic"))
            dc_net_extratic = 1;

        dc_game_pcchksum = 0;
        if (first_argv("-pcchecksum"))
            dc_game_pcchksum = 1;
    }

    dc_game_skill = 2;
    i = first_argv("-skill");
    if (i)
        sscanf(myargv[i+1], "%d", &dc_game_skill);

    dc_game_level = 1;
    i = first_argv("-warp");
    if (i && isIWADDoom2())
        sscanf(myargv[i+1], "%d", &dc_game_level);
    else if (i)
    {
        int t1, t2;
        sscanf(myargv[i+1], "%d", &t1);
        sscanf(myargv[i+2], "%d", &t2);
        dc_game_level = (t1-1)*9 + t2;
    }

}
