#include <kos.h>
#include "debug_dc.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "z_zone.h"

#include "i_system.h"
#include "i_sound.h"
#include "m_argv.h"
#include "m_misc.h"
#include "w_wad.h"
#include "m_swap.h"

#include "doomdef.h"

#include "aica.h"

#include <dc/g2bus.h>
#include <dc/spu.h>

#include "mus2mid.h"
#include "libWildMidi/wildmidi_lib.h"


// Any value of numChannels set
// by the defaults code in M_misc is now clobbered by I_InitSound().
// number of channels available for sound effects

extern int numChannels;
extern char dc_home[8];

/**********************************************************************/

#define SFX_VOICES 16

struct Voice {
  uint32 index;
  uint32 step;
  uint32 loop;
  uint32 length;
  int ltvol;
  int rtvol;
  int flags;
  char *wave;
};

static struct Voice audVoice[SFX_VOICES];

// The actual lengths of all sound effects.
static int lengths[NUMSFX];

static int sound_status = 0;

static kthread_t * snd_thread = NULL;

#define NUM_SAMPLES 1024

static short musout[NUM_SAMPLES * 2];
static short pcmoutl[NUM_SAMPLES * 2];
static short pcmoutr[NUM_SAMPLES * 2];
static int pcmflip = 0;

static int changepitch;

/* sampling freq (Hz) for each pitch step when changepitch is TRUE
   calculated from (2^((i-128)/64))*11025
   I'm not sure if this is the right formula */
static uint32 freqs[256] = {
  2756, 2786, 2817, 2847, 2878, 2910, 2941, 2973,
  3006, 3038, 3072, 3105, 3139, 3173, 3208, 3242,
  3278, 3313, 3350, 3386, 3423, 3460, 3498, 3536,
  3574, 3613, 3653, 3692, 3733, 3773, 3814, 3856,
  3898, 3940, 3983, 4027, 4071, 4115, 4160, 4205,
  4251, 4297, 4344, 4391, 4439, 4487, 4536, 4586,
  4635, 4686, 4737, 4789, 4841, 4893, 4947, 5001,
  5055, 5110, 5166, 5222, 5279, 5336, 5394, 5453,
  5513, 5573, 5633, 5695, 5757, 5819, 5883, 5947,
  6011, 6077, 6143, 6210, 6278, 6346, 6415, 6485,
  6556, 6627, 6699, 6772, 6846, 6920, 6996, 7072,
  7149, 7227, 7305, 7385, 7465, 7547, 7629, 7712,
  7796, 7881, 7967, 8053, 8141, 8230, 8319, 8410,
  8501, 8594, 8688, 8782, 8878, 8975, 9072, 9171,
  9271, 9372, 9474, 9577, 9681, 9787, 9893, 10001,
  10110, 10220, 10331, 10444, 10558, 10673, 10789, 10906,
  11025, 11145, 11266, 11389, 11513, 11638, 11765, 11893,
  12023, 12154, 12286, 12420, 12555, 12692, 12830, 12970,
  13111, 13254, 13398, 13544, 13691, 13841, 13991, 14144,
  14298, 14453, 14611, 14770, 14931, 15093, 15258, 15424,
  15592, 15761, 15933, 16107, 16282, 16459, 16639, 16820,
  17003, 17188, 17375, 17564, 17756, 17949, 18144, 18342,
  18542, 18744, 18948, 19154, 19363, 19574, 19787, 20002,
  20220, 20440, 20663, 20888, 21115, 21345, 21578, 21812,
  22050, 22290, 22533, 22778, 23026, 23277, 23530, 23787,
  24046, 24308, 24572, 24840, 25110, 25384, 25660, 25940,
  26222, 26508, 26796, 27088, 27383, 27681, 27983, 28287,
  28595, 28907, 29221, 29540, 29861, 30187, 30515, 30848,
  31183, 31523, 31866, 32213, 32564, 32919, 33277, 33639,
  34006, 34376, 34750, 35129, 35511, 35898, 36289, 36684,
  37084, 37487, 37896, 38308, 38725, 39147, 39573, 40004,
  40440, 40880, 41325, 41775, 42230, 42690, 43155, 43625
};

static int musicdies=-1;
static int music_okay = 0;

static int mus_playing = 0;
static int mus_looping = 0;
static int mus_volume = 127;

static midi *midi_ptr = NULL;
static struct _WM_Info *wm_info = NULL;

/**********************************************************************/

void fill_buffer(int offset);

