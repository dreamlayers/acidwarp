#include <string.h>
#include <emscripten.h>

#include "handy.h"
#include "bit_map.h"
#include "acidwarp.h"
#include "worker.h"
#include "display.h"

static int imageFuncList[NUM_IMAGE_FUNCTIONS];
static int imageFuncListIndex=0;

static worker_param wpar;
static worker_handle worker;
static UCHAR *buf_graf;
unsigned int buf_graf_stride;

void draw_init(int draw_flags) {
  wpar.flags = draw_flags;
  makeShuffledList(imageFuncList, NUM_IMAGE_FUNCTIONS);
  wpar.next = (draw_flags & DRAW_LOGO) ? -1 : imageFuncList[imageFuncListIndex];
  worker = emscripten_create_worker("worker.js");
  /* Could start a worker call here but simpler not to. */
}

void draw_quit(void) {
  // FIXME
}

static void worker_callback(char *data, int size, void *arg)
{
  if (buf_graf_stride == wpar.width) {
    memcpy(buf_graf, data, wpar.width * wpar.height);
  } else {
    int i;
    char *inp = data;
    UCHAR *outp = buf_graf;
    for (i = 0; i < wpar.height; i++) {
      memcpy(outp, inp, wpar.width);
      inp += wpar.width;
      outp += buf_graf_stride;
    }
  }
  disp_finishUpdate();
  disp_swapBuffers();
  startloop();
}

void draw_same(void)
{
  disp_beginUpdate(&buf_graf, &buf_graf_stride, &wpar.width, &wpar.height);
  emscripten_call_worker(worker, "draw", (char *)&wpar, sizeof(wpar),
                         worker_callback, NULL);
  /* Worker callback will restart main loop */
  emscripten_cancel_main_loop();
}

static void draw_advance(void)
{
  if (wpar.flags & DRAW_LOGO) {
    wpar.flags &= ~DRAW_LOGO;
  } else {
    if (++imageFuncListIndex >= NUM_IMAGE_FUNCTIONS) {
      imageFuncListIndex = 0;
      makeShuffledList(imageFuncList, NUM_IMAGE_FUNCTIONS);
    }
  }
}

void draw_next()
{
  wpar.want = wpar.next;
  draw_advance();
  wpar.next = imageFuncList[imageFuncListIndex];
  draw_same();
}
