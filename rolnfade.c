#include <stdlib.h>
#include <string.h>

#include "handy.h"
#include "acidwarp.h"
#include "rolnfade.h"
#include "palinit.h"
#include <vga.h>
#include <vgagl.h>

int RedRollDirection = 0, GrnRollDirection = 0, BluRollDirection = 0;
UINT FadeCompleteFlag = 0;

/*
extern UCHAR MainPalArray  [256 * 3];
extern UCHAR TargetPalArray  [256 * 3];
extern UCHAR TempPalArray [256 * 3];
*/

void rotatebackward(int color, UCHAR *Pal)
{
  int temp;
  int x;
  
  temp = Pal[((254)*3)+3+color];

  for(x=(254); x >= 1; --x)
    Pal[(x*3)+3+color] = Pal[(x*3)+color];
  Pal[(1*3)+color] = temp; 
  
}

void rotateforward(int color, UCHAR *Pal)
{  
  int temp;
  int x;
  
  temp = Pal[(1*3)+color];
  for(x=1; x < (256) ; ++x)
    Pal[x*3+color] = Pal[(x*3)+3+color]; 
  Pal[((256)*3)-3+color] = temp;
}


void rollMainPalArrayAndLoadDACRegs(UCHAR *MainPalArray)
{
        maybeInvertSubPalRollDirection();
        roll_rgb_palArray(MainPalArray);
        gl_setpalettecolors(0, 255, MainPalArray);
}


void rolNFadeWhtMainPalArrayNLoadDAC(UCHAR *MainPalArray)
{
/* Fade to white, and keep the palette rolling while the fade is in progress.	*/
	if (!FadeCompleteFlag)
	{
		if (fadePalArrayToWhite(MainPalArray) == DONE)
			FadeCompleteFlag = 1;
                rollMainPalArrayAndLoadDACRegs(MainPalArray);
	}
}

void rolNFadeBlkMainPalArrayNLoadDAC(UCHAR *MainPalArray)
{
/* Fade to black, and keep the palette rolling while the fade is in progress.   */
	if (!FadeCompleteFlag)
	{
		if (fadePalArrayToBlack (MainPalArray) == DONE)
			FadeCompleteFlag = 1;
                rollMainPalArrayAndLoadDACRegs(MainPalArray);
	}
}

void rolNFadeMainPalAryToTargNLodDAC(UCHAR *MainPalArray, UCHAR *TargetPalArray)
{
/* Fade from one palette to a new palette, and keep the palette rolling while the fade is in progress.	*/
	if (!FadeCompleteFlag)
	{
		if (fadePalArrayToTarget (MainPalArray, TargetPalArray) == DONE)
			FadeCompleteFlag = 1;

		maybeInvertSubPalRollDirection();
		roll_rgb_palArray (  MainPalArray);
		roll_rgb_palArray (TargetPalArray);
	}
	else
    rollMainPalArrayAndLoadDACRegs(MainPalArray);
}

/* WARNING! This is the function that handles the case of the SPECIAL PALETTE TYPE.
   This palette type is special in that there is no specific palette assigned to its
   palette number. Rather the palette is morphed from one static palette to another.
   The effect is quite interesting.
*/

void rolNFadMainPalAry2RndTargNLdDAC(UCHAR *MainPalArray, UCHAR *TargetPalArray)
{
	if (fadePalArrayToTarget (MainPalArray, TargetPalArray) == DONE)
         initPalArray (TargetPalArray, RANDOM (NUM_PALETTE_TYPES));

	maybeInvertSubPalRollDirection();
	roll_rgb_palArray (  MainPalArray);
	roll_rgb_palArray (TargetPalArray);
        gl_setpalettecolors(0, 256, MainPalArray);
}

/**********************************************************************************/

/* These routines do the actual fading of a palette array to white, black,
	or to the values of another ("target") palette array.
*/

int fadePalArrayToWhite (UCHAR *palArray)
{
/* Returns DONE if the entire palette is white, else NOT_DONE */

	int palByteNum, num_white = 0;

	for (palByteNum = 3; palByteNum < 768; ++palByteNum)
	{
		if (palArray[palByteNum] < 63)	/* Increment every color in the palette array until it becomes white.	*/
			++palArray[palByteNum];
		else
			++num_white;
	}

	return ((num_white >= 765) ? DONE : NOT_DONE);
}

int fadePalArrayToBlack (UCHAR *palArray)
{	/* Returns DONE if the entire palette is black, else NOT_DONE	 */
	int palByteNum, num_black = 0;

	for (palByteNum = 3; palByteNum < 768; ++palByteNum)
	{
		if (palArray[palByteNum] > 0)		/* Decrement every color in the palette array until it becomes black.	*/
			--palArray[palByteNum];
		else
			++num_black;
	}

	return ((num_black >= 765) ? DONE : NOT_DONE);
}

/* Increments (fades) every color in palArrayBeingChanged closer to the corresponding color in targetPalArray.	*/

int fadePalArrayToTarget (UCHAR *palArrayBeingChanged, UCHAR *targetPalArray)
{													/* Returns DONE if the two palette arrays are equal, else NOT_DONE.	*/
	int palByteNum, num_equal = 0;

	for (palByteNum = 3; palByteNum < 768; ++palByteNum)
	{
		if   (palArrayBeingChanged[palByteNum] < targetPalArray[palByteNum])
			 ++palArrayBeingChanged[palByteNum];
    else if (palArrayBeingChanged[palByteNum] > targetPalArray[palByteNum])
			 --palArrayBeingChanged[palByteNum];
		else
			++num_equal;
	}

	return ((num_equal >= 765) ? DONE : NOT_DONE);
}

/**********************************************************************************/

/* Rolls the R, G, and B components of the palette ONE place in the direction specified by r, g, and b.	*/
void roll_rgb_palArray(UCHAR *Pal)
{
    if (!RedRollDirection)
         rotateforward(RED,Pal);
    else
         rotatebackward(RED,Pal);

    if(!GrnRollDirection)
         rotateforward(GREEN,Pal);
    else
         rotatebackward(GREEN,Pal);

    if(!BluRollDirection)
         rotateforward(BLUE,Pal);
    else
         rotatebackward(BLUE,Pal);
}

/* This routine switches the current direction of one of the sub-palettes (R, G, or B) with probability
 * 1/DIRECTN_CHANGE_PERIOD_IN_TICKS, when it is called by the timer ISR.  Only one color direction can change at a time.
 */

void maybeInvertSubPalRollDirection(void)
{
  switch (RANDOM(DIRECTN_CHANGE_PERIOD_IN_TICKS))
	{
		case 0 :
			RedRollDirection = !RedRollDirection;
		break;

		case 1 :
			GrnRollDirection = !GrnRollDirection;
		break;

		case 2 :
			BluRollDirection = !BluRollDirection;
		break;
	}
}