void Sfx_Start(char *wave, int cnum, int rate, int vol, int sep, int length);
void Sfx_Update(int cnum, int rate, int vol, int sep);
void Sfx_Stop(int cnum);
int Sfx_Pos(int cnum);

void Mus_SetVol(int vol);
int Mus_Register(void *musdata, int length);
void Mus_Unregister(int handle);
void Mus_Play(int handle, int looping);
void Mus_Stop(int handle);
void Mus_Pause(int handle);
void Mus_Resume(int handle);

/**********************************************************************/

#if 0
extern void video_set_vmode(void);

void dc_error_msg(char *str)
{
    vid_set_mode(DM_640x480, PM_RGB555);
    vid_empty();
    bfont_set_pmode(PM_RGB555);
    DebugDC_Init(DM_640x480, PM_RGB555); // enable debug output to screen

    DebugDC_ScreenSetXY((53-strlen(str))/2, 10);
    DebugDC_ScreenPuts(str);
    thd_sleep(10*1000);

    video_set_vmode();
}
#endif

/**********************************************************************/
//
// This function loads the sound data for sfxname from the WAD lump,
// and returns a ptr to the data in fastmem and its length in len.
//
static void *getsfx (char *sfxname, int *len)
{
  unsigned char *sfx;
  unsigned char *cnvsfx;
  int i;
  int size;
  char name[32];
  int sfxlump;

  // Get the sound data from the WAD, allocate lump
  //  in zone memory.
  sprintf(name, "ds%s", sfxname);

  // Now, there is a severe problem with the
  //  sound handling, in it is not (yet/anymore)
  //  gamemode aware. That means, sounds from
  //  DOOM II will be requested even with DOOM
  //  shareware.
  // The sound list is wired into sounds.c,
  //  which sets the external variable.
  // I do not do runtime patches to that
  //  variable. Instead, we will use a
  //  default sound for replacement.
  if (W_CheckNumForName(name) == -1)
    sfxlump = W_GetNumForName("dspistol");
  else
    sfxlump = W_GetNumForName (name);

  size = W_LumpLength (sfxlump);

  // Debug.
  // fprintf( stderr, "." );
  // fprintf( stderr, " -loading  %s (lump %d, %d bytes)\n",
  //         sfxname, sfxlump, size );
  //fflush( stderr );

  sfx = (unsigned char*)W_CacheLumpNum (sfxlump, PU_STATIC);

  // Allocate from zone memory.
  cnvsfx = (unsigned char*)Z_Malloc (size, PU_STATIC, 0);

  // Now copy and convert offset to signed.
  for (i = 0; i < size; i++)
    cnvsfx[i] = sfx[i] ^ 0x80;

  // Remove the cached lump.
  Z_Free( sfx );

  // return length.
  *len = size;

  // Return allocated converted data.
  return (void *)cnvsfx;
}

/**********************************************************************/
// Buffer filling

static void *do_snd(void *argp)
{
    while(sound_status != 0xDEADBEEF){
        while(aica_get_pos(0)/NUM_SAMPLES == pcmflip)
            thd_pass();

        fill_buffer(pcmflip ? NUM_SAMPLES : 0);
        pcmflip ^= 1;
    }
    return NULL;
}

/**********************************************************************/
// Init at program start...

void I_InitSound (void)
{
    int i;

    printf("I_InitSound()\n");

    numChannels = 2;
    sound_status = 1;

    if (M_CheckParm("-nosfx"))
        return;

    // S_AdjustSoundParams doesn't alter pitch, so this won't be found
    changepitch = M_CheckParm ("-changepitch");

    // Initialize external data (all sounds) at start, keep static.
    for (i=1; i<NUMSFX; i++) {
        // Alias? Example is the chaingun sound linked to pistol.
        if (!S_sfx[i].link) {
            // Load data from WAD file.
            S_sfx[i].data = getsfx(S_sfx[i].name, &lengths[i]);
        } else {
            // Previously loaded already?
            S_sfx[i].data = S_sfx[i].link->data;
            lengths[i] = lengths[(S_sfx[i].link - S_sfx)/sizeof(sfxinfo_t)];
        }
    }
    printf ("I_InitSound: Pre-cached all sound data.\n");

    // disable all voices
    for (i=0; i<SFX_VOICES; i++)
        audVoice[i].flags = 0;

    numChannels = SFX_VOICES;

    // start aica voices
    aica_play(0, SM_16BIT, 0x10000, 0, NUM_SAMPLES*2, 44100, 255, 0, 1); // left channel
    aica_play(1, SM_16BIT, 0x10000+NUM_SAMPLES*4, 0, NUM_SAMPLES*2, 44100, 255, 255, 1); // right channel

    //Start buffer filling thread:
    snd_thread = thd_create(0, do_snd, NULL);
    if(!snd_thread)
    {
        printf ("I_InitSound: Cannot create sound daemon, no sfx or music.\n");
        return;
    }
    thd_sleep(100);

    sound_status = 2;

    // Finished initialization.
    printf ("I_InitSound: Sound module ready.\n");
}

