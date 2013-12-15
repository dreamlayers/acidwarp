#include <stdlib.h>
#include <string.h>

#include "handy.h"
#include "acidwarp.h"
#include "rolnfade.h"
#include "palinit.h"
#include "display.h"

static int RedRollDirection = 0, GrnRollDirection = 0, BluRollDirection = 0;
static UINT FadeCompleteFlag = 0;
static UCHAR MainPalArray [256 * 3];
static UCHAR TargetPalArray [256 * 3];
static int paletteTypeNum = 0;
static int fade_dir = TRUE;

static void roll_rgb_palArray (UCHAR *MainpalArray);
static void maybeInvertSubPalRollDirection(void);
static int fadePalArrayToWhite (UCHAR *MainpalArray);
static int fadePalArrayToBlack (UCHAR *MainpalArray);
static int fadePalArrayToTarget (UCHAR *palArrayBeingChanged,
                                 UCHAR *targetPalArray);

static void rotatebackward(int color, UCHAR *Pal)
{
  int temp;
  int x;
  
  temp = Pal[((254)*3)+3+color];

  for(x=(254); x >= 1; --x)
    Pal[(x*3)+3+color] = Pal[(x*3)+color];
  Pal[(1*3)+color] = temp; 
  
}

static void rotateforward(int color, UCHAR *Pal)
{  
  int temp;
  int x;
  
  temp = Pal[(1*3)+color];
  for(x=1; x < (256) ; ++x)
    Pal[x*3+color] = Pal[(x*3)+3+color]; 
  Pal[((256)*3)-3+color] = temp;
}


static void rollMainPalArrayAndLoadDACRegs(UCHAR *MainPalArray)
{
        maybeInvertSubPalRollDirection();
        roll_rgb_palArray(MainPalArray);
		disp_setPalette(MainPalArray);
}


static void rolNFadeWhtMainPalArrayNLoadDAC(UCHAR *MainPalArray)
{
/* Fade to white, and keep the palette rolling while the fade is in progress.	*/
	if (!FadeCompleteFlag)
	{
		if (fadePalArrayToWhite(MainPalArray) == DONE)
			FadeCompleteFlag = 1;
                rollMainPalArrayAndLoadDACRegs(MainPalArray);
	}
}

static void rolNFadeBlkMainPalArrayNLoadDAC(UCHAR *MainPalArray)
{
/* Fade to black, and keep the palette rolling while the fade is in progress.   */
	if (!FadeCompleteFlag)
	{
		if (fadePalArrayToBlack (MainPalArray) == DONE)
			FadeCompleteFlag = 1;
                rollMainPalArrayAndLoadDACRegs(MainPalArray);
	}
}

static void rolNFadeMainPalAryToTargNLodDAC(UCHAR *MainPalArray, UCHAR *TargetPalArray)
{
/* Fade from one palette to a new palette, and keep the palette rolling while the fade is in progress.	*/
	if (!FadeCompleteFlag)
	{
		if (fadePalArrayToTarget (MainPalArray, TargetPalArray) == DONE)
			FadeCompleteFlag = 1;

		maybeInvertSubPalRollDirection();
		roll_rgb_palArray (  MainPalArray);
		roll_rgb_palArray (TargetPalArray);
		disp_setPalette(MainPalArray);
	}
	else
    rollMainPalArrayAndLoadDACRegs(MainPalArray);
}

/* WARNING! This is the function that handles the case of the SPECIAL PALETTE TYPE.
   This palette type is special in that there is no specific palette assigned to its
   palette number. Rather the palette is morphed from one static palette to another.
   The effect is quite interesting.
*/

static void rolNFadMainPalAry2RndTargNLdDAC(UCHAR *MainPalArray,
                                            UCHAR *TargetPalArray)
{
	if (fadePalArrayToTarget (MainPalArray, TargetPalArray) == DONE)
         initPalArray (TargetPalArray, RANDOM (NUM_PALETTE_TYPES));

	maybeInvertSubPalRollDirection();
	roll_rgb_palArray (  MainPalArray);
    roll_rgb_palArray (TargetPalArray);
    disp_setPalette(MainPalArray);
}

/**********************************************************************************/

/* These routines do the actual fading of a palette array to white, black,
	or to the values of another ("target") palette array.
*/

static int fadePalArrayToWhite (UCHAR *palArray)
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

static int fadePalArrayToBlack (UCHAR *palArray)
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

static int fadePalArrayToTarget (UCHAR *palArrayBeingChanged,
                                 UCHAR *targetPalArray)
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
static void roll_rgb_palArray(UCHAR *Pal)
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

static void maybeInvertSubPalRollDirection(void)
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

void newPalette(void)
{
  paletteTypeNum = RANDOM(NUM_PALETTE_TYPES + 1);
  if (paletteTypeNum >= NUM_PALETTE_TYPES) {
    /* Beginning special morphing palette */
    initPalArray(TargetPalArray, RANDOM(NUM_PALETTE_TYPES));
  } else {
    /* Fading to specific constant palette */
    initPalArray(TargetPalArray, paletteTypeNum);
    FadeCompleteFlag = FALSE; /* Fade-in needed next */
  }
}

/* This does not return status because a constantly
 * morphing palette never finishes morphing.
 */
void fadeInAndRotate(void)
{
  if (paletteTypeNum == NUM_PALETTE_TYPES) {
    rolNFadMainPalAry2RndTargNLdDAC(MainPalArray,TargetPalArray);
  } else if (!FadeCompleteFlag) {
    rolNFadeMainPalAryToTargNLodDAC(MainPalArray,TargetPalArray);
  } else {
    rollMainPalArrayAndLoadDACRegs(MainPalArray);
  }
}

void beginFadeOut(int toblack)
{
  if (toblack || RANDOM(2) == 0) {
    fade_dir = 1;
  } else {
    fade_dir = 0;
  }
  FadeCompleteFlag = 0;
}

/* Returns 1 when faded completely to white or black */
int fadeOut(void)
{
  if (fade_dir) {
    rolNFadeBlkMainPalArrayNLoadDAC(MainPalArray);
  } else {
    rolNFadeWhtMainPalArrayNLoadDAC(MainPalArray);
  }
  return FadeCompleteFlag;
}

/* Normally, palette is applied automatically. This only needs to be
 * called from outside rolnfade.c when a new SDL surface is created
 * and colour cycling is paused.
 */
void applyPalette(void)
{
  disp_setPalette(MainPalArray);
}

void initRolNFade(int logo)
{
  if (logo) {
    initPalArray(MainPalArray, RGBW_LIGHTNING_PAL);
    memcpy(TargetPalArray, MainPalArray, sizeof(TargetPalArray));
  } else {
    memset(MainPalArray, 0, sizeof(MainPalArray));
  }
  applyPalette();
}
