/* ACID WARP (c)Copyright 1992, 1993 by Noah Spurrier
 * All Rights reserved. Private Proprietary Source Code by Noah Spurrier
 * Ported to Linux by Steven Wills
 * Ported to SDL by Boris Gjenero
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <SDL.h>

#include "handy.h"
#include "acidwarp.h"
#include "display.h"

#if !defined(WIN32) && !SDL_VERSION_ATLEAST(2,0,0)
#define HAVE_PALETTE
#endif

#if SDL_VERSION_ATLEAST(2,0,0)
static SDL_Window *window = NULL;
static SDL_Palette *sdlPalette = NULL;
#endif
static SDL_Surface *surface = NULL, *screen = NULL;
static int disp_DrawingOnSurface;
#ifdef HAVE_PALETTE
static int disp_UsePalette;
#endif
#ifdef HAVE_FULLSCREEN
static int fullscreen = 0;
static int nativewidth = 0, nativeheight;
static int winwidth = 0;
static int winheight;
#endif
static int scaling = 1;
static int width, height;

void disp_setPalette(unsigned char *palette)
{
  static SDL_Color sdlColors[256];

  int i;
  for(i=0;i<256;i++) {
    sdlColors[i].r = palette[i*3+0] << 2;
    sdlColors[i].g = palette[i*3+1] << 2;
    sdlColors[i].b = palette[i*3+2] << 2;
  }

#ifdef HAVE_PALETTE
  if (disp_UsePalette) {
    /* Simply change the palette */
    SDL_SetPalette(screen, SDL_PHYSPAL, sdlColors, 0, 256);
#ifdef EMSCRIPTEN
    /* This is needed for palette change to take effect. */
    SDL_LockSurface(screen);
    SDL_UnlockSurface(screen);
#endif
  } else
#endif
  {
    /* Update colours in software surface, blit it to the screen
     * with updated colours, and then show it on the screen.
     */
#if SDL_VERSION_ATLEAST(2,0,0)
    /* Is this really necessary,
     * or could code above write directly into sdlPalette->Colors?
     */
    SDL_SetPaletteColors(sdlPalette, sdlColors, 0, 256);
#else
    SDL_SetColors(surface, sdlColors, 0, 256);
#endif
    if (surface != screen) {
      SDL_BlitSurface(surface, NULL, screen, NULL);
    }
#if SDL_VERSION_ATLEAST(2,0,0)
    SDL_UpdateWindowSurface(window);
#else
    SDL_Flip(screen);
#endif
  }
}

void disp_beginUpdate(void)
{
  /* Locking only needed at this point if drawing routines directly draw
   * on a surface, and that surface needs locking.
   */
  if (disp_DrawingOnSurface && SDL_MUSTLOCK(surface)) {
    if (SDL_LockSurface(surface) != 0) {
      fatalSDLError("locking surface when starting update");
    }
    buf_graf = surface->pixels;
    buf_graf_stride = surface->pitch;
  }
}

void disp_finishUpdate(void)
{
  if (!disp_DrawingOnSurface) {
    int row;
    unsigned char *outp, *inp = buf_graf;

    /* This means drawing was on a separate buffer and it needs to be
     * copied to the surface. It also means the surface hasn't been locked.
     */
    if (SDL_MUSTLOCK(surface)) {
      if (SDL_LockSurface(surface) != 0) {
        fatalSDLError("locking surface when ending update");
        exit(-1);
      }
    }
    outp = surface->pixels;

  if (scaling == 1) {
    for (row = 0; row < height; row++) {
      memcpy(outp, inp, width);
      outp += surface->pitch;
      inp += width;
    }
  } else if (scaling == 2) {
    unsigned char *outp2 = outp + surface->pitch;
    int skip = (surface->pitch - width) << 1;
    int col;
    unsigned char c;
    for (row = 0; row < height; row++) {
      for (col = 0; col < width; col++) {
        c = *(inp++);
        *(outp++) = c;
        *(outp++) = c;
        *(outp2++) = c;
        *(outp2++) = c;
      }
      outp += skip;
      outp2 += skip;
    }
  }
  }

  SDL_UnlockSurface(surface);
  if (surface != screen) {
    SDL_BlitSurface(surface, NULL, screen, NULL);
  }
#if SDL_VERSION_ATLEAST(2,0,0)
  SDL_UpdateWindowSurface(window);
#else
  SDL_Flip(screen);
#endif
}