/**********************************************************************/
// ... update sound buffer and audio device at runtime...
void I_UpdateSound (void)
{
}

/**********************************************************************/
// ... update sound buffer and audio device at runtime...
void I_SubmitSound (void)
{
}

/**********************************************************************/
// ... shut down and relase at program termination.
void I_ShutdownSound (void)
{
    printf (" I_ShutdownSound: ");

    if (sound_status > 0) {
        sound_status = 0xDEADBEEF;         // DIE, DAEMON! DIE!!
        thd_sleep(1000);
    }
    aica_stop(0);
    aica_stop(1);

    printf (" Sound module closed.\n");
}

/**********************************************************************/
/**********************************************************************/
//
//  SFX I/O
//

/**********************************************************************/
// Initialize number of channels
void I_SetChannels (void)
{
}

/**********************************************************************/
// Get raw data lump index for sound descriptor.
int I_GetSfxLumpNum (sfxinfo_t *sfx)
{
  char namebuf[9];

//  fprintf (stderr, "I_GetSfxLumpNum()\n");

  sprintf(namebuf, "ds%s", sfx->name);
  return W_GetNumForName(namebuf);
}

/**********************************************************************/
// Starts a sound in a particular sound channel.
int I_StartSound (
  int id,
  int cnum,
  int vol,
  int sep,
  int pitch,
  int priority )
{
//  fprintf (stderr, "I_StartSound(%d,%d,%d,%d,%d,%d)\n", id, cnum, vol, sep, pitch, priority);

  if (sound_status == 2) {
    Sfx_Stop(cnum);
    Sfx_Start (S_sfx[id].data, cnum, changepitch ? freqs[pitch] : 11025,
               vol, sep, lengths[id]);
  }
  return cnum;
}

/**********************************************************************/
// Stops a sound channel.
void I_StopSound(int handle)
{
//  fprintf (stderr, "I_StopSound(%d)\n", handle);

  if (sound_status == 2)
    Sfx_Stop(handle);
}

/**********************************************************************/
// Called by S_*() functions
//  to see if a channel is still playing.
// Returns 0 if no longer playing, 1 if playing.
int I_SoundIsPlaying(int handle)
{
//  fprintf (stderr, "I_SoundIsPlaying(%d)\n", handle);

  if (sound_status == 2)
    return Sfx_Pos(handle) ? 1 : 0;

  return 0;
}

/**********************************************************************/
// Updates the volume, separation,
//  and pitch of a sound channel.
void
I_UpdateSoundParams
( int       handle,
  int       vol,
  int       sep,
  int       pitch )
{
//  fprintf (stderr, "I_UpdateSoundParams(%d,%d,%d,%d)\n", handle, vol, sep, pitch);

  if (sound_status == 2)
    Sfx_Update(handle, changepitch ? freqs[pitch] : 11025, vol, sep);
}

/**********************************************************************/
/**********************************************************************/
//
// MUSIC API.
//
/**********************************************************************/
//
//  MUSIC I/O
//
void I_InitMusic(void)
{
  uint32 mixer_options = 0;

  printf ("I_InitMusic:");

  music_okay = 0;

  if (M_CheckParm("-music")) {
    // try to init WildMidi
    //mixer_options |= WM_MO_REVERB;
    //mixer_options |= WM_MO_EXPENSIVE_INTERPOLATION;
    mixer_options |= WM_MO_LINEAR_VOLUME;
    if (WildMidi_Init("/cd/timidity/timidity.cfg", 44100, mixer_options) != -1) {
      WildMidi_MasterVolume(mus_volume);
      music_okay = 1;
      printf (" Music okay.\n");
      thd_sleep(2*1000);
    } else {
      printf(" WildMidi failed to initialize.\n   Music not enabled.\n");
      thd_sleep(2*1000);
    }
  } else {
    printf (" Music not enabled.\n");
    thd_sleep(2*1000);
  }
}

/**********************************************************************/
void I_ShutdownMusic(void)
{
  printf (" I_ShutdownMusic: ");
  if (music_okay)
  {
    WildMidi_Shutdown();
    music_okay = 0;
  }
  printf (" Music module closed.\n");
}

