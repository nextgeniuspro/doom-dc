// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005 Simon Howard
// Copyright(C) 2006 Ben Ryves 2006
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.
//
// mus2mid.c - Ben Ryves 2006 - http://benryves.com - benryves@benryves.com
// Use to convert a MUS file into a single track, type 0 MIDI file.

#include <stdio.h>

#include "doomdef.h"
#include "doomtype.h"
#include "m_swap.h"

#include "mus2mid.h"

// MUS event codes
typedef enum
{
	mus_releasekey = 0x00,
	mus_presskey = 0x10,
	mus_pitchwheel = 0x20,
	mus_systemevent = 0x30,
	mus_changecontroller = 0x40,
	mus_scoreend = 0x60
} musevent;

// MIDI event codes
typedef enum
{
	midi_releasekey = 0x80,
	midi_presskey = 0x90,
	midi_aftertouchkey = 0xA0,
	midi_changecontroller = 0xB0,
	midi_changepatch = 0xC0,
	midi_aftertouchchannel = 0xD0,
	midi_pitchwheel = 0xE0
} midievent;


// Structure to hold MUS file header
typedef struct
{
	byte id[4];
	unsigned short scorelength;
	unsigned short scorestart;
	unsigned short primarychannels;
	unsigned short secondarychannels;
	unsigned short instrumentcount;
} __attribute__((packed)) musheader;

// Standard MIDI type 0 header + track header
static byte midiheader[] =
{
	'M', 'T', 'h', 'd',     // Main header
	0x00, 0x00, 0x00, 0x06, // Header size
	0x00, 0x00,             // MIDI type (0)
	0x00, 0x01,             // Number of tracks
	0x00, 0x46,             // Resolution
	'M', 'T', 'r', 'k',		// Start of track
	0x00, 0x00, 0x00, 0x00  // Placeholder for track length
};

// Cached channel velocities
static byte channelvelocities[] =
{
	127, 127, 127, 127, 127, 127, 127, 127,
	127, 127, 127, 127, 127, 127, 127, 127
};

// Timestamps between sequences of MUS events

static unsigned int queuedtime = 0;

// Counter for the length of the track

static unsigned int tracksize;

static byte mus2midi_translation[] =
{
	0x00, 0x20, 0x01, 0x07, 0x0A, 0x0B, 0x5B, 0x5D,
	0x40, 0x43, 0x78, 0x7B, 0x7E, 0x7F, 0x79
};

// Write timestamp to a MIDI file.

static boolean midi_writetime(unsigned int time, FILE *midioutput)
{
	unsigned int buffer = time & 0x7F;
	byte writeval;

	while ((time >>= 7) != 0)
	{
		buffer <<= 8;
		buffer |= ((time & 0x7F) | 0x80);
	}

	for (;;)
	{
		writeval = (byte)(buffer & 0xFF);

		if (fwrite(&writeval, 1, 1, midioutput) != 1)
		{
			return TRUE;
		}

		++tracksize;

		if ((buffer & 0x80) != 0)
		{
			buffer >>= 8;
		}
		else
		{
			queuedtime = 0;
			return FALSE;
		}
	}
}


// Write the end of track marker
static boolean midi_writeendtrack(FILE *midioutput)
{
	byte endtrack[] = {0xFF, 0x2F, 0x00};

	if (midi_writetime(queuedtime, midioutput))
	{
		return TRUE;
	}

	if (fwrite(endtrack, 1, 3, midioutput) != 3)
	{
		return TRUE;
	}

	tracksize += 3;
	return FALSE;
}

// Write a key press event
static boolean midi_writepresskey(byte channel, byte key, byte velocity, FILE *midioutput)
{
	byte working = midi_presskey | channel;

	if (midi_writetime(queuedtime, midioutput))
	{
		return TRUE;
	}

	if (fwrite(&working, 1, 1, midioutput) != 1)
	{
		return TRUE;
	}

	working = key & 0x7F;

	if (fwrite(&working, 1, 1, midioutput) != 1)
	{
		return TRUE;
	}

	working = velocity & 0x7F;

	if (fwrite(&working, 1, 1, midioutput) != 1)
	{
		return TRUE;
	}

	tracksize += 3;

	return FALSE;
}

