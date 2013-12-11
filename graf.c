#include <dos.h>
#include <conio.h>
#include "handy.h"
#include "graf.h"

#define DAC_RD_IDX      0x3C7
#define DAC_WR_IDX      0x3C8
#define DAC_DATA        0x3C9

int XMax, YMax, ColorMax;
int OldVideoMode = -1;
int NewVideoMode = -1;

struct videoMode
{
	int mode, XMax, YMax, ColorMax;
}	VideoModeTab[] = 	{
							{ 0x0d, 320, 200,  16 },
							{ 0x0e, 640, 200,  16 },
							{ 0x0f, 640, 350,   1 },
							{ 0x10, 640, 350,   4 },
							{ 0x11, 640, 480,   1 },
							{ 0x12, 640, 480,  16 },
							{ 0x13, 320, 200, 256 },
							{ 0x6a, 800, 600,  16 },

							{ -1 }	/* The Terminator	*/
						};


int setParamsForNewVideoMode (int reqdVideoMode)
{
	int modeFound = 0;
	struct videoMode *videoModePtr;

	for (videoModePtr = VideoModeTab; videoModePtr->mode > -1; ++videoModePtr)
		if (videoModePtr->mode == reqdVideoMode)
		{
			NewVideoMode = videoModePtr->mode;
			XMax         = videoModePtr->XMax;
			YMax         = videoModePtr->YMax;
			ColorMax     = videoModePtr->ColorMax;
	
			modeFound = 1;
			break;
		}

	return (!modeFound);
}

void setNewVideoMode (void)
{
	__asm mov ah, 0x0f	__asm Int 0x10		__asm and ax, 0x00ff		__asm mov OldVideoMode, ax

	__asm mov ah, 0x00	__asm mov al, BYTE PTR NewVideoMode			__asm Int 0x10

}

void restoreOldVideoMode (void)
{
	if (OldVideoMode >= 0)
	{
		__asm mov ah, 0x00
		__asm mov al, BYTE PTR OldVideoMode
		__asm Int 0x10
	}

	OldVideoMode = -1;
}

/* I can't figure out how to directly clear the page, so I
	* instruct the graphics card to preserve the palette and then set
	* the graphics mode to the current graphics mode.
	*/
void graf_clear ()
{
	if (NewVideoMode != -1)
	{
		/* Disable default pallete loading on mode set */
		__asm mov ah, 0x12
		__asm mov bl, 0x31
		__asm mov al, 0x01  /* 0x01 to disable, 0x00 to enable palette init */
		__asm Int 0x10

		/* Set mode to the current mode */
		__asm mov ah, 0x00
		__asm mov al, BYTE PTR NewVideoMode
		__asm Int 0x10

		/* ###BUG This might not be the default before Acid Warp started... */
		/* Renable default pallete loading on mode set */
		__asm mov ah, 0x12
		__asm mov bl, 0x31
		__asm mov al, 0x00
		__asm Int 0x10
	}
}

void writePixel(int x, int y, int color)
{
	__asm mov ah, 0x0c
	__asm mov al, BYTE PTR color
	__asm mov bh, 0x00
	__asm mov cx, x
	__asm mov dx, y
	__asm Int 0x10
}

void loadAllDACRegs(UCHAR far *pal)
{
	int segment_pal, offset_pal;

	/* Get segment and offset of far pointer */
	segment_pal = _FP_SEG (pal);
	offset_pal  = _FP_OFF (pal);

	/* Crude Sync -- There is still a small amount of snow,
	almost unnoticeable. The sync does not get rid of the snow
	flakes, but it does bunch them all up into a single snow ball
	at the top right corner of the screen.
	*/

	while ( ! (_inp (0x03da) & 0x08))	/* Wait for Vertical Sync to begin.							*/
		;											/* The remainder of Vert Blanking occurs after this.	*/

	_asm mov dx, DAC_WR_IDX
	_asm mov ax, 0x0000 /* startRegNum */
	_asm out dx, al

	_asm mov cx, 0x0300

	_asm cld
	_asm mov si, offset_pal /* DAC Buf */
	_asm mov dx, DAC_DATA
	_asm push ds
	_asm mov ds, segment_pal
	_asm rep outsb
	_asm pop ds
}

void loadOneDACReg (int color, UCHAR r, UCHAR g, UCHAR b)
{
	__asm mov ax, 0x1010
	__asm mov bx, color
	__asm mov dh, r
	__asm mov ch, g
	__asm mov cl, b
	__asm Int 0x10
}

int check_display (void)
{					/* If BX were examined after the call, this could test display config	*/
	int _AL;

	__asm mov ah, 0x1a
	__asm mov al, 0x00
	__asm Int 0x10
	__asm and ax, 0x00ff
	__asm mov _AL, ax

	if (_AL != 0x1a)
		return 1;
	else
		return 0;
}

