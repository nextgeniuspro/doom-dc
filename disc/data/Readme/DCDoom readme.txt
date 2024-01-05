==========================================================================
                      Doom v1.4 for the Dreamcast
==========================================================================

Version 1.4 by Chilly Willy (ChillyWillyGuru@gmail.com) based on Doom for
the PSP Version 1.4 by Chilly Willy based on ADoomPPC version 1.7 by Jarmo
Laakkonen (jami.laakkonen@kolumbus.fi) based on ADoomPPC version 1.3 by
Joseph Fenton based on ADoom version 1.2 by Peter McGavin.

Danzeff OSK by Danzel and Jeff Chen. Mus2Midi by Ben Ryves. WildMidi by
Chris Ison.

PAL selection menu by BlueCrab.

MSX Font by Marcus R. Brown, James Forshaw, and John Kelley.

==========================================================================

This archive contains a DC port of ADoomPPC, itself a port of ADoom.
ADoom is a port of ID Software's Linux DOOM.  You can get the original
ID Software Linux DOOM source from:

    ftp://ftp.idsoftware.com/idstuff/source/doomsrc.zip

Source code for this port of Doom is included. Remember, an SBI file is
just a zip file, so if all you want to do is look at the source, unzip
DCDoom.sbi and look in the src directory.

Features include three different resolutions for both VGA and composite
out, stereo sound and music, and control over most of the game options.

Installation
============
Copy DCDoom.sbi into the SBI directory of SelfBoot Inducer. Run SBI,
select DCDoom, and click Extract. Then switch over to a Windows file
manager. Insider the inducer directory, copy your main WAD files into
the iwad directory, your patch WAD files into the pwad directory, and
your DeHackEd files into the deh directory. Go back to SBI and create
an disc image. If you don't have any other files, making a disc image
after extracting the SBI will allow you to play the shareware version
of Doom.

The makefile in the source now allows you to make a self-booting disc
by running "make clean", "make all", then "make selfboot". You should
know about making Dreamcast homebrew before attempting this. The CD
writer is set to /dev/cdrom by default in the makefile. If your writer
is at some other device, edit the makefile before trying this.

Building Doom from source also requires you to patch the biosfont
routines in KOS. Look for the diff file in the kos-fixes directory
in the source directory. If you can't patch and rebuild kos, you
probably shouldn't be attempting to build Doom from the source.

Requirements
============

Doom for the DC requires a WAD file compatible with DOOM for the PC or
Macintosh; you can get a compilation of The Ultimate DOOM, DOOM II, and
Final DOOM for around $20 (US); get it.  Only wimps use the shareware
wad file. Despite the last sentence, the shareware WAD file is included
so that you can play DOOM on your DC right away.

GUI
===

At the upper level, press B to run DOOM. You change your selection
in the menu using the directional pad. Items in red are disabled and
cannot be selected, while items in green can. The selected item will
be white. When you are told to hold A to change something, use the
D-Pad to make the change. Down and left will increase the item or
select the next one, while up and right will decrease the item or
select the previous one.

Load Settings: Press A to load a settings file. My own OSK is used to
enter the load name. This file must exist on the VMU.

Save Settings: Press A to save the current settings. My own OSK is
used to enter the save name. The default name is "doom.set", which
is the name of the file loaded automatically when you start Doom for
the DC. You don't have to call your file this, but it won't be loaded
automatically if you don't. The settings will be saved to the VMU,
assuming there is space for it.

Load Game Presets: Press A to load a settings file from the presets
directory of the CD. A file requestor will allow you to select the
preset file to load. This allows you to make settings files for modified
games.

Video Settings: Press A to enter the Video sub-menu. It has two
sub-menus and a Display selector.

Monitor Settings: Press A to enter the Monitor Settings sub-menu. This
will be disabled if you don't have a VGA cable.

Cable: This just tells you what TV cable is detected.

Resolution: Hold A and use the d-pad to change the resolution. The
selections are 320x240 and 640x480.