#ifdef HAVE_FULLSCREEN
static void disp_toggleFullscreen(void)
{
  if (fullscreen) {
    /* If going back to windowed mode, restore window size */
    if (winwidth != 0) {
      disp_init(winwidth, winheight, 0);
      winwidth = 0;
    } else {
      disp_init(width, height, 0);
    }
  } else {
    /* Save window size for return to windowed mode */
    winwidth = width;
    winheight = height;
    /* disp_init() may select a different size than suggested. It will
     * handle resizing if needed.
     */
    disp_init(width, height, DISP_FULLSCREEN);
  }
}
#endif

static void disp_processKey(
#if SDL_VERSION_ATLEAST(2,0,0)
                            SDL_Keycode
#else
                            SDLKey
#endif
                            key
)
{
  switch (key) {
    case SDLK_UP: handleinput(CMD_PAL_FASTER); break;
    case SDLK_DOWN: handleinput(CMD_PAL_SLOWER); break;
    case SDLK_p: handleinput(CMD_PAUSE); break;
    case SDLK_n: handleinput(CMD_SKIP); break;
    case SDLK_q: handleinput(CMD_QUIT); break;
    case SDLK_k: handleinput(CMD_NEWPAL); break;
    case SDLK_l: handleinput(CMD_LOCK); break;
    default: break;
  }
}

void disp_processInput(void) {
  SDL_Event event;

  while ( SDL_PollEvent(&event) > 0 ) {
    switch (event.type) {
#if !SDL_VERSION_ATLEAST(2,0,0)
      case SDL_VIDEOEXPOSE:
        /* Redraw parts that were overwritten. (This is unlikely with
         * modern compositing window managers */
        if (surface != screen) {
          SDL_BlitSurface(surface, NULL, screen, NULL);
          SDL_Flip(screen);
        } else {
          /* Copy from buf_graf to screen */
          disp_beginUpdate();
          disp_finishUpdate();
        }
        break;
#endif
#ifdef HAVE_FULLSCREEN
      /* SDL full screen switching has no useful effect with Emscripten */
      case SDL_MOUSEBUTTONDOWN:
        if (event.button.button == SDL_BUTTON_LEFT) {
          disp_toggleFullscreen();
        }
        break;
#endif
      case SDL_KEYDOWN:
        disp_processKey(event.key.keysym.sym);
        break;
#if !SDL_VERSION_ATLEAST(2,0,0)
      case SDL_VIDEORESIZE:
        /* Why are there events when there is no resize? */
        if (width != (event.resize.w / scaling) ||
            height != (event.resize.h / scaling)) {
          disp_init(event.resize.w / scaling, event.resize.h / scaling,
#ifdef HAVE_FULLSCREEN
                    fullscreen
#else
                    0
#endif
                    );
        }
        break;
#endif
      case SDL_QUIT:
        handleinput(CMD_QUIT);
        break;

      default:
        break;
    }
  }
}

#ifdef HAVE_FULLSCREEN
/* Function for finding the best SDL full screen mode for filling the screen.
 *
 * Inputs:
 * modes: array of pointers to SDL_Rect structures describing modes.
 * width, height: dimensions of desired mode
 * desiredaspect: desired aspect ratio
 *
 * Outputs:
 * width, height: updated with dimensions of found mode
 * scaling: updated with scaling to be used along with that mode
 */
