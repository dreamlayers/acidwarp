/* ACID WARP (c)Copyright 1992, 1993 by Noah Spurrier
 * All Rights reserved. Private Proprietary Source Code by Noah Spurrier
 * Ported to Linux by Steven Wills
 * Ported to X by Boris Gjenero
 */

#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif

#include <SDL.h>
#include <SDL_main.h>

#if !defined(WIN32) && !defined(EMSCRIPTEN)
#define HAVE_PALETTE
#endif
 
#include "warp_text.c"
#include "handy.h"
#include "acidwarp.h"
#include "lut.h"
#include "bit_map.h"
#include "palinit.h"
#include "rolnfade.h"
#include "display.h"
 
#define NUM_IMAGE_FUNCTIONS 40
#define NOAHS_FACE   0

/* there are WAY too many global things here... */
static int ROTATION_DELAY = 30000;
/* GraphicsContext *physicalscreen; */
static int show_logo = 1, image_time = 20;
int XMax = 319, YMax = 199;
UCHAR *buf_graf = NULL;
unsigned int buf_graf_stride = 0;
static int GO = TRUE;
int SKIP = FALSE;
static int NP = FALSE; /* flag indicates new palette */
static int LOCK = FALSE; /* flag indicates don't change to next image */
static UCHAR MainPalArray [256 * 3];
static UCHAR TargetPalArray [256 * 3];

/* Prototypes for forward referenced functions */
static void newpal(void);
static void printStrArray(char *strArray[]);
static void makeShuffledList(int *list, int listSize);
static void generate_image(int imageFuncNum, UCHAR *buf_graf,
                           int xcenter, int ycenter,
                           int xmax, int ymax,
                           int colormax, int stride);
static void commandline(int argc, char *argv[]);
static void mainLoop(void);

int main (int argc, char *argv[])
{
  RANDOMIZE();
  
  /* Default options */
  
  commandline(argc, argv);
  
  printf ("\nPlease wait...\n"
	  "\n\n*** Press Control-C to exit the program at any time. ***\n");
  printf ("\n\n%s\n", VERSION);
  
  disp_init();

  memset(MainPalArray, 0, sizeof(MainPalArray));
  disp_setPalette(MainPalArray);

#ifdef EMSCRIPTEN
  emscripten_set_main_loop(mainLoop, 1000000/ROTATION_DELAY, 1);
#else
  while(1) {
    mainLoop();
    usleep(ROTATION_DELAY);
  }
#endif
}

