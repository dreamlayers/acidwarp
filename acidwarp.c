/* ACID WARP (c)Copyright 1992, 1993 by Noah Spurrier
 * All Rights reserved. Private Proprietary Source Code by Noah Spurrier
 */

#include <dos.h>
#include <conio.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "handy.h"
#include "lut.h"
#include "acidwarp.h"

#define NUM_IMAGE_FUNCTIONS 40

int ExitFlag = 0;
void (__interrupt __far *origKbdHdweBiosIntVect09)();
void (__interrupt __far *origCtrlCBrkDosIntVect23)();

void main (int argc, char *argv[])
{
  int imageFuncList[NUM_IMAGE_FUNCTIONS], userOptionImageFuncNum;
	int paletteTypeNum, userPaletteTypeNumOptionFlag;
  int delay_time, argNum, imageFuncListIndex, dosCtrlC_ChkFlag, firstTimeFlag;
	UCHAR huge *buf_graf;
/*  time_t ltime;*/
	
	beepOff();

  RANDOMIZE();
/*
  time(&ltime);
  ltime gets a long that increments once per second
  srand((UINT)ltime);
  a call to srand() starts a different sequence of rand() outputs
*/
  
  /* Default options */
	delay_time = 20;
  userPaletteTypeNumOptionFlag = 0;       /* User Palette option is OFF */
  userOptionImageFuncNum = -1; /* No particular functions goes first. */
  
	/* Parse the command line */
	if (argc >= 2)
	{
		for (argNum = 1; argNum < argc; ++argNum)
		{
			switch (*(argv[argNum]))
			{
        case 'D' :
        case 'd' :  /* Set delay time */
					delay_time = ABS(atoi(argv[argNum] + 1));
				break;
  
				/* Set User Palette Option ON and set paletteTypeNum choice.  If there is an error
               then this option is set back to the default. The User is not warned.  */
        case 'P' :
        case 'p' :
					userPaletteTypeNumOptionFlag = 1;
					paletteTypeNum = atoi (argv[argNum] + 1);
  
					if (paletteTypeNum < 0 || paletteTypeNum > NUM_PALETTE_TYPES)
						userPaletteTypeNumOptionFlag = 0;
				break;
  
        case 'F' :
        case 'f' :
					userOptionImageFuncNum = atoi (argv[argNum] + 1);
				break;

        case 'W' :
        case 'w' :  /* NOTE! This case exits the program! */
					printStrArray(The_warper_string);
					exit (0);
				break;

				case '?' :	/* NOTE! This case exits the program! */
					printStrArray(Help_string);
					printStrArray(Command_summary_string);
					printf ("%s\n", VERSION);
					exit (0);
	break;

				default :	/* NOTE! The default case exits the program! */
					printStrArray(Command_summary_string);
					printf ("%s\n", VERSION);
					exit(0);
	break; 
    }
    }
  } 
  
	setParamsForNewVideoMode(0x13);
  
	buf_graf = (UCHAR huge *) _fmalloc((UINT)XMax * (UINT)YMax);
  
	if (buf_graf == NULL)
      {
		/* printf ("\nFree Memory>%lu\n", _ffree());	*/
		printf ("\nYou don't have enough memory!\n");
		printf ("%s\n", VERSION);
		exit(1);
      }
    
  printf ("\nPlease wait...\n"
			  "\n\n*** CTRL-BREAK will exit the program at any time. ***\n");
	printf ("\n\n%s\n", VERSION);
    
	origKbdHdweBiosIntVect09 = _dos_getvect(0x09);
	origCtrlCBrkDosIntVect23 = _dos_getvect(0x23);
    
	_disable();											/* Save the old DOS Ctrl-C Check Flag	*/
		__asm mov ah, 0x33	__asm mov al, 0x00	__asm Int 0x21			__asm mov dosCtrlC_ChkFlag, dx

															/* Set the DOS Ctrl-C Check Flag to ON	*/
		__asm mov ah, 0x33	__asm mov al, 0x01	__asm mov dl, 0x01	__asm Int 0x21

		_dos_setvect(0x09, newKbdHdweInt09Svc);
		_dos_setvect(0x23, newCtrlCBrkInt23Svc);
	_enable(); 
    
	/* Generate and display title screen */
	memset (buf_graf, 0x00, (size_t)(XMax * YMax));
    
	writeBitmapImageToArray ((UCHAR far *)buf_graf, NOAHS_FACE, XMax, YMax);
	setNewVideoMode();
    
	initPalArray (  MainPalArray, RGBW_LIGHTNING_PAL);
	initPalArray (TargetPalArray, RGBW_LIGHTNING_PAL);
  loadAllDACRegs( MainPalArray);
    
	_movedata (FP_SEG(buf_graf), FP_OFF(buf_graf), 0xa000, 0x0000, (UINT)XMax * (UINT)YMax);
	backgroundPaletteProcessNum = ROLL;								/* Start rolling palette	*/
	beg_timer_hook ();

	/* Wait about 10 seconds after showing my face.  Let the awe sink in.	*/
	TimerDelayCountdown = 10 * 18; /* Should be 18.6, but avoid float */

	while (TimerDelayCountdown && !ExitFlag)
		if (_kbhit())
{
			_getch();
			break;
}

	for (firstTimeFlag = 1, imageFuncListIndex = NUM_IMAGE_FUNCTIONS; !ExitFlag; firstTimeFlag = 0)
	{
		if (++imageFuncListIndex >= NUM_IMAGE_FUNCTIONS)
{
			imageFuncListIndex = 0;
			makeShuffledList(imageFuncList, NUM_IMAGE_FUNCTIONS);
  }

		if (generate_image(  (userOptionImageFuncNum < 0) ? imageFuncList[imageFuncListIndex] : userOptionImageFuncNum,
                   buf_graf, XMax/2, YMax/2, XMax, YMax, ColorMax)
			)
			break;		/* Exit flag is set by Ctrl-Brk/Ctrl-C ISRs and causes generate_image() to return 1 immediately.	*/

		FadeCompleteFlag = 0;											/* Stop rolling palette	*/
																				/* Start fading palette to white or black	*/
    switch (firstTimeFlag ? BLACK_FADE : RANDOM (NUM_FADE_METHODS))
    {
			case BLACK_FADE:
				backgroundPaletteProcessNum = ROLL_AND_FADE_BLACK;
      break;

			case WHITE_FADE:
				backgroundPaletteProcessNum = ROLL_AND_FADE_WHITE;
      break;

			default:
				FadeCompleteFlag = 1;
      break;
    }

		while (!FadeCompleteFlag)
			;																	/* Stop (done) fadeing palette	*/
																				/* Move new image to screen		*/
		_movedata (_FP_SEG(buf_graf), _FP_OFF(buf_graf), 0xa000, 0x0000, (size_t)(XMax * YMax));

		if (!userPaletteTypeNumOptionFlag)
      paletteTypeNum = RANDOM (NUM_PALETTE_TYPES + 1);

		if (paletteTypeNum == NUM_PALETTE_TYPES)					/* Start fading palette toward random palette types	*/
			backgroundPaletteProcessNum = ROLL_AND_FADE_TO_RANDOM_TARGET;
		else
{
			FadeCompleteFlag = 0;
			initPalArray (TargetPalArray, paletteTypeNum);
																				/* Start fading palette toward new palette type	*/
			backgroundPaletteProcessNum = ROLL_AND_FADE_TO_TARGET;

			while (!FadeCompleteFlag)
				;
																				/* Stop (done) fading palette toward new palette type	*/ 
			backgroundPaletteProcessNum = ROLL;						/* Start rolling palette	*/
        }
																/* Should be 18.6, but integer 18 avoids floating point library.	*/
		for (TimerDelayCountdown = delay_time * 18; TimerDelayCountdown && !ExitFlag; )
			if (_kbhit())
    {
				_getch();
      break;
    }
    }
  /* endfor (;!Exit_Flag;) */

	end_timer_hook();				/* This needs to be done before restoreOldVideoMode() */
	restoreOldVideoMode();
  
  printStrArray(Command_summary_string);
	printf("%s\n", VERSION);
  
	_disable();                         /* Restore the old DOS Ctrl-C Check Flag  */
		__asm mov ah, 0x33	__asm mov al, 0x01	__asm mov dx, dosCtrlC_ChkFlag	__asm Int 0x21

		_dos_setvect (0x09, origKbdHdweBiosIntVect09);
		_dos_setvect (0x23, origCtrlCBrkDosIntVect23);
	_enable();
  
	beepOff();
}

		/* If the user selected a specific paletteTypeNum then that will be used, else a random paletteTypeNum
         will be chosen. The +1 is used to allow the SPECIAL PALETTE TYPE to be chosen. This is a dynamic
         palette whose number is always one higher than the number of static palette types.	*/

		/* NUM_PALETTE_TYPES is set only to the number of *STATIC* palettes. If the
	 	* paletteTypeNum equals this number then that signifies that the SPECIAL
	 	* PALETTE TYPE should be used instead of any of the static types.
	 	*/