/**********************************************************************/
// Volume.
void I_SetMusicVolume(int volume)
{
//  fprintf (stderr, "I_SetMusicVolume(%d)\n", volume);

  if (music_okay)
    Mus_SetVol(volume);
}

/**********************************************************************/
// PAUSE game handling.
void I_PauseSong(int handle)
{
//  fprintf (stderr, "I_PauseSong(%d)\n", handle);

  if (music_okay)
    Mus_Pause(handle);
}

/**********************************************************************/
void I_ResumeSong(int handle)
{
//  fprintf (stderr, "I_ResumeSong(%d)\n", handle);

  if (music_okay)
    Mus_Resume(handle);
}

/**********************************************************************/
// Registers a song handle to song data.
int I_RegisterSong(void *data, int length)
{
//  fprintf (stderr, "I_RegisterSong(%08x)\n", data);

  if (music_okay)
    return Mus_Register(data, length);

  return 0;
}

/**********************************************************************/
// Called by anything that wishes to start music.
//  plays a song, and when the song is done,
//  starts playing it again in an endless loop.
// Horrible thing to do, considering.
void I_PlaySong(int handle, int looping)
{
//  fprintf (stderr, "I_PlaySong(%d,%d)\n", handle, looping);

  if (music_okay)
    Mus_Play(handle, looping);

  musicdies = gametic + TICRATE*30;
}

/**********************************************************************/
// Stops a song over 3 seconds.
void I_StopSong(int handle)
{
//  fprintf (stderr, "I_StopSong(%d)\n", handle);

  if (music_okay)
    Mus_Stop(handle);

  musicdies = 0;
}

/**********************************************************************/
// See above (register), then think backwards
void I_UnRegisterSong(int handle)
{
//  fprintf (stderr, "I_UnRegisterSong(%d)\n", handle);

  if (music_okay)
    Mus_Unregister(handle);
}

/**********************************************************************/

void Sfx_Start(char *wave, int cnum, int rate, int volume, int seperation, int length)
{
  int vol = volume ? (volume << 4) | 0x0F : 0;
  int pan = seperation;

  audVoice[cnum].wave = wave + 8;
  audVoice[cnum].index = 0;
  audVoice[cnum].step = (rate << 16) / 11025; // 16.16
  audVoice[cnum].loop = 0;
  audVoice[cnum].length = (length - 8) << 16; // 16.16
  audVoice[cnum].ltvol = vol - (vol * pan * pan) / 65025;
  pan -= 255;
  audVoice[cnum].rtvol = vol - (vol * pan * pan) / 65025;
  audVoice[cnum].flags = 0x81; // active SFX
  thd_pass();
}

void Sfx_Update(int cnum, int rate, int volume, int seperation)
{
  int vol = volume ? (volume << 4) | 0x0F : 0;
  int pan = seperation;

  audVoice[cnum].step = (rate << 16) / 11025;
  audVoice[cnum].ltvol = vol - (vol * pan * pan) / 65025;
  pan -= 255;
  audVoice[cnum].rtvol = vol - (vol * pan * pan) / 65025;
  thd_pass();
}

void Sfx_Stop(int cnum)
{
  audVoice[cnum].flags = 0; // deactivate voice
  thd_pass();
}

int Sfx_Pos(int cnum)
{
  thd_pass();
  return (audVoice[cnum].flags & 0x01) ? (audVoice[cnum].index >> 16) + 1 : 0;
}

/**********************************************************************/

void Mus_SetVol(int vol)
{
  mus_volume = vol ? (vol << 3) | 0x07 : 0; // 00, 0f, 17, 1f, 27, 2f... 77, 7f
  WildMidi_MasterVolume(mus_volume);
}

int Mus_Register(void *musdata, int length)
{
  FILE *mus_handle;
  FILE *mid_handle;
  boolean result;

  mus_playing = 0; // don't play until told to start

  // on DC, we count on the current directory being the ram disk
  mus_handle = fopen("temp.mus", "wb");
  if (mus_handle == NULL) {
    //dc_error_msg("Failed to open MUS file for writing");
    return 0;
  }
  fwrite(musdata, 1, length, mus_handle);
  fclose(mus_handle);
  mus_handle = fopen("temp.mus", "rb");
  mid_handle = fopen("temp.mid", "wb");
  if (mus_handle && mid_handle)
    result = mus2mid(mus_handle, mid_handle);
  else
    result = TRUE;
  fclose(mus_handle);
  fclose(mid_handle);
  remove("temp.mus");

  if (result) {
    //dc_error_msg("Failed to convert MUS file to MIDI");
    return 0;
  }

  midi_ptr = WildMidi_Open("temp.mid");
  if (midi_ptr) {
    wm_info = WildMidi_GetInfo(midi_ptr);
    WildMidi_LoadSamples(midi_ptr);
    WildMidi_MasterVolume(mus_volume);
  } else {
    //dc_error_msg("WildMidi_Open failed");
  }

  return (int)midi_ptr;
}

