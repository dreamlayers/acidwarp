/* ACID WARP (c)Copyright 1992, 1993 by Noah Spurrier
 * All Rights reserved. Private Proprietary Source Code by Noah Spurrier
 * Ported to Linux by Steven Wills
 * Ported to SDL by Boris Gjenero
 */

#include <SDL.h>

#include "handy.h"
#include "acidwarp.h"
#include "bit_map.h"
#include "display.h"

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
int quit_draw = 0;
static int redraw_same = 0;
#else /* !ENABLE_THREADS */
static int draw_first = 0;
#endif /* !ENABLE_THREADS */

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

static void draw_advance(void)
{
  flags &= ~DRAW_LOGO;
  if (++imageFuncListIndex >= NUM_IMAGE_FUNCTIONS) {
    imageFuncListIndex = 0;
    makeShuffledList(imageFuncList, NUM_IMAGE_FUNCTIONS);
  }
}

#ifdef ENABLE_THREADS
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

    if (quit_draw) break;

    if (redraw_same) {
      draw_img = displayed_img;
      redraw_same = 0;
    } else {
      /* move to the next image */
      draw_advance();
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

  /* Tell drawing thread it can continue now that buffers are swapped */
  drawnext = SDL_TRUE;
  SDL_CondSignal(drawnext_cond);
  SDL_UnlockMutex(draw_mtx);
}

static void draw_continue(void) {
  SDL_LockMutex(draw_mtx);
  drawnext = SDL_TRUE;
  SDL_CondSignal(drawnext_cond);
  SDL_UnlockMutex(draw_mtx);
}

void draw_same(void) {
  redraw_same = 1;
  draw_continue();
  draw_next();
}
#else /* !ENABLE_THREADS */
void draw_same(void) {
  draw((flags & DRAW_LOGO) ? -1 : imageFuncList[imageFuncListIndex]);
  disp_swapBuffers();
}

void draw_next(void) {
  if (draw_first) {
    draw_first = 0;
  } else {
    draw_advance();
  }
  draw_same();
}
#endif /* !ENABLE_THREADS */

void draw_init(int draw_flags) {
  flags = draw_flags;
  makeShuffledList(imageFuncList, NUM_IMAGE_FUNCTIONS);
#ifdef ENABLE_THREADS
  abort_draw = 0;
  quit_draw = 0;
  if (!(draw_mtx = SDL_CreateMutex()) ||
      !(drawdone_cond = SDL_CreateCond()) ||
      !(drawnext_cond = SDL_CreateCond()))
    fatalSDLError("creating drawing synchronization primitives");
  drawing_thread = SDL_CreateThread(drawing_main,
#if SDL_VERSION_ATLEAST(2,0,0)
                                    "DrawingThread",
#endif
                                    NULL);
  if (drawing_thread == NULL)
    fatalSDLError("creating drawing thread");
  /* TODO check SDL errors */
#else /* !ENABLE_THREADS */
  draw_first = 1;
#endif /* !ENABLE_THREADS */
}

void draw_quit(void) {
#ifdef ENABLE_THREADS
  if (drawing_thread != NULL) {
    int status;
    quit_draw = 1;
    draw_abort();
    draw_continue();
    SDL_WaitThread(drawing_thread, &status);
    drawing_thread = NULL;
  }
  if (drawdone_cond != NULL) {
    SDL_DestroyCond(drawdone_cond);
    drawdone_cond = NULL;
  }
  if (drawnext_cond != NULL) {
    SDL_DestroyCond(drawnext_cond);
    drawnext_cond = NULL;
  }
  if (draw_mtx != NULL) {
    SDL_DestroyMutex(draw_mtx);
    draw_mtx = NULL;
  }
#endif /* ENABLE_THREADS */
}
