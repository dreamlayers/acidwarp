/* ACID WARP (c)Copyright 1992, 1993 by Noah Spurrier
 * All Rights reserved. Private Proprietary Source Code by Noah Spurrier
 * Ported to Linux by Steven Wills
 * Ported to SDL by Boris Gjenero
 */

#ifndef __APPLE__
#include <malloc.h>
#endif
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
 
#include "handy.h"
#include "acidwarp.h"
#include "lut.h"
#include "bit_map.h"
#include "palinit.h"
#include "rolnfade.h"
#include "display.h"

#include "warp_text.c"
 
#define NUM_IMAGE_FUNCTIONS 40
#define NOAHS_FACE   0

/* there are WAY too many global things here... */
static int ROTATION_DELAY = 30000;
/* GraphicsContext *physicalscreen; */
static int show_logo = 1, image_time = 20;
static int floating_point = 1, normalize = 1;
static int disp_flags = 0;
static int ready_to_draw = 0;
static int width = 320, height = 200;
UCHAR *buf_graf = NULL;
unsigned int buf_graf_stride = 0;
static int GO = TRUE;
static int SKIP = FALSE;
static int NP = FALSE; /* flag indicates new palette */
static int LOCK = FALSE; /* flag indicates don't change to next image */
static int imageFuncList[NUM_IMAGE_FUNCTIONS];
static int imageFuncListIndex=0;
#ifndef EMSCRIPEN
SDL_bool drawnext = SDL_FALSE;
SDL_cond *drawnext_cond;
SDL_bool drawdone = SDL_FALSE;
SDL_cond *drawdone_cond;
SDL_mutex *draw_mtx;
static int drawingthread(void *param);
#endif /* !EMSCRITPEN */

/* Prototypes for forward referenced functions */
static void printStrArray(char *strArray[]);
static void makeShuffledList(int *list, int listSize);
static void generate_image(int imageFuncNum, UCHAR *buf_graf,
                           int xcenter, int ycenter,
                           int width, int height,
                           int colormax, int pitch);
static void commandline(int argc, char *argv[]);
static void mainLoop(void);
static void redraw(void);

void quit(int retcode)
{
  disp_quit();
  SDL_Quit();
  exit(retcode);
}

void fatalSDLError(const char *msg)
{
  fprintf(stderr, "SDL error while %s: %s", msg, SDL_GetError());
  quit(-1);
}

#ifndef EMSCRIPTEN
static Uint32 timerProc(Uint32 interval, void *param)
{
  SDL_CondSignal((SDL_cond *)param);
  return ROTATION_DELAY / 1000;
}
#endif /* !EMSCRIPTEN */

int main (int argc, char *argv[])
{
#ifdef EMSCRIPTEN
#if !SDL_VERSION_ATLEAST(2,0,0)
  /* https://dreamlayers.blogspot.ca/2015/04/optimizing-emscripten-sdl-1-settings.html
   * SDL.defaults.opaqueFrontBuffer = false; wouldn't work because alpha
   * values are not set.
   */
  EM_ASM({
    SDL.defaults.copyOnLock = false;
    SDL.defaults.discardOnLock = true;
    SDL.defaults.opaqueFrontBuffer = false;
    Module.screenIsReadOnly = true;
  });
#endif
#else /* !EMSCRIPTEN */
  SDL_cond *cond;
  SDL_mutex *mutex;
#endif /* !EMSCRIPTEN */

  /* Initialize SDL */
  if ( SDL_Init(SDL_INIT_VIDEO
#ifndef EMSCRIPTEN
                | SDL_INIT_TIMER
#endif
                ) < 0 ) {
    fprintf(stderr, "Failed to initialize SDL: %s\n", SDL_GetError());
#if SDL_VERSION_ATLEAST(2,0,0)
    /* SDL 2 docs say this is safe, but SDL 1 docs don't. */
    SDL_Quit();
#endif
    return -1;
  }

  RANDOMIZE();
  
  /* Default options */
  
  commandline(argc, argv);
  
  printf ("\nPlease wait...\n"
	  "\n\n*** Press Control-C to exit the program at any time. ***\n");
  printf ("\n\n%s\n", VERSION);
  
  disp_init(width, height, disp_flags);

#ifdef EMSCRIPTEN
  emscripten_set_main_loop(mainLoop, 1000000/ROTATION_DELAY, 1);
#else /* !EMSCRIPTEN */
  mutex = SDL_CreateMutex();
  if (mutex == NULL) {
    fatalSDLError("creating mutex");
  }
  cond = SDL_CreateCond();
  if (cond == NULL) {
    fatalSDLError("creating condition variable");
  }
  draw_mtx = SDL_CreateMutex();
  drawdone_cond = SDL_CreateCond();
  drawnext_cond = SDL_CreateCond();
  if (SDL_AddTimer(ROTATION_DELAY / 1000, timerProc, cond) == 0) {
    fatalSDLError("adding timer");
  }
  SDL_CreateThread(drawingthread, "DrawingThread", NULL);
  while(1) {
    mainLoop();
    if (SDL_LockMutex(mutex) != 0) {
      fatalSDLError("locking mutex");
    }
    if (SDL_CondWait(cond, mutex) != 0) {
      fatalSDLError("waiting on condition");
    }
  }
#endif /* !EMSCRIPTEN */
}