void __interrupt __far newCtrlCBrkInt23Svc()
{
	ExitFlag = 1;
}

void __interrupt __far newKbdHdweInt09Svc()
{
	(*origKbdHdweBiosIntVect09)();
  
	if (*((char far *) 0x00400071))		/* if BIOS BREAK KEY flag is set */
    {
		*((char far *)0x00400071) = 0;	/* reset the BIOS BREAK KEY flag */
      
		ExitFlag = 1;
    }
}


int generate_image(int imageFuncNum, UCHAR far *buf_graf, int xcenter, int ycenter,
             int xmax, int ymax, int colormax)
{		/* Exits immediately and returns 1 if exitFlag is set by Ctrl-Brk/Ctrl-C ISRs, else returns 0	*/
  
  /* WARNING!!! Major change from long to int.*/
  /* ### Changed back to long. Gives lots of warnings. Will fix soon. */
  
  long /* int */ x, y, dx, dy, dist, angle;
  long color;
  
  /* Some general purpose random angles and offsets. Not all functions use them. */
  long a1, a2, a3, a4, x1,x2,x3,x4,y1,y2,y3,y4;
  
  x1 = RANDOM(40)-20;  x2 = RANDOM(40)-20;  x3 = RANDOM(40)-20;  x4 = RANDOM(40)-20;
  y1 = RANDOM(40)-20;  y2 = RANDOM(40)-20;  y3 = RANDOM(40)-20;  y4 = RANDOM(40)-20;
  
  a1 = RANDOM(ANGLE_UNIT);  a2 = RANDOM(ANGLE_UNIT);  a3 = RANDOM(ANGLE_UNIT);  a4 = RANDOM(ANGLE_UNIT);

  for (y = 0; y < ymax; ++y)
    {
		if (ExitFlag)
			return (1);

    /* ### Got rid of progress flag option
    if (progressFlag)
			writePixel (x, 0, x % (colormax-1) +1);
    */
      
      for (x = 0; x < xmax; ++x)
	{
	  dx = x - xcenter;
	  dy = y - ycenter;
	  
	  dist  = lut_dist (dx, dy);
	  angle = lut_angle (dx, dy);
	  
			/* select function. Could be made a separate function, but since this function is
            evaluated over a large iteration of values I am afraid that it might slow
            things down even more to have a separate function.									*/
	  
	  switch (imageFuncNum)
	    {
	      /* case -1:	Eight Arm Star -- produces weird discontinuity
                color = dist+ lut_sin(angle * (200 - dist)) / 32;
						break;
						*/
	    case 0: /* Rays plus 2D Waves */
	      color = angle + lut_sin (dist * 10) / 64 +
		lut_cos (x * ANGLE_UNIT / xmax * 2) / 32 +
		lut_cos (y * ANGLE_UNIT / ymax * 2) / 32;
	      break;
	      
	    case 1:	/* Rays plus 2D Waves */
	      color = angle + lut_sin (dist * 10) / 16 +
		lut_cos (x * ANGLE_UNIT / xmax * 2) / 8 +
		lut_cos (y * ANGLE_UNIT / ymax * 2) / 8;
	      break;
	      
	    case 2:
	      color = lut_sin (lut_dist(dx + x1, dy + y1) *  4) / 32 +
		lut_sin (lut_dist(dx + x2, dy + y2) *  8) / 32 +
		lut_sin (lut_dist(dx + x3, dy + y3) * 16) / 32 +
		lut_sin (lut_dist(dx + x4, dy + y4) * 32) / 32;
	      break;
	      
	    case 3:	/* Peacock */
	      color = angle + lut_sin (lut_dist(dx + 20, dy) * 10) / 32 +
		angle + lut_sin (lut_dist(dx - 20, dy) * 10) / 32;
	      break;
	      
	    case 4:
	      color = lut_sin (dist) / 16;
	      break;
	      
	    case 5:	/* 2D Wave + Spiral */
	      color = lut_cos (x * ANGLE_UNIT / xmax) / 8 +
		lut_cos (y * ANGLE_UNIT / ymax) / 8 +
		angle + lut_sin(dist) / 32;
	      break;
	      
	    case 6:	/* Peacock, three centers */
	      color = lut_sin (lut_dist(dx,      dy - 20) * 4) / 32+
		lut_sin (lut_dist(dx + 20, dy + 20) * 4) / 32+
		lut_sin (lut_dist(dx - 20, dy + 20) * 4) / 32;
	      break;
	      
	    case 7:	/* Peacock, three centers */
	      color = angle +
		lut_sin (lut_dist(dx,      dy - 20) * 8) / 32+
		lut_sin (lut_dist(dx + 20, dy + 20) * 8) / 32+
		lut_sin (lut_dist(dx - 20, dy + 20) * 8) / 32;
	      break;
	      
	    case 8:	/* Peacock, three centers */
	      color = lut_sin (lut_dist(dx,      dy - 20) * 12) / 32+
		lut_sin (lut_dist(dx + 20, dy + 20) * 12) / 32+
		lut_sin (lut_dist(dx - 20, dy + 20) * 12) / 32;
	      break;
	      
	    case 9:	/* Five Arm Star */
	      color = dist + lut_sin (5 * angle) / 64;
	      break;
	      
	    case 10:	/* 2D Wave */
	      color = lut_cos (x * ANGLE_UNIT / xmax * 2) / 4 +
		lut_cos (y * ANGLE_UNIT / ymax * 2) / 4;
	      break;
	      
	    case 11:	/* 2D Wave */
	      color = lut_cos (x * ANGLE_UNIT / xmax) / 8 +
		lut_cos (y * ANGLE_UNIT / ymax) / 8;
	      break;
	      
	    case 12:	/* Simple Concentric Rings */
	      color = dist;
	      break;
	      
	    case 13:	/* Simple Rays */
	      color = angle;
	      break;
	      
	    case 14:	/* Toothed Spiral Sharp */
	      color = angle + lut_sin(dist * 8)/32;
	      break;
	      
	    case 15:	/* Rings with sine */
	      color = lut_sin(dist * 4)/32;
	      break;
	      
	    case 16:	/* Rings with sine with sliding inner Rings */
	      color = dist+ lut_sin(dist * 4) / 32;
	      break;
	      
	    case 17:
					color = lut_sin(lut_cos(2 * x * ANGLE_UNIT / xmax)) / (20 + dist) +
						  	  lut_sin(lut_cos(2 * y * ANGLE_UNIT / ymax)) / (20 + dist);
	      break;
	      
	    case 18:	/* 2D Wave */
	      color = lut_cos(7 * x * ANGLE_UNIT / xmax)/(20 + dist) +
		lut_cos(7 * y * ANGLE_UNIT / ymax)/(20 + dist);
	      break;
	      
	    case 19:	/* 2D Wave */
	      color = lut_cos(17 * x * ANGLE_UNIT/xmax)/(20 + dist) +
		lut_cos(17 * y * ANGLE_UNIT/ymax)/(20 + dist);
	      break;
	      
	    case 20:	/* 2D Wave Interference */
	      color = lut_cos(17 * x * ANGLE_UNIT / xmax) / 32 +
		lut_cos(17 * y * ANGLE_UNIT / ymax) / 32 + dist + angle;
	      break;
	      
	    case 21:	/* 2D Wave Interference */
	      color = lut_cos(7 * x * ANGLE_UNIT / xmax) / 32 +
		lut_cos(7 * y * ANGLE_UNIT / ymax) / 32 + dist;
	      break;
	      
	    case 22:	/* 2D Wave Interference */
	      color = lut_cos( 7 * x * ANGLE_UNIT / xmax) / 32 +
		lut_cos( 7 * y * ANGLE_UNIT / ymax) / 32 +
		lut_cos(11 * x * ANGLE_UNIT / xmax) / 32 +
		lut_cos(11 * y * ANGLE_UNIT / ymax) / 32;
	      break;
	      
	    case 23:
	      color = lut_sin (angle * 7) / 32;
	      break;
	      
	    case 24:
	      color = lut_sin (lut_dist(dx + x1, dy + y1) * 2) / 12 +
		lut_sin (lut_dist(dx + x2, dy + y2) * 4) / 12 +
		lut_sin (lut_dist(dx + x3, dy + y3) * 6) / 12 +
		lut_sin (lut_dist(dx + x4, dy + y4) * 8) / 12;
	      break;
	      
	    case 25:
	      color = angle + lut_sin (lut_dist(dx + x1, dy + y1) * 2) / 16 +
		angle + lut_sin (lut_dist(dx + x2, dy + y2) * 4) / 16 +
		lut_sin (lut_dist(dx + x3, dy + y3) * 6) /  8 +
		lut_sin (lut_dist(dx + x4, dy + y4) * 8) /  8;
	      break;
	      
	    case 26:
	      color = angle + lut_sin (lut_dist(dx + x1, dy + y1) * 2) / 12 +
		angle + lut_sin (lut_dist(dx + x2, dy + y2) * 4) / 12 +
		angle + lut_sin (lut_dist(dx + x3, dy + y3) * 6) / 12 +
		angle + lut_sin (lut_dist(dx + x4, dy + y4) * 8) / 12;
	      break;
	      
	    case 27:
	      color = lut_sin (lut_dist(dx + x1, dy + y1) * 2) / 32 +
		lut_sin (lut_dist(dx + x2, dy + y2) * 4) / 32 +
		lut_sin (lut_dist(dx + x3, dy + y3) * 6) / 32 +
		lut_sin (lut_dist(dx + x4, dy + y4) * 8) / 32;
	      break;

	    case 28:	/* Random Curtain of Rain (in strong wind) */
	      if (y == 0 || x == 0)
		color = RANDOM (16);
	      else
		color = (  *(buf_graf + (xmax *  y   ) + (x-1))
			   + *(buf_graf + (xmax * (y-1)) +    x)) / 2
                  + RANDOM (16) - 8;
	      break;
	      
	    case 29:
	      if (y == 0 || x == 0)
		color = RANDOM (1024);
	      else
		color = dist/6 + (*(buf_graf + (xmax * y    ) + (x-1))
				  +  *(buf_graf + (xmax * (y-1)) +    x)) / 2
		+ RANDOM (16) - 8;
	      break;
	      
	    case 30:
	      color = lut_sin (lut_dist(dx,     dy - 20) * 4) / 32 ^
		lut_sin (lut_dist(dx + 20,dy + 20) * 4) / 32 ^
		lut_sin (lut_dist(dx - 20,dy + 20) * 4) / 32;
	      break;
	      
	    case 31:
	      color = (angle % (ANGLE_UNIT/4)) ^ dist;
	      break;
	      
	    case 32:
	      color = dy ^ dx;
	      break;
	      
	    case 33:	/* Variation on Rain */
	      if (y == 0 || x == 0)
		color = RANDOM (16);
	      else
		color = (  *(buf_graf + (xmax *  y   ) + (x-1))
			   + *(buf_graf + (xmax * (y-1)) +  x   )  ) / 2;
	      
	      color += RANDOM (2) - 1;
	      
	      if (color < 64)
		color += RANDOM (16) - 8;
	      else
						color = color;
	      break; 
	      
	    case 34:	/* Variation on Rain */
	      if (y == 0 || x == 0)
		color = RANDOM (16);
	      else
		color = (  *(buf_graf + (xmax *  y   ) + (x-1))
			   + *(buf_graf + (xmax * (y-1)) +  x   )  ) / 2;
	      
	      if (color < 100)
		color += RANDOM (16) - 8;
	      break;
	      
	    case 35:
	      color = angle + lut_sin(dist * 8)/32;
	      dx = x - xcenter;
	      dy = (y - ycenter)*2;
	      dist  = lut_dist (dx, dy);
          angle = lut_angle (dx, dy);
          color = (color + angle + lut_sin(dist * 8)/32) / 2;
	  break;
	  
	    case 36:
	      color = angle + lut_sin (dist * 10) / 16 +
		lut_cos (x * ANGLE_UNIT / xmax * 2) / 8 +
		lut_cos (y * ANGLE_UNIT / ymax * 2) / 8;
	      dx = x - xcenter;
	      dy = (y - ycenter)*2;
	      dist  = lut_dist (dx, dy);
	      angle = lut_angle (dx, dy);
	      color = (color + angle + lut_sin(dist * 8)/32) / 2;
	      break;
	      
	    case 37:
	      color = angle + lut_sin (dist * 10) / 16 +
		lut_cos (x * ANGLE_UNIT / xmax * 2) / 8 +
		lut_cos (y * ANGLE_UNIT / ymax * 2) / 8;
	      dx = x - xcenter;
	      dy = (y - ycenter)*2;
	      dist  = lut_dist (dx, dy);
          angle = lut_angle (dx, dy);
          color = (color + angle + lut_sin (dist * 10) / 16 +
		   lut_cos (x * ANGLE_UNIT / xmax * 2) / 8 +
		   lut_cos (y * ANGLE_UNIT / ymax * 2) / 8)  /  2;
	  break;
	  
	    case 38:
	      if (dy%2)
		{
		  dy *= 2;
		  dist  = lut_dist (dx, dy);
		  angle = lut_angle (dx, dy);
		}
	      color = angle + lut_sin(dist * 8)/32;
	      break;
	      
	    case 39:
	      color = (angle % (ANGLE_UNIT/4)) ^ dist;
	      dx = x - xcenter;
	      dy = (y - ycenter)*2;
	      dist = lut_dist (dx, dy);
	      angle = lut_angle (dx, dy);
	      color = (color +  ((angle % (ANGLE_UNIT/4)) ^ dist)) / 2;
	      break;
	      
	    case 40:
	      color = dy ^ dx;
	      dx = x - xcenter;
	      dy = (y - ycenter)*2;
	      color = (color +  (dy ^ dx)) / 2;
	      break;

	    default:
	      color = RANDOM (colormax - 1) + 1;
	      break;
	    }
	  
			/* Fit color value into the palette range using modulo.  It seems that the
			 * Turbo-C MOD function does not behave the way I expect. It gives negative values
			 * for the MOD of a negative number. I expect MOD to function as it does on my HP-28S.
	   */

	  color = color % (colormax-1);
	  
	  if (color < 0)
	    color += (colormax - 1);
	  
			++color; /* color 0 is never used, so all colors are from 1 through 255 */
	  
			*(buf_graf + (xmax * y) + x) = (UCHAR)color;	/* Store the color in the buffer */
	}
      /* end for (y = 0; y < ymax; ++y)	*/
    }
  /* end for (x = 0; x < xmax; ++x)	*/
  
#if 0	/* For diagnosis, put palette display line at top of new image	*/
  for (x = 0; x < xmax; ++x)
    {
      color = (x <= 255) ? x : 0;
      
      for (y = 0; y < 3; ++y)
	*(buf_graf + (xmax * y) + x) = (UCHAR)color;
    }
#endif
  
  return (0);
}


void makeShuffledList(int *list, int listSize)
{
	int entryNum, r;

	for (entryNum = 0; entryNum < listSize; ++entryNum)
		list[entryNum] = -1;

	for (entryNum = 0; entryNum < listSize; ++entryNum)
	{
		do
      r = RANDOM(listSize);
		while (list[r] != -1);

		list[r] = entryNum;
	}
}

void printStrArray(char *strArray[])
{					/* Prints an array of strings.  The array is terminated with a null string.	*/
	char **strPtr;

	for (strPtr = strArray; **strPtr; ++strPtr)
		printf ("%s", *strPtr);
}