// Write a key release event
static boolean midi_writereleasekey(byte channel, byte key, FILE *midioutput)
{
	byte working = midi_releasekey | channel;

	if (midi_writetime(queuedtime, midioutput))
	{
		return TRUE;
	}

	if (fwrite(&working, 1, 1, midioutput) != 1)
	{
		return TRUE;
	}

	working = key & 0x7F;

	if (fwrite(&working, 1, 1, midioutput) != 1)
	{
		return TRUE;
	}

	working = 0;

	if (fwrite(&working, 1, 1, midioutput) != 1)
	{
		return TRUE;
	}

	tracksize += 3;

	return FALSE;
}

// Write a pitch wheel/bend event
static boolean midi_writepitchwheel(byte channel, short wheel, FILE *midioutput)
{
	byte working = midi_pitchwheel | channel;

	if (midi_writetime(queuedtime, midioutput))
	{
		return TRUE;
	}

	if (fwrite(&working, 1, 1, midioutput) != 1)
	{
		return TRUE;
	}

	working = wheel & 0x7F;

	if (fwrite(&working, 1, 1, midioutput) != 1)
	{
		return TRUE;
	}

	working = (wheel >> 7) & 0x7F;

	if (fwrite(&working, 1, 1, midioutput) != 1)
	{
		return TRUE;
	}

	tracksize += 3;
	return FALSE;
}

// Write a patch change event
static boolean midi_writechangepatch(byte channel, byte patch, FILE *midioutput)
{
	byte working = midi_changepatch | channel;

	if (midi_writetime(queuedtime, midioutput))
	{
		return TRUE;
	}

	if (fwrite(&working, 1, 1, midioutput) != 1)
	{
		return TRUE;
	}

	working = patch & 0x7F;

	if (fwrite(&working, 1, 1, midioutput) != 1)
	{
		return TRUE;
	}

	tracksize += 2;

	return FALSE;
}



// Write a valued controller change event
static boolean midi_writechangecontroller_valued(byte channel, byte control, byte value, FILE *midioutput)
{
	byte working = midi_changecontroller | channel;

	if (midi_writetime(queuedtime, midioutput))
	{
		return TRUE;
	}

	if (fwrite(&working, 1, 1, midioutput) != 1)
	{
		return TRUE;
	}

	working = control & 0x7F;

	if (fwrite(&working, 1, 1, midioutput) != 1)
	{
		return TRUE;
	}
	// Quirk in vanilla DOOM? MUS controller values should be
	// 7-bit, not 8-bit.

	working = value;// & 0x7F;

	// Fix on said quirk to stop MIDI players from complaining that
	// the value is out of range:

	if (working & 0x80)
	{
		working = 0x7F;
	}

	if (fwrite(&working, 1, 1, midioutput) != 1)
	{
		return TRUE;
	}

	tracksize += 3;

	return FALSE;
}

// Write a valueless controller change event
static boolean midi_writechangecontroller_valueless(byte channel, byte control, FILE *midioutput)
{
	return midi_writechangecontroller_valued(channel, control, 0, midioutput);
}

static boolean read_musheader(FILE *file, musheader *header)
{
	boolean result;

	result = (fread(&header->id, sizeof(byte), 4, file) == 4)
              && (fread(&header->scorelength, sizeof(short), 1, file) == 1)
              && (fread(&header->scorestart, sizeof(short), 1, file) == 1)
              && (fread(&header->primarychannels, sizeof(short), 1, file) == 1)
              && (fread(&header->secondarychannels, sizeof(short), 1, file) == 1)
              && (fread(&header->instrumentcount, sizeof(short), 1, file) == 1);

	if (result)
	{
		header->scorelength = SWAPSHORT(header->scorelength);
		header->scorestart = SWAPSHORT(header->scorestart);
		header->primarychannels = SWAPSHORT(header->primarychannels);
		header->secondarychannels = SWAPSHORT(header->secondarychannels);
		header->instrumentcount = SWAPSHORT(header->instrumentcount);
	}

	return result;
}


// Read a MUS file from a stream (musinput) and output a MIDI file to
// a stream (midioutput).
//
// Returns 0 on success or 1 on failure.