static void mainLoop(void)
{
  static time_t ltime, mtime;
  static enum {
    STATE_INITIAL,
    STATE_NEXT,
    STATE_DISPLAY,
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

  if(NP) {
    if (!show_logo) newPalette();
    NP = FALSE;
  }

  switch (state) {
  case STATE_INITIAL:
    makeShuffledList(imageFuncList, NUM_IMAGE_FUNCTIONS);
    ready_to_draw = 1;
    if (show_logo != 0) {
      /* Begin showing logo here. Logo continues to be shown
       * in STATE_DISPLAY, like any other image.
       */
      redraw();
      initRolNFade(1);
      ltime = time(NULL);
      mtime = ltime + image_time;
      state = STATE_DISPLAY;
      break;
    } else {
      initRolNFade(0);
    }

    state = STATE_NEXT;
    /* Fall through */
  case STATE_NEXT:
    /* install a new image */
    redraw();

    if (!SKIP) {
      newPalette();
    }
    SKIP = FALSE;
    
    ltime = time(NULL);
    mtime = ltime + image_time;
    state = STATE_DISPLAY;
    /* Fall through */
  case STATE_DISPLAY:
    /* rotate the palette for a while */
    if(GO) {
      fadeInAndRotate();
    }

    ltime=time(NULL);
    if(ltime > mtime && !LOCK) {
      /* Transition from logo only fades to black,
       * like the first transition in Acidwarp 4.10.
       */
      beginFadeOut(show_logo);
      state = STATE_FADEOUT;
    }
    break;

  case STATE_FADEOUT:
    /* fade out */
    if(GO) {
      if (fadeOut()) {
        state = STATE_NEXT;
      }
    }
    break;
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

void handleinput(enum acidwarp_command cmd)
{
  switch(cmd)
    {
    case CMD_PAUSE:
      if(GO)
	GO = FALSE;
      else
	GO = TRUE;
      break;
    case CMD_SKIP:
      SKIP = TRUE;
      break;
    case CMD_QUIT:
      quit(0);
      break;
    case CMD_NEWPAL:
      NP = TRUE;
      break;
    case CMD_LOCK:
      if(LOCK)
	LOCK = FALSE;
      else
	LOCK = TRUE;
      break;
    case CMD_PAL_FASTER:
      ROTATION_DELAY = ROTATION_DELAY - 5000;
      if (ROTATION_DELAY < 0)
	ROTATION_DELAY = 0;
      break;
    case CMD_PAL_SLOWER:
      ROTATION_DELAY = ROTATION_DELAY + 5000;
      break;
    }
}

/* Drawing code which runs on separate thread */
static void draw(void) {
  disp_beginUpdate();
  if (show_logo) {
    writeBitmapImageToArray(buf_graf, NOAHS_FACE, width, height,
                            buf_graf_stride);
  } else {
    if (floating_point) {
      generate_image_float(imageFuncList[imageFuncListIndex],
                           buf_graf, width/2, height/2, width, height,
                           256, buf_graf_stride, normalize);
    } else {
      generate_image(imageFuncList[imageFuncListIndex],
                     buf_graf, width/2, height/2, width, height,
                     256, buf_graf_stride);
    }
  }
  disp_finishUpdate();
}

/* Drawing runs on separate thread, so as soon as one image is drawn to the
 * screen, the next can be computed in the background. This way, typical
 * image changes won't need to wait for drawing computations, which can be
 * slow when generating large images using floating point.
 */
static int drawingthread(void *param) {
  while (1) {
    /* Draw next image to back buffer */
    draw();

    /* Tell main thread that image is drawn */
    SDL_LockMutex(draw_mtx);
    drawdone = SDL_TRUE;
    SDL_CondSignal(drawdone_cond);

    /* Wait for next image */
    while (!drawnext) {
      SDL_CondWait(drawnext_cond, draw_mtx);
    }
    drawnext = SDL_FALSE;
    SDL_UnlockMutex(draw_mtx);

    /* move to the next image */
    show_logo = 0;
    if (++imageFuncListIndex >= NUM_IMAGE_FUNCTIONS) {
      imageFuncListIndex = 0;
      makeShuffledList(imageFuncList, NUM_IMAGE_FUNCTIONS);
    }
  }
  return 0;
}

void redraw(void) {
  /* Wait for image to finish drawing image */
  SDL_LockMutex(draw_mtx);
  while (!drawdone) {
    SDL_CondWait(drawdone_cond, draw_mtx);
  }
  drawdone = SDL_FALSE;

  /* This should actually display what the thread drew */
  disp_swapBuffers();

  drawnext = SDL_TRUE;
  /* Tell drawing thread it can continue now that buffers are swapped */
  SDL_CondSignal(drawnext_cond);
  SDL_UnlockMutex(draw_mtx);
}

void handleresize(int newwidth, int newheight)
{
    width = newwidth;
    height = newheight;
    if (ready_to_draw) {
      redraw();
      applyPalette();
    }
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
      if(!strcmp("-f",argv[argNum])) {
        disp_flags |= DISP_FULLSCREEN;
      }
      else
      if(!strcmp("-k",argv[argNum])) {
        disp_flags |= DISP_DESKTOP_RES_FS;
      }
      else
      if(!strcmp("-o",argv[argNum])) {
        floating_point = 0;
      }
      else
      if(!strcmp("-u",argv[argNum])) {
        normalize = 0;
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

/* Fixed point image generator using lookup tables goes here */
#define mod(x, y) ((x) % (y))
#define xor(x, y) ((x) ^ (y))
#include "gen_img.c"
