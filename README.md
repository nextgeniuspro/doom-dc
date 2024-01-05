# Doom-DC
Doom for Sega Dreamcast

## Description

This is a port of Doom for the Sega Dreamcast home video game console. It allows you to play the classic first-person shooter Doom on Dreamcast hardware.

## Build

To build Doom-DC:

1. Install the KallistiOS SDK and development tools for Dreamcast development.

2. Clone this repository:

```
git clone https://github.com/yevgeniy-logachev/Doom-DC.git
``` 

3. Run `make all` in the repo directory to build the executable.

4. The built `doom.elf` executable will be output to the root directory.

5. To make a CD image, run `make self` in the repo directory. Image disc.cdi will be output to the 'disc' directory.

6. Burn this to a CD by using DiscJuggler or transfer it to a Dreamcast via SD card and DreamShell.