boolean mus2mid(FILE *musinput, FILE *midioutput)
{
	// Header for the MUS file
	musheader musfileheader;

	// Descriptor for the current MUS event
	byte eventdescriptor;
	int channel; // Channel number
	musevent event;


	// Bunch of vars read from MUS lump
	byte key;
	byte controllernumber;
	byte controllervalue;

	// Buffer used for MIDI track size record
	byte tracksizebuffer[4];

	// Flag for when the score end marker is hit.
	int hitscoreend = 0;

	// Temp working byte
	byte working;
	// Used in building up time delays
	unsigned int timedelay;

	// Grab the header

	if (!read_musheader(musinput, &musfileheader))
	{
		return TRUE;
	}

#ifdef CHECK_MUS_HEADER
	// Check MUS header
	if (musfileheader.id[0] != 'M'
	 || musfileheader.id[1] != 'U'
	 || musfileheader.id[2] != 'S'
	 || musfileheader.id[3] != 0x1A)
	{
		return TRUE;
	}
#endif

	// Seek to where the data is held
	if (fseek(musinput, (long)musfileheader.scorestart, SEEK_SET) != 0)
	{
		return TRUE;
	}

	// So, we can assume the MUS file is faintly legit. Let's start
	// writing MIDI data...

	fwrite(midiheader, 1, sizeof(midiheader), midioutput);
	tracksize = 0;

	// Now, process the MUS file:
	while (!hitscoreend)
	{
		// Handle a block of events:

		while (!hitscoreend)
		{
			// Fetch channel number and event code:

			if (fread(&eventdescriptor, 1, 1, musinput) != 1)
			{
				return TRUE;
			}

			channel = eventdescriptor & 0x0F;
			event = eventdescriptor & 0x70;

			// Swap channels 15 and 9.
			// MIDI channel 9 = percussion.
			// MUS channel 15 = percussion.

			if (channel == 15)
			{
				channel = 9;
			}
			else if (channel == 9)
			{
				channel = 15;
			}

			switch (event)
			{
				case mus_releasekey:
					if (fread(&key, 1, 1, musinput) != 1)
					{
						return TRUE;
					}

					if (midi_writereleasekey(channel, key, midioutput))
					{
						return TRUE;
					}

					break;

				case mus_presskey:
					if (fread(&key, 1, 1, musinput) != 1)
					{
						return TRUE;
					}

					if (key & 0x80)
					{
						if (fread(&channelvelocities[channel], 1, 1, musinput) != 1)
						{
							return TRUE;
						}

						channelvelocities[channel] &= 0x7F;
					}

					if (midi_writepresskey(channel, key, channelvelocities[channel], midioutput))
					{
						return TRUE;
					}

					break;

				case mus_pitchwheel:
					if (fread(&key, 1, 1, musinput) != 1)
					{
						break;
					}
					if (midi_writepitchwheel(channel, (short)(key * 64), midioutput))
					{
						return TRUE;
					}

					break;

				case mus_systemevent:
					if (fread(&controllernumber, 1, 1, musinput) != 1)
					{
						return TRUE;
					}
					if (controllernumber < 10 || controllernumber > 14)
					{
						return TRUE;
					}

					if (midi_writechangecontroller_valueless(channel, mus2midi_translation[controllernumber], midioutput))
					{
						return TRUE;
					}

					break;

				case mus_changecontroller:
					if (fread(&controllernumber, 1, 1, musinput) != 1)
					{
						return TRUE;
					}

					if (fread(&controllervalue, 1, 1, musinput) != 1)
					{
						return TRUE;
					}

					if (controllernumber == 0)
					{
						if (midi_writechangepatch(channel, controllervalue, midioutput))
						{
							return TRUE;
						}
					}
					else
					{
						if (controllernumber < 1 || controllernumber > 9)
						{
							return TRUE;
						}

						if (midi_writechangecontroller_valued(channel, mus2midi_translation[controllernumber], controllervalue, midioutput))
						{
							return TRUE;
						}
					}

					break;

				case mus_scoreend:
					hitscoreend = 1;
					break;

				default:
					return TRUE;
					break;
			}

			if (eventdescriptor & 0x80)
			{
				break;
			}
		}
		// Now we need to read the time code:
		if (!hitscoreend)
		{
			timedelay = 0;
			for (;;)
			{
				if (fread(&working, 1, 1, musinput) != 1)
				{
					return TRUE;
				}

				timedelay = timedelay * 128 + (working & 0x7F);
				if ((working & 0x80) == 0)
				{
					break;
				}
			}
			queuedtime += timedelay;
		}
	}

	// End of track
	if (midi_writeendtrack(midioutput))
	{
		return TRUE;
	}

	// Write the track size into the stream
	if (fseek(midioutput, 18, SEEK_SET))
	{
		return TRUE;
	}

        tracksizebuffer[0] = (tracksize >> 24) & 0xff;
        tracksizebuffer[1] = (tracksize >> 16) & 0xff;
        tracksizebuffer[2] = (tracksize >> 8) & 0xff;
        tracksizebuffer[3] = tracksize & 0xff;

	if (fwrite(tracksizebuffer, 1, 4, midioutput) != 4)
	{
		return TRUE;
	}

	return FALSE;
}