static void disp_findBestMode(SDL_Rect ** modes,
                              int *width, int *height,
                              int *scaling, int desiredaspect)
{
  int bestdiff = -1;
  int curpix = *width * *height;
  int i, j;
  for(i=0;modes[i];i++) {
    /* For every mode, try every possible scaling */
    for (j=1; j<=2; j++) {
      int asperr, pixerr, curdiff;

      /* Difference in number of pixels */
      pixerr = modes[i]->w * modes[i]->h / (j * j) - curpix;
      if (pixerr < 0) pixerr = -pixerr;

      /* Difference in aspect ratio compared to desktop */
      if (desiredaspect > 0) {
        int aspect = modes[i]->w * 1024 / modes[i]->h;
        asperr = aspect - desiredaspect;
        if (asperr < 0) asperr = -asperr;
        /* Aspect ratio is important because we want to fill screen */
        asperr *= 1024;
      } else {
        asperr = 0;
      }

      /* Use sum of pixel and aspect ratio error */
      curdiff = pixerr + asperr;

      /* Check if this mode is better */
      if (bestdiff == -1 || curdiff < bestdiff ||
          (curdiff == bestdiff && j < *scaling)) {
        *scaling = j;
        *width = modes[i]->w / j;
        *height = modes[i]->h / j;
        bestdiff = curdiff;
      }
    }
  }
}
#endif /* HAVE_FULLSCREEN */

static void disp_allocateOffscreen(void)
{
  /* If there was a separate graphics buffer, free it. */
  if (!disp_DrawingOnSurface && buf_graf != NULL) {
    free(buf_graf);
  }
  /* Free secondary surface */
  if (surface != NULL && surface != screen) {
    SDL_FreeSurface(surface);
  }

#ifdef HAVE_PALETTE
  if (disp_UsePalette) {
    /* When using a real palette, buf_graf is used instead. */
    surface = screen;
  } else
#endif
  {
    /* Create 8 bit surface to draw into. This is needed if pixel
     * formats differ or to respond to SDL_VIDEOEXPOSE events.
     */
    surface = SDL_CreateRGBSurface(SDL_SWSURFACE,
                                   width*scaling, height*scaling,
                                   8, 0, 0, 0, 0);

    if (!surface) fatalSDLError("creating secondary surface");

    SDL_SetSurfacePalette(surface, sdlPalette);
  }

  if (scaling == 1
      /* Normally need to have offscreen data for expose events,
       * but no need for that with Emscripten.
       */
#if defined(HAVE_PALETTE) && !defined(EMSCRIPTEN)
      && !disp_UsePalette
#endif
      ) {
    disp_DrawingOnSurface = 1;
    if (!SDL_MUSTLOCK(surface)) {
      buf_graf = surface->pixels;
      buf_graf_stride = surface->pitch;
    }
  } else {
    disp_DrawingOnSurface = 0;
    buf_graf = malloc (width * height);
    buf_graf_stride = width;
    memset(buf_graf, 0, width * height);
  }
}

