#ifndef DEBUG32X_H
#define DEBUG32X_H

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

extern void DebugDC_Init(int vmode, int pmode);
extern void DebugDC_SetFGColor(int r, int g, int b);
extern void DebugDC_SetBGColor(int r, int g, int b);
extern int DebugDC_ScreenGetX();
extern int DebugDC_ScreenGetY();
extern void DebugDC_ScreenClear();
extern void DebugDC_ScreenSetXY(int x, int y);
extern void DebugDC_ScreenPutChar(int x, int y, unsigned char ch);
extern void DebugDC_ScreenClearLine(int Y);
extern int DebugDC_ScreenPrintData(const char *buff, int size);
extern int DebugDC_ScreenPuts(const char *str);
extern void DebugDC_Delay(int ticks);

#define printf(format, args...) 		\
    do {					\
	char str[256];				\
	snprintf(str, 256, format , ## args);	\
	str[255] = '\0';			\
	DebugDC_ScreenPuts(str);		\
    } while (0)

#endif