static void mainLoop(void) {
  static int imageFuncList[NUM_IMAGE_FUNCTIONS];
  static int paletteTypeNum = 0;
  static int imageFuncListIndex=0, fade_dir = TRUE;
  static time_t ltime, mtime;
  static enum {
    STATE_INITIAL,
    STATE_NEXT,
    STATE_FADEIN,
    STATE_ROTATE,
    STATE_FADEOUT_START,
    STATE_FADEOUT
  } state = STATE_INITIAL;

  disp_processInput();
  if (SKIP) {
    if (state == STATE_INITIAL) {
      show_logo = 0;
      SKIP = FALSE;
    } else {
      state = STATE_NEXT;
    }
  }

  switch (state) {
  case STATE_INITIAL:
    makeShuffledList(imageFuncList, NUM_IMAGE_FUNCTIONS);
    if (show_logo != 0) {
    /* show the logo for a while */
    disp_beginUpdate();
    writeBitmapImageToArray(buf_graf, NOAHS_FACE, XMax, YMax,
                            buf_graf_stride);
    disp_finishUpdate();
    initPalArray(TargetPalArray, RGBW_LIGHTNING_PAL);
    FadeCompleteFlag = FALSE; /* Fade-in needed next */
    goto logo_entry;
    }

    state = STATE_NEXT;
    /* Fall through */
  case STATE_NEXT:
    /* move to the next image */
    if (++imageFuncListIndex >= NUM_IMAGE_FUNCTIONS)
      {
	imageFuncListIndex = 0;
	makeShuffledList(imageFuncList, NUM_IMAGE_FUNCTIONS);
      }
    
    /* install a new image */
    disp_beginUpdate();
    generate_image(imageFuncList[imageFuncListIndex],
		           buf_graf, XMax/2, YMax/2, XMax, YMax,
		           255, buf_graf_stride);
    disp_finishUpdate();

    if (!SKIP) {
    /* create new palette */
    paletteTypeNum = RANDOM(NUM_PALETTE_TYPES +1);
    initPalArray(TargetPalArray, paletteTypeNum);
    FadeCompleteFlag = FALSE; /* Fade-in needed next */
    }
    SKIP = FALSE;
    
logo_entry:
    state = STATE_FADEIN;
    /* Fall through */
  case STATE_FADEIN:

    /* this is the fade in */
    if (!FadeCompleteFlag) {
      if(GO) {
	rolNFadeMainPalAryToTargNLodDAC(MainPalArray,TargetPalArray);
      }
      break;
    }
    
    ltime = time(NULL);
    mtime = ltime + image_time;
    
    state = STATE_ROTATE;
    /* Fall through */
  case STATE_ROTATE:
    /* rotate the palette for a while */
      if(GO)
	rollMainPalArrayAndLoadDACRegs(MainPalArray);
      if(NP) {
	newpal();
	NP = FALSE;
      }
      ltime=time(NULL);
      if((ltime>mtime) && !LOCK) {
	state = STATE_FADEOUT_START; /* Fall through */
      } else
	break;

  case STATE_FADEOUT_START:
    FadeCompleteFlag = FALSE;
    state = STATE_FADEOUT;
    /* Fall through */
  case STATE_FADEOUT:
    /* fade out */
    if (!FadeCompleteFlag) {
      if(GO) {
	if (fade_dir)
	  rolNFadeBlkMainPalArrayNLoadDAC(MainPalArray);
	else
	  rolNFadeWhtMainPalArrayNLoadDAC(MainPalArray);
      }
    } else {
      state = STATE_NEXT;
    }
  }
#if 0
  /* This was unreachable before */
  /* exit */
  printStrArray(Command_summary_string);
  printf("%s\n", VERSION);

  return 0;
#endif
}

/* ------------------------END MAIN----------------------------------------- */

static void newpal()
{
  int paletteTypeNum;
  
  paletteTypeNum = RANDOM(NUM_PALETTE_TYPES +1);
  initPalArray(MainPalArray, paletteTypeNum);

  disp_setPalette(MainPalArray);
}

void handleinput(int key)
{
  switch(key)
    {
    case 1:
      if(GO)
	GO = FALSE;
      else
	GO = TRUE;
      break;
    case 2:
      SKIP = TRUE;
      break;
    case 3:
	  SDL_Quit();
      exit(0);
      break;
    case 4:
      NP = TRUE;
      break;
    case 5:
      if(LOCK)
	LOCK = FALSE;
      else
	LOCK = TRUE;
      break;
    case 6:
      ROTATION_DELAY = ROTATION_DELAY - 5000;
      if (ROTATION_DELAY < 0)
	ROTATION_DELAY = 0;
      break;
    case 7:
      ROTATION_DELAY = ROTATION_DELAY + 5000;
      break;
    }
}

void handleInputChar(int c) {
  int r;
  
  switch (c) {
  case 'p':
  case 'P':        r = 1;     break;
  case 'n':
  case 'N':        r = 2;     break;
  case 'q':
  case 'Q':        r = 3;     break;
      
  case 'k':
  case 'K':        r = 4;     break;
  case 'l':
  case 'L':        r = 5;     break;
  default:
    return;
  }
  handleinput(r);
}

static void commandline(int argc, char *argv[])
{
  int argNum;

  /* Parse the command line */
  if (argc >= 2) {
    for (argNum = 1; argNum < argc; ++argNum) {
      if (!strcmp("-w",argv[argNum])) {
        printStrArray(The_warper_string);
        exit (0);
      } 
      else
      if (!strcmp("-h",argv[argNum])) {
        printStrArray(Help_string);
        printf("\n%s\n", VERSION);
        exit (0);
      }
      else
      if(!strcmp("-n",argv[argNum])) {
        show_logo = 0;
      }
      else
      if(!strcmp("-d",argv[argNum])) {
        if((argc-1) > argNum) {
          argNum++;
          image_time = atoi(argv[argNum]);
        }
      }
      else
      if(!strcmp("-s", argv[argNum])) {
        if((argc-1) > argNum) {
          argNum++;
          ROTATION_DELAY = atoi(argv[argNum]);
        }
      }
      else
	  {
	    fprintf(stderr, "Unknown option \"%s\"\n", argv[argNum]);
	    exit(-1);
	  }
    }
  }
}  

