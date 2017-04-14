#include <string.h>
#include <emscripten.h>

#include "handy.h"
#include "bit_map.h"
#include "acidwarp.h"
#include "worker.h"

/* This drawing code runs as a web worker.
 * Requests specify the size, wanted image, and next image.
 * If requested image is ready from the last call, it is sent as a response.
 * Else, image is generated and then sent as a response.
 * In either case, after the response, the next image is generated.
 */
EMSCRIPTEN_KEEPALIVE
static void draw(char *data, int size)
{
  static worker_param param = { 0 };
  worker_param newparam;
  static UCHAR *buf_graf = NULL;
  static unsigned int res_size = 0;
  int which, redraw = 0;

  if (size < sizeof(worker_param)) return;

  memcpy(&newparam, data, sizeof(worker_param));

  if (buf_graf != NULL) {
    if (newparam.width == param.width && newparam.height == param.height) {
      if (newparam.want == param.next) {
        /* We have the desired image. Send it to the main thread */
        emscripten_worker_respond_provisionally((char *)buf_graf, res_size);
      } else {
        /* We need to draw a different image, but same size */
        redraw = 1;
      }
    } else {
      /* Image dimensions changed, reallocate? */
      unsigned int new_size = newparam.width * newparam.height;
      if (res_size != new_size) {
        free(buf_graf);
        res_size = new_size;
        buf_graf = calloc(res_size, 1);
      }
      redraw = 1;
    }
  } else {
    /* First call, allocate response buffer */
    res_size = newparam.width * newparam.height;
    buf_graf = calloc(res_size, 1);
    redraw = 1;
  }

  memcpy(&param, &newparam, sizeof(worker_param));

  /* Looping in case desired image needs to be regenerated */
  while (1) {
    which = redraw ? param.want : param.next;
    if (which < 0) {
      writeBitmapImageToArray(buf_graf, NOAHS_FACE, param.width, param.height,
                              param.width);
    } else {
      if (param.flags & DRAW_FLOAT) {
        generate_image_float(which,
                             buf_graf, param.width/2, param.height/2,
                             param.width, param.height,
                             256, param.width, param.flags & DRAW_SCALED);
      } else {
        generate_image(which,
                       buf_graf, param.width/2, param.height/2,
                       param.width, param.height, 256, param.width);
      }
    }

    if (redraw) {
      /* Desired image was generated. Respond and then generate next. */
      emscripten_worker_respond_provisionally((char *)buf_graf, res_size);
      redraw = 0;
    } else {
      /* Next image was generated. Desired image was sent up above.
       * Nothing left to do, as next image will be sent on next call.
       */
      break;
    }
  }
}
