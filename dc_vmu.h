/*
	dc_vmu.h
	This file is from the Wolf4SDL\DC project with
    some changes by Chilly Willy.
	It defines the VMU save icon.

	Cyle Terry <cyle.terry@gmail.com>
*/

void StatusDrawLCD(uint8 *source);
int DC_SaveToVMU(char *src, int tp);
int DC_LoadFromVMU(char *dst);