void printStrArray(char *strArray[])
{
  /* Prints an array of strings.  The array is terminated with a null string.     */
  char **strPtr;
  
  for (strPtr = strArray; **strPtr; ++strPtr)
    printf ("%s", *strPtr);
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

void generate_image(int imageFuncNum, UCHAR *buf_graf,
                    int xcenter, int ycenter,
                    int xmax, int ymax,
                    int colormax, int stride)
{
  
  /* WARNING!!! Major change from long to int.*/
  /* ### Changed back to long. Gives lots of warnings. Will fix soon. */
  
  long /* int */ x, y, dx, dy, dist, angle, xsize = xmax + 1;
  long color;
  
  /* Some general purpose random angles and offsets. Not all functions use them. */
  long x1,x2,x3,x4,y1,y2,y3,y4;
  
  x1 = RANDOM(40)-20;  x2 = RANDOM(40)-20;  x3 = RANDOM(40)-20;  x4 = RANDOM(40)-20;
  y1 = RANDOM(40)-20;  y2 = RANDOM(40)-20;  y3 = RANDOM(40)-20;  y4 = RANDOM(40)-20;
  
  for (y = 0; y <= ymax; ++y)
    {
      
      for (x = 0; x <= xmax; ++x)
	{
	  dx = x - xcenter;
	  dy = y - ycenter;
	  
	  dist  = lut_dist (dx, dy);
	  angle = lut_angle (dx, dy);
	  
	  /* select function. Could be made a separate function, but since 
             this function is evaluated over a large iteration of values I am 
             afraid that it might slow things down even more to have a 
             separate function.	*/
	  
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
	      color = lut_sin(lut_cos(2 * x * ANGLE_UNIT / xmax)) / (20 + dist)
                + lut_sin(lut_cos(2 * y * ANGLE_UNIT / ymax)) / (20 + dist);
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
		color = (  *(buf_graf + (xsize *  y   ) + (x-1))
			   + *(buf_graf + (xsize * (y-1)) +    x)) / 2
                  + RANDOM (16) - 8;
	      break;
	      
	    case 29:
	      if (y == 0 || x == 0)
		color = RANDOM (1024);
	      else
		color = dist/6 + (*(buf_graf + (xsize * y    ) + (x-1))
				  +  *(buf_graf + (xsize * (y-1)) +    x)) / 2
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
		color = (  *(buf_graf + (xsize *  y   ) + (x-1))
			   + *(buf_graf + (xsize * (y-1)) +  x   )  ) / 2;
	      
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
		color = (  *(buf_graf + (xsize *  y   ) + (x-1))
			   + *(buf_graf + (xsize * (y-1)) +  x   )  ) / 2;
	      
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
	  
	  /* Fit color value into the palette range using modulo.  It seems 
             that the Turbo-C MOD function does not behave the way I expect.
             It gives negative values for the MOD of a negative number.
             I expect MOD to function as it does on my HP-28S.
	   */

	  color = color % (colormax-1);
	  
	  if (color < 0)
	    color += (colormax - 1);
	  
	  ++color;
          /* color 0 is never used, so all colors are from 1 through 255 */
	  
	  *(buf_graf + (stride * y) + x) = (UCHAR)color;
          /* Store the color in the buffer */
	}
      /* end for (y = 0; y < ymax; ++y)	*/
    }
  /* end for (x = 0; x < xmax; ++x)	*/
  
#if 0	/* For diagnosis, put palette display line at top of new image	*/
  for (x = 0; x < xmax; ++x)
    {
      color = (x <= 255) ? x : 0;
      
      for (y = 0; y < 3; ++y)
	*(buf_graf + (xsize * y) + x) = (UCHAR)color;
    }
#endif
}