void disp_init(int newwidth, int newheight, int flags)
{
  Uint32 videoflags;
  static int inited = 0;
  static int nativedepth = 8;
#ifdef HAVE_FULLSCREEN
  static int desktopaspect = 0;
#endif
  int usedepth;

  width = newwidth;
  height = newheight;
#ifdef HAVE_FULLSCREEN
  fullscreen = (flags & DISP_FULLSCREEN) ? 1 : 0;
#endif

#if SDL_VERSION_ATLEAST(2,0,0)
  videoflags =
#ifdef HAVE_FULLSCREEN
               (fullscreen ? SDL_WINDOW_FULLSCREEN : SDL_WINDOW_RESIZABLE);
#else
               SDL_WINDOW_RESIZABLE;
#endif
#else /* !SDL_VERSION_ATLEAST(2,0,0) */
  videoflags = SDL_HWSURFACE | SDL_DOUBLEBUF |
#ifndef HAVE_PALETTE
               SDL_ANYFORMAT |
#endif
#ifdef HAVE_FULLSCREEN
               (fullscreen ? SDL_FULLSCREEN : SDL_RESIZABLE);
#else
               SDL_RESIZABLE;
#endif
#endif

  if (!inited) {
#ifdef HAVE_FULLSCREEN
    const SDL_VideoInfo *vi;

    vi = SDL_GetVideoInfo();
    if (vi != NULL) {
      nativedepth = vi->vfmt->BitsPerPixel;
      if (vi->current_w > 0 && vi->current_h > 0) {
        if (flags & DISP_DESKTOP_RES_FS) {
          nativewidth = vi->current_w;
          nativeheight = vi->current_h;
          if (flags & DISP_FULLSCREEN) {
            /* Save size, which is for windowed mode */
            winwidth = newwidth;
            winheight = newheight;
          }
        } else {
          desktopaspect = vi->current_w * 1024 / vi->current_h;
        }
      }
    }
#endif
#if SDL_VERSION_ATLEAST(2,0,0)
    /* The palette must be allocated this way.
       Otherwise, SDL_FreeSurface() could try to free() it. */
    sdlPalette = SDL_AllocPalette(256);
    if (sdlPalette == NULL) {
      fatalSDLError("allocating palette");
    }
#else /* !SDL_VERSION_ATLEAST(2,0,0) */
    SDL_WM_SetCaption("Acidwarp","acidwarp");
#endif
  }

#ifdef HAVE_FULLSCREEN
  /* This causes an error when using Emscripten and Firefox */
  SDL_ShowCursor(!fullscreen);
#endif

  usedepth = nativedepth;
#ifdef HAVE_FULLSCREEN
  if (fullscreen && nativewidth == 0) {
    SDL_Rect **modes;

#ifdef HAVE_PALETTE
    /* Attempt to list 256 colour modes */
    struct SDL_PixelFormat wantpf;
    memset(&wantpf, 0, sizeof(wantpf));
    wantpf.BitsPerPixel = 8;
    wantpf.BytesPerPixel = 1;
    modes = SDL_ListModes(&wantpf, videoflags | SDL_HWPALETTE);
    if (modes != NULL) {
      /* Found 256 colour mode. Use it. */
      disp_UsePalette = 1;
      usedepth = 8;
      videoflags |= SDL_HWPALETTE;
    } else {
      /* Couldn't find a 256 colour mode. Try to find any mode. */
      disp_UsePalette = 0;
      videoflags |= SDL_ANYFORMAT;
      modes = SDL_ListModes(NULL, videoflags);
    }
#else /* !HAVE_PALETTE */
    /* Get available fullscreen modes */
    modes = SDL_ListModes(NULL, videoflags);
#endif /* !HAVE_PALETTE */
    if (modes == NULL) {
      fatalSDLError("listing full screen modes");
    } else if (modes == (SDL_Rect **)-1) {
      /* All resolutions ok */
      scaling = 1;
    } else {
      disp_findBestMode(modes, &width, &height, &scaling, desktopaspect);
    }
  } else
#endif /* HAVE_FULLSCREEN */
  {
#ifdef HAVE_FULLSCREEN
    if (fullscreen) {
      /* This happens when using desktop
       * resolution for full screen.
       */
      width = nativewidth;
      height = nativeheight;
#ifdef HAVE_PALETTE
      usedepth = 8;
#endif
    }
#endif

    scaling = 1;
#ifdef HAVE_PALETTE
    if (usedepth == 8) {
      disp_UsePalette = 1;
#ifndef EMSCRIPTEN
      /* This seems to have no beneficial effect with Emscrpten SDL.
       * The displayed image never chages in response to SDL_SetPalette().
       * Setting the flag just increases CPU usage.
       */
      videoflags |= SDL_HWPALETTE;
#endif
    } else {
      disp_UsePalette = 0;
      videoflags |= SDL_ANYFORMAT;
    }
#endif
  }

  /* The screen is a destination for SDL_BlitSurface() copies.
   * Nothing is ever directly drawn here, except with Emscripten.
   */
#if SDL_VERSION_ATLEAST(2,0,0)
  window = SDL_CreateWindow("Acidwarp",
                            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                            width*scaling, height*scaling, videoflags);
  screen = SDL_GetWindowSurface(window);
#else
  screen = SDL_SetVideoMode(width*scaling, height*scaling,
                            usedepth, videoflags);
#endif
  if (!screen) fatalSDLError("setting video mode");
  /* No need to ever free the screen surface from SDL_SetVideoMode() */

  disp_allocateOffscreen();

  /* This may be unnecessary if switching between windowed
   * and full screen mode with the same dimensions. */
  handleresize(width, height);

  inited = 1;
}