Sync to VBlank: Press A to toggle on or off. When on, the video refresh
waits for the vertical blank before flipping the screen.

Show FPS: Press A to toggle on or off. While on, the frames per second
will be shown in the upper-left corner of the screen. The number is the
FPS for the last second of play.

Detail: Hold A and use the d-pad to select High or Low detail. This
sets the starting detail mode of DOOM regardless of what you set it to
the last time you played.

TV Settings: Press A to enter the TV Settings sub-menu. This will be
disabled if you don't have a composite or RGB cable.

Cable: This just tells you what TV cable is detected.

Resolution: Hold A and use the d-pad to change the resolution. The
selections are 304x224, 608x448, and 576x448. The third resolution
assumes you are using a TV with 16:9 aspect ratio; it sets the output
to 768x480 and uses a 4:3 region within that display. The main reason
for having smaller screens for the TV is a smaller screen allows you to
compensate for overscan on the TV, and adjust the positioning so that it
looks good on YOUR TV. Note that the last two resolutions will be inter-
laced on the TV.

Sync to VBlank: Press A to toggle on or off. When on, the video refresh
waits for the vertical blank before flipping the screen.

Show FPS: Press A to toggle on or off. While on, the frames per second
will be shown in the upper-left corner of the screen. The number is the
FPS for the last second of play.

Detail: Hold A and use the d-pad to select High or Low detail. This
sets the starting detail mode of DOOM regardless of what you set it to
the last time you played.

Center Screen: Press A and the TV screen shows a calibration screen.
Use the d-pad to center the screen. Press A or B to exit back to the
menu.

Display: Shows whether the video output will be for a monitor or TV.
The cable plugged into the DC determines this setting.

Sound Settings: Press A to enter the Sound Settings sub-menu.

Sound Effects: Press A to toggle on or off. Disables all sound when
off.

Music: Press A to toggle on or off. Disables music when off, but not
sound effects.

Controller Settings: Press A to enter the Controller Settings sub-menu.

Jump Pack: Press A to toggle force-feedback on or off. When on, injuries
and firing projectile weapons will cause the controller to shake.

Disable Analog Stick: Press A to toggle on or off. When on, the
analog stick will not be used as a mouse inside the game.

Calibrate Analog Stick: Press A to enter the calibration routine.
You need to try to make the minimum and maximum values as small and
large, respectively, as possible, then allow the stick return to its
natural center. Press A or B to return to the previous menu.

Y + B Cheat: Hold A and use the d-pad to select a cheat to use when
the Y and B buttons are pressed inside the game.

Y + A Cheat: Hold A and use the d-pad to select a cheat to use when
the Y and A buttons are pressed inside the game.

Y + X Cheat: Hold A and use the d-pad to select a cheat to use when
the Y and X buttons are pressed inside the game.

File Settings: Press A to enter the File Settings sub-menu.

Main WAD: Press A to bring up the file requester. In the file requester,
press A to select a file or go into a directory. Press Y to back one
directory level. Press B to cancel the file requester without selecting
anything.

Patch WAD: Press A to bring up the file requester. There are four
patch WAD fields.

DEH File : Press A to bring up the file requester. There are four
deh file fields.

Game Settings: Press A to enter the Game Settings sub-menu.

No Monsters: Press A to toggle on or off. When enabled, monsters will
not appear in the game.

Respawn: Press A to toggle on or off. When enabled, monsters will
respawn at random intervals after they are killed.

Fast: Press A to toggle on or off. When enabled, monsters will be faster.

Turbo: Hold A and use the d-pad to change. This sets how fast the
player will be as a percent of the original value. 200 will make the
player twice as fast.

Map on HU: Press A to toggle on or off. When enabled, the map will
appear over the regular display instead of a black screen.

Rotate Map: Press A to toggle on or off. When enabled, the map will
rotate as the player turns.

Force Demo: Press A to toggle on or off. When on, it forces DOOM to
attempt to play demos from other versions.

Record / Play Demo / Time Demo: Hold A and use the d-pad to select the
number of the demo file to Record/Play/Time. Zero means no demo. If
you wish to time a demo, be sure to set Force Demo to on as well.

