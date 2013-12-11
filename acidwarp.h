/* ACIDWARP.H */

#define VERSION "Acid Warp Version 4.10 (C)Copyright 1992, 1993 by Noah Spurrier and Mark Bilk"

/* OBJECTS_LIST = acidwarp + bit_map + graf + int_hook + lut + palinit + rolnfade + warp_txt	*/

#if _MSC_VER != 700 /* Needed only until I upgrade my old version of MSC  */
	#define __interrupt	interrupt
	#define __far			far
	#define __cdecl		_cdecl
	#define __asm			_asm

	#define VOID			void

	#define _FP_SEG		FP_SEG
	#define _FP_OFF		FP_OFF

	#define _getch			getch
	#define _inp			inp
	#define _kbhit			kbhit
	#define _movedata		movedata
	#define _outp			outp
#endif


/* HANDY	***********************************************************************/


/* ACIDWARP.C	*****************************************************************/

void  __interrupt __far newCtrlCBrkInt23Svc();
void  __interrupt __far newKbdHdweInt09Svc ();
int generate_image (int function, UCHAR far * buf_graf, int xcenter, int ycenter,
              int xmax, int ymax, int colormax);
void makeShuffledList(int *list, int listSize);
void printStrArray(char *strArray[]);


/* BIT_MAP.C	*****************************************************************/

#define X_TITLE		80
#define Y_TITLE		98/*92*/

#define NOAHS_FACE	0

void bit_map_uncompress (UCHAR far *buf_graf, UCHAR far *bit_data, int x_map, int y_map, int xmax, int ymax);
void writeBitmapImageToArray(UCHAR far *buf_graf, int image_number, int xmax, int ymax);


/* GRAF.C	********************************************************************/

#include "graf.h"

/* INT_HOOK.C	*****************************************************************/

#define NORMAL_DOS_TICK_FREQ		0

extern UINT	newTickFreqHz;
extern int	TimerDelayCountdown, backgroundPaletteProcessNum;

void __interrupt _far newTimerHdweInt08Svc(void);
void beg_timer_hook(void);
void end_timer_hook(void);
void beepOn(UINT divisor);
void beepOff(void);


/* LUT.C	***********************************************************************/

/* PALINIT.C	*****************************************************************/
/* Palette types  */
#define RGBW_PAL          0 
#define W_PAL						1
#define W_HALF_PAL				2
#define PASTEL_PAL				3
#define RGBW_LIGHTNING_PAL		4
#define W_LIGHTNING_PAL			5
#define W_HALF_LIGHTNING_PAL	6
#define PASTEL_LIGHTNING_PAL	7

#define NUM_PALETTE_TYPES	8

void initPalArray (UCHAR *pal, int pal_type);
void add_sparkles_to_palette (UCHAR *palArray, int sparkle_amount);

void init_rgbw_palArray					(UCHAR *pal);
void init_rgbw_lightning_palArray	(UCHAR *pal);
void init_w_palArray						(UCHAR *pal);
void init_w_lightning_palArray		(UCHAR *pal);
void init_w_half_palArray				(UCHAR *pal);
void init_w_half_lightning_palArray	(UCHAR *pal);
void init_pastel_palArray				(UCHAR *pal);
void init_pastel_lightning_palArray	(UCHAR *pal);


/* ROLNFADE.C	*****************************************************************/

#define DIRECTN_CHANGE_PERIOD_IN_TICKS		256

#define BLACK_FADE			0	/* Fade methods	*/
#define WHITE_FADE			1

#define NUM_FADE_METHODS	2

extern UINT  FadeCompleteFlag;
extern UCHAR MainPalArray  [256 * 3];
extern UCHAR TargetPalArray [256 * 3];

#define NONE										0	/* Background palette process numbers	*/
#define ROLL										1
#define ROLL_AND_FADE_WHITE					2
#define ROLL_AND_FADE_BLACK					3
#define ROLL_AND_FADE_TO_TARGET				4
#define ROLL_AND_FADE_TO_RANDOM_TARGET		5

void rollMainPalArrayAndLoadDACRegs  (void);  /* Background palette process routines  */
void rolNFadeWhtMainPalArrayNLoadDAC (void);
void rolNFadeBlkMainPalArrayNLoadDAC (void);
void rolNFadeMainPalAryToTargNLodDAC (void);
void rolNFadMainPalAry2RndTargNLdDAC (void);

int  fadePalArrayToWhite  (UCHAR *palArray);
int  fadePalArrayToBlack  (UCHAR *palArray);
int  fadePalArrayToTarget (UCHAR *palArrayBeingChanged, UCHAR *targetPalArray);

void roll_rgb_palArray    (UCHAR *palArray);

void maybeInvertSubPalRollDirection(void);


/* WARP_TXT.C	*****************************************************************/

extern char *Command_summary_string[];
extern char *Help_string[];
extern char *The_warper_string[];

