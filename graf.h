#ifndef __graf
#define __graf 1

#include "handy.h"

extern int XMax, YMax, ColorMax;

/* Initialize graf system for the requested video mode */
int  setParamsForNewVideoMode (int reqdVideoMode);

/* Fire up the currently defined mode. Call setParams...Mode() first. */
void setNewVideoMode (void);

/* Restore the old video mode */
void restoreOldVideoMode(void);

/* Clear the graphics page */
void graf_clear (void);

/* Write a pixel color to the coordinates */
void writePixel (int x, int y, int color);

/* Load all the DAC color registers */
void loadAllDACRegs(UCHAR far * pal);

/* Load just one register */
void loadOneDACReg(int color, UCHAR r, UCHAR g, UCHAR b);

/* Returns the probable display type. I.e. check for VGA, EGA, etc.
This function really sucks, but this is a non-trivial task unless VESA
is installed.
*/
int  check_display(void);

#endif
