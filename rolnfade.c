#include <dos.h>
#include <stdlib.h>
#include <string.h>

#include "handy.h"
#include "acidwarp.h"

/* Globals for use by the background roll functions */
int RedRollDirection = 0, GrnRollDirection = 0, BluRollDirection = 0;
UINT  FadeCompleteFlag;

UCHAR MainPalArray  [256 * 3];
UCHAR TargetPalArray  [256 * 3];

/**********************************************************************************/

/* These functions are called by the timer ISR; the first one is also called by some of the others.	*/
  
void rollMainPalArrayAndLoadDACRegs (void)
{
        maybeInvertSubPalRollDirection();
        roll_rgb_palArray(MainPalArray);
  loadAllDACRegs    (MainPalArray);
}

void rolNFadeWhtMainPalArrayNLoadDAC (void)
{							/* Fade to white, and keep the palette rolling while the fade is in progress.	*/
	if (!FadeCompleteFlag)
	{
		if (fadePalArrayToWhite(MainPalArray) == DONE)
			FadeCompleteFlag = 1;
    rollMainPalArrayAndLoadDACRegs();
	}
}

void rolNFadeBlkMainPalArrayNLoadDAC (void)
{							/* Fade to black, and keep the palette rolling while the fade is in progress.	*/
	if (!FadeCompleteFlag)
	{
		if (fadePalArrayToBlack (MainPalArray) == DONE)
			FadeCompleteFlag = 1;
    rollMainPalArrayAndLoadDACRegs();
	}
}

void rolNFadeMainPalAryToTargNLodDAC (void)
{		/* Fade from one palette to a new palette, and keep the palette rolling while the fade is in progress.	*/
	if (!FadeCompleteFlag)
	{
		if (fadePalArrayToTarget (MainPalArray, TargetPalArray) == DONE)
			FadeCompleteFlag = 1;

		maybeInvertSubPalRollDirection();
		roll_rgb_palArray (  MainPalArray);
		roll_rgb_palArray (TargetPalArray);
    loadAllDACRegs    (  MainPalArray);
	}
	else
    rollMainPalArrayAndLoadDACRegs();
}

/* WARNING! This is the function that handles the case of the SPECIAL PALETTE TYPE.
   This palette type is special in that there is no specific palette assigned to its
   palette number. Rather the palette is morphed from one static palette to another.
   The effect is quite interesting.
*/
void rolNFadMainPalAry2RndTargNLdDAC (void)
{
	if (fadePalArrayToTarget (MainPalArray, TargetPalArray) == DONE)
         initPalArray (TargetPalArray, RANDOM (NUM_PALETTE_TYPES));

	maybeInvertSubPalRollDirection();
	roll_rgb_palArray (  MainPalArray);
	roll_rgb_palArray (TargetPalArray);
  loadAllDACRegs    (  MainPalArray);
}

/**********************************************************************************/

/* These routines do the actual fading of a palette array to white, black,
	or to the values of another ("target") palette array.
*/

int fadePalArrayToWhite (UCHAR *palArray)
{													/* Returns DONE if the entire palette is white, else NOT_DONE	 */
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

void roll_rgb_palArray (UCHAR *palArray)
{
	int palRegNum, palByteNum;
	UCHAR tempRed, tempGrn, tempBlu;

	tempRed = (!RedRollDirection) ? palArray[3] : palArray[765];
	tempGrn = (!GrnRollDirection) ? palArray[4] : palArray[766];
	tempBlu = (!BluRollDirection) ? palArray[5] : palArray[767];

	for (palRegNum = 1; palRegNum < 255; ++palRegNum)
{
		palByteNum = palRegNum * 3;

		if (!RedRollDirection) palArray[      palByteNum] = palArray[      palByteNum + 3];
		                  else palArray[768 - palByteNum] = palArray[765 - palByteNum    ];
		++palByteNum;

		if (!GrnRollDirection) palArray[      palByteNum] = palArray[      palByteNum + 3];
								else palArray[770 - palByteNum] = palArray[767 - palByteNum    ];
		++palByteNum;

		if (!BluRollDirection) palArray[      palByteNum] = palArray[      palByteNum + 3];
								else palArray[772 - palByteNum] = palArray[769 - palByteNum    ];
}

	if (!RedRollDirection) palArray[765] = tempRed; else palArray[3] = tempRed;
	if (!GrnRollDirection) palArray[766] = tempGrn; else palArray[4] = tempGrn;
	if (!BluRollDirection) palArray[767] = tempBlu; else palArray[5] = tempBlu;
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

