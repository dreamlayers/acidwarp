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
#include "display.h"

#define NUM_IMAGE_FUNCTIONS 40
#define NOAHS_FACE   0

static int imageFuncList[NUM_IMAGE_FUNCTIONS];
static int imageFuncListIndex=0;
static int flags = 0;
#ifdef ENABLE_THREADS
static SDL_bool drawnext = SDL_FALSE;
static SDL_cond *drawnext_cond = NULL;
static SDL_bool drawdone = SDL_FALSE;
static SDL_cond *drawdone_cond = NULL;
static SDL_mutex *draw_mtx = NULL;
static int drawing_main(void *param);
static SDL_Thread *drawing_thread = NULL;
int abort_draw = 0;
static int redraw_same = 0;
#endif /* !EMSCRITPEN */

/* Prototypes for forward referenced functions */
static void generate_image(int imageFuncNum, UCHAR *buf_graf,
                           int xcenter, int ycenter,
                           int width, int height,
                           int colormax, int pitch);

/* Drawing code which runs on separate thread */
static void draw(int which) {
  UCHAR *buf_graf;
  unsigned int buf_graf_stride, width, height;
  disp_beginUpdate(&buf_graf, &buf_graf_stride, &width, &height);
  if (which < 0) {
    writeBitmapImageToArray(buf_graf, NOAHS_FACE, width, height,
                            buf_graf_stride);
  } else {
    if (flags & DRAW_FLOAT) {
      generate_image_float(which,
                           buf_graf, width/2, height/2, width, height,
                           256, buf_graf_stride, flags & DRAW_SCALED);
    } else {
      generate_image(which,
                     buf_graf, width/2, height/2, width, height,
                     256, buf_graf_stride);
    }
  }
  disp_finishUpdate();
}

static void makeShuffledList(int *list, int listSize)
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

/* Drawing runs on separate thread, so as soon as one image is drawn to the
 * screen, the next can be computed in the background. This way, typical
 * image changes won't need to wait for drawing computations, which can be
 * slow when generating large images using floating point.
 */
static int drawing_main(void *param) {
  int displayed_img;
  int draw_img = (flags & DRAW_LOGO) ? -1 : imageFuncList[imageFuncListIndex];
  displayed_img = draw_img;
  while (1) {
    /* Draw next image to back buffer */
    draw(draw_img);

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

    if (redraw_same) {
      draw_img = displayed_img;
      redraw_same = 0;
    } else {
      /* move to the next image */
      flags &= ~DRAW_LOGO;
      if (++imageFuncListIndex >= NUM_IMAGE_FUNCTIONS) {
        imageFuncListIndex = 0;
        makeShuffledList(imageFuncList, NUM_IMAGE_FUNCTIONS);
      }
      displayed_img = draw_img;
      draw_img = imageFuncList[imageFuncListIndex];
    }
  }
  return 0;
}

void draw_abort(void) {
  if (drawing_thread != NULL) {
    SDL_LockMutex(draw_mtx);
    while (!drawdone) {
      abort_draw = 1;
      SDL_CondWait(drawdone_cond, draw_mtx);
      abort_draw = 0;
    }
    drawdone = SDL_FALSE;
    SDL_UnlockMutex(draw_mtx);
  }
}

void draw_next(void) {
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

void draw_same(void) {
  redraw_same = 1;
  drawnext = SDL_TRUE;
  SDL_CondSignal(drawnext_cond);
  SDL_UnlockMutex(draw_mtx);
  draw_next();
}

void draw_init(int draw_flags) {
  flags = draw_flags;
  draw_mtx = SDL_CreateMutex();
  drawdone_cond = SDL_CreateCond();
  drawnext_cond = SDL_CreateCond();
  drawing_thread = SDL_CreateThread(drawing_main,
#if SDL_VERSION_ATLEAST(2,0,0)
                                    "DrawingThread",
#endif
                                    NULL);
  /* TODO check SDL errors */
  makeShuffledList(imageFuncList, NUM_IMAGE_FUNCTIONS);
}

void draw_quit(void) {
  draw_abort();
  /* TODO free stuff here */
}

/* Fixed point image generator using lookup tables goes here */
#define mod(x, y) ((x) % (y))
#define xor(x, y) ((x) ^ (y))
#include "gen_img.c"