void Mus_Unregister(int handle)
{
  if (handle) {
    Mus_Stop(handle);
    WildMidi_Close((midi *)handle);
  }
  remove("temp.mid");
}

void Mus_Play(int handle, int looping)
{
  if (!handle)
    return;

  mus_looping = looping;
  mus_playing = 2;                     // 2 = play from start
  thd_pass();
}

void Mus_Stop(int handle)
{
  if(mus_playing) {
    mus_playing = -1;                  // stop playing
    mus_looping = 0;
    while (mus_playing)
      thd_sleep(100);
  }
}

void Mus_Pause(int handle)
{
  mus_playing = 0;                     // 0 = not playing
  thd_pass();
}

void Mus_Resume(int handle)
{
  mus_playing = 1;                     // 1 = play from current position
  thd_pass();
}

/**********************************************************************/

void fill_buffer(int offset)
{
  uint32 index;
  uint32 step;
  uint32 loop;
  uint32 length;
  int ltvol;
  int rtvol;
  char *wvbuff;
  int sample, sum;
  int ix;
  int iy;

  // determine if music is playing
  if (mus_playing < 0)
    mus_playing = 0;                 // music now off
  else if (mus_playing > 1) {
    uint32 start_pos = 0;
    // start music from beginning
    WildMidi_FastSeek(midi_ptr, &start_pos);
    wm_info = WildMidi_GetInfo(midi_ptr);
    mus_playing = 1;
  }

  // fill buffers with either music or silence
  if (mus_playing == 1) {
    int output_result;
    uint32 count_diff = wm_info->approx_total_samples - wm_info->current_sample;
    if (count_diff <= NUM_SAMPLES) {
      mus_playing = mus_looping ? 2 : 0; // restart from beginning next loop if looping
      output_result = WildMidi_GetOutput(midi_ptr, (char *)musout, (count_diff * 4));
      memset(&musout[count_diff*2], 0, (NUM_SAMPLES - count_diff)*4);
    } else {
      output_result = WildMidi_GetOutput(midi_ptr, (char *)musout, (NUM_SAMPLES * 4));
    }
    wm_info = WildMidi_GetInfo(midi_ptr);
    for (iy=0; iy<NUM_SAMPLES; iy++) {
      pcmoutl[offset + iy] = musout[iy * 2];
      pcmoutr[offset + iy] = musout[iy * 2 + 1];
    }
  } else {
    // clear buffers
    memset((void *)&pcmoutl[offset], 0, NUM_SAMPLES*2);
    memset((void *)&pcmoutr[offset], 0, NUM_SAMPLES*2);
  }

  // mix active sfx voices
  for (ix=0; ix<SFX_VOICES; ix++) {
    if (audVoice[ix].flags & 0x01) {
      // process active voice
      index = audVoice[ix].index;
      step = audVoice[ix].step >> 2;
      loop = audVoice[ix].loop;
      length = audVoice[ix].length;
      ltvol = audVoice[ix].ltvol;
      rtvol = audVoice[ix].rtvol;
      wvbuff = audVoice[ix].wave;

      for (iy=0; iy<NUM_SAMPLES; iy++) {
        if (index >= length) {
            audVoice[ix].flags = 0;  // disable sfx voice
            break;                   // exit sample loop
        }
        sample = wvbuff ? wvbuff[index >> 16] : 0;  // for safety
        sum = (int)pcmoutl[offset + iy] + (sample * ltvol >> 1);
        if (sum > 32767)
            sum = 32767;
        else if (sum < -32768)
            sum = -32768;
        pcmoutl[offset + iy] = (short)sum;
        sum = (int)pcmoutr[offset + iy] + (sample * rtvol >> 1);
        if (sum > 32767)
            sum = 32767;
        else if (sum < -32768)
            sum = -32768;
        pcmoutr[offset + iy] = (short)sum;
        index += step;
      }
      audVoice[ix].index = index;
    }
  }
  spu_memload(0x10000 + offset*2, &pcmoutl[offset], NUM_SAMPLES*2);
  spu_memload(0x10000 + NUM_SAMPLES*4 + offset*2, &pcmoutr[offset], NUM_SAMPLES*2);
}

/**********************************************************************/