Network Settings: Press A to enter the Network Settings sub-menu.
This will be disabled if the network failed to initialize during the
startup.

Play Network Game: Press A to toggle on or off. Must be on to play
against others via the network.

Player Number: Hold A and use the d-pad to set your player number.
All players must have different numbers.

Deathmatch: Press A to toggle on or off. When on, the game runs in
deathmatch mode, and players will start in different locations and
you get frag stats at the end of the level. When off, the players all
start in the same place.

Alt Deathmatch: Press A to toggle on or off. When on, the game runs
in version 2 deathmatch mode where everything is doubled. This has
priority over the Deathmatch settings.

Starting Skill Level: Hold A and use the d-pad to select the skill
level you will start at.

Starting Map Level: Hold A and use the d-pad to select the map you
will start on. Values are 1 to 32 for Doom 2, 1 to 27 for Doom 1,
1 to 36 for the Ultimate Doom, and 1 to 9 for shareware Doom.

Timed Game: Hold A and use the d-pad to set the number of minutes the
level will last.

A.V.G.: Press A to toggle on or off. When on, the level will last 20
minutes.

Copy Stats: Press A to toggle on or off. When on, allows an external
statistics program to capture the intermission screen statistics.

Force Version: Hold A and use the d-pad to set the version of DOOM.
This can allow you to play against people with a different version of
DOOM.

Use PC Checksum: Press A to toggle on or off. When on, uses the same
method of computing the checksum for network packets as the PC version.
This may allow you to play against PC versions of Doom rather than just
other DCs. This should allow play with Doom for the PSP and SDL_Doom
v1.10 in linux, among other versions.

Network Port: Hold A and use the d-pad to change the port which will
be used to network the players.

Network Player #1: Press A to enter a network address for the first
network player via the OSK. This can be either an IP address, or a network
address name. If you do use a name, it CANNOT start with '-' or '.', or a
number.

Network Player #2: Press A to enter a network address for the second
network player via the OSK.

Network Player #3: Press A to enter a network address for the third
network player via the OSK.

Local Network Address: This shows the network address for this DC and
cannot be changed.

Network Extra Tic: Press A to toggle on or off. Extra Tic tells DOOM to
send the previous ticcmd along with the current one. This effectively
handles single packet losses without a delay, but doubles the bandwidth
of data transmitted.

Network Tic Dup: Hold A and use the d-pad to change. Valid range is
one to nine. Tic Dup tells DOOM to send ticcmds at a lower rate to
reduce network traffic. The normal rate is to send ticcmds 35 times per
second. Setting Tic Dup to '2' tells DOOM to send ticcmds 17.5 times per
second, and so forth. However, doing this reduces the accuracy of movement
for network players.

About: Press A to see the About screen.

Controls
========

START and Y act as alternate function selectors. Y also acts as BACKSPACE
when entering a savegame description, or as the 'y' key when you select QUIT.

The following controls are when you are not pressing START or Y:

Analog stick - mouse
D-Pad - forward/back/turn
LTRIGGER - strafe left
RTRIGGER - strafe right
A - fire
B - action
X - run

The following controls are when you are not pressing START, but pressing Y:

D-Pad UP - change gamma setting
D-Pad DOWN - toggle the detail setting
D-Pad Left - previous cheat
D-Pad Right - next cheat
LTRIGGER - previous weapon
RTRIGGER - next weapon
B - instant cheat 1
A - instant cheat 2
X - instant cheat 3

The following controls are when you are pressing START:

A - pause
B - menu
X - map
Y - enter text via On-Screen Keyboard (OSK)

The following controlls are when you are in the OSK:

Analog stick - select one of the nine squares
D-Pad UP - backspace
D-Pad DOWN - enter
LTRIGGER - toggle between letters and numbers
RTRIGGER - hold for shifted characters
A/B/X/Y - select the character corresponding to the button in the square
START - cancel

If you have a DC mouse or keyboard, Doom for the DC will use them like
they would be used on a PC.

WAD Files
=========

Main WAD files should be in the in the iwad directory, or a subdirectory
therein. While it is not a requirement, the requester starts in the
iwad directory.

Some older Registered WAD files may not work due to later changes made
to the WAD file contents.

It is possible to upgrade at least some old main WAD files using
freely available patches from ftp://ftp.idsoftware.com/idstuff/doom/.
You need a PC to apply the patches.

Playstation WAD files do not work as they are split by level and
compressed.

WAD files from games derived from DOOM (such as Heretic or Hexen)
do not work due to differences in WAD contents.

WAD files are identified by their name. A misnamed WAD file will
not work. Use the following to get the proper name:

Shareware Doom WAD = doom1.wad
Registered Doom WAD = doom.wad
The Ultimate DOOM WAD = doomu.wad
Doom 2 WAD = doom2.wad
Final Doom Plutonia WAD = plutonia.wad
Final Doom TNT WAD = tnt.wad

The FreeDoom WAD does not work because it is designed for BOOM, not
DOOM.

Patch WAD Files
===============

Patch WAD files are used to change graphics, sounds, and level
designs on DOOM; many are available on CDROM and the net.  You
can choose up to four patch WAD files with DOOM for the DC. It
is recommended that you put patch WAD files into the appropriate
subdirectory of the pwad directory. Patch WAD files are made for
a specific version of the DOOM WAD, so keep them straight. Again,
this is not a requirement. Doom for the DC uses full path names
for all files, so a file can be located anywhere on the CD. It's
just quicker and easier to use the recommended directories.

DeHackEd
========

DeHackEd is a PC program that patches certain internal tables inside
DOOM so that certain effects can be achieved; these include, a rapid
fire shotgun, floating barrels, and many other things.

Doom for the DC supports the use of DeHackEd configuration files;
they have a .DEH extension on them.  You may select up to four DEH
files. It is recommended that you put them in the deh directory, but
again, is not required.

Networking
==========

TCP/IP:
-------

TCP/IP networking in Doom for the DC is based on the Linux DOOM
source code.

To start a two player game of DOOM:

  1:  Make certain both players are using identical WAD files and
      game settings.

  2:  The first player should make sure Play Network Game is 'on',
      the Player Number is set to '1', and that Network Player #1
      is set to the network address of the second player.

  3:  The second player should make sure Play Network Game is 'on',
      the Player Number is set to '2', and that Network Player #1
      is set to the network address of the first player.

To start a three player game of DOOM:

  1:  Make certain all players are using identical WAD files and
      game settings.

  2:  The first player should make sure Play Network Game is 'on',
      the Player Number is set to '1', that Network Player #1 is
      set to the network address of the second player, and that
      Network Player #2 is set to the network address of the third
      player.

  3:  The second player should make sure Play Network Game is 'on',
      the Player Number is set to '2', that Network Player #1 is
      set to the network address of the first player, and that
      Network Player #2 is set to the network address of the third
      player.

  4:  The third player should make sure Play Network Game is 'on',
      the Player Number is set to '3', that Network Player #1 is
      set to the network address of the first player, and that
      Network Player #2 is set to the network address of the second
      player.

A four player game would be set up in a similar manner. Note that
network addresses may be either names or IP addresses. If you don't
know how to find the network name of a DC, consult someone else. I
haven't the slightest idea. The program gives you the local IP
address, so I'd suggest using that. Further note that although this
is intended for DC to DC networking, nothing stops you from playing
against someone running a compatible DOOM port on a computer hooked
to the network.

Cheats
======

There are two kinds of cheats: "instant" cheats and selected cheats.
Instant cheats are set up in the GUI. You press Y and one of the
other three main buttons to activate an instant cheat. Selected
cheats are always available. You hold Y and press RIGHT or LEFT
until you reach the cheat you wish, then release Y.

More...
=======

For more info on DOOM, check out the DOOM wiki.

    http://doom.wikia.com/wiki/Entryway

History
=======

v1.0 - 2009/07/25: First version for the DC.
