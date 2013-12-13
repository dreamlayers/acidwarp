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

#if !defined(WIN32)
#define HAVE_PALETTE
#endif

static SDL_Surface *surface = NULL, *screen = NULL;
static int disp_DrawingOnSurface;
#ifdef HAVE_PALETTE
static int disp_UsePalette;
#endif
static int fullscreen = 0;
static int scaling = 1;
// Save window size when in full screen
static int winwidth = 0;
static int winheight;
static int width, height;

static void disp_SDLFatal(const char *msg) {
  fprintf(stderr, "SDL error while %s: %s", msg, SDL_GetError());
  exit(-1);
}

void disp_setPalette(unsigned char *palette)
{
  static SDL_Color sdlPalette[256];
  int i;
  for(i=0;i<256;i++) {
    sdlPalette[i].r = palette[i*3+0] << 2;
    sdlPalette[i].g = palette[i*3+1] << 2;
    sdlPalette[i].b = palette[i*3+2] << 2;
  }

#ifdef HAVE_PALETTE
  if (disp_UsePalette) {
    /* Simply change the palette */
    SDL_SetPalette(screen, SDL_PHYSPAL, sdlPalette, 0, 256);
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
    SDL_SetColors(surface, sdlPalette, 0, 256);
    if (surface != screen) {
      SDL_BlitSurface(surface, NULL, screen, NULL);
    }
    SDL_Flip(screen);
  }
}

void disp_beginUpdate(void)
{
  /* Locking only needed at this point if drawing routines directly draw
   * on a surface, and that surface needs locking.
   */
  if (disp_DrawingOnSurface && SDL_MUSTLOCK(surface)) {
    if (SDL_LockSurface(surface) != 0) {
      disp_SDLFatal("locking surface when starting update");
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
        disp_SDLFatal("locking surface when ending update");
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
  SDL_Flip(screen);
}

static void disp_processKey(SDLKey key)
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
#ifndef EMSCRIPTEN
      /* SDL full screen switching has no useful effect with Emscripten */
      case SDL_MOUSEBUTTONDOWN:
		fullscreen = !fullscreen;
		disp_init(width, height);
		break;
#endif
      case SDL_KEYDOWN:
        disp_processKey(event.key.keysym.sym);
        break;
      case SDL_VIDEORESIZE:
        /* Why are there events when there is no resize? */
        if (width != (event.resize.w / scaling) ||
            height != (event.resize.h / scaling)) {
		disp_init(event.resize.w / scaling, event.resize.h / scaling);
		handleresize(width, height);
        }
		break;
      case SDL_QUIT:
		handleinput(CMD_QUIT);
		break;

      default:
        break;
    }
  }
}

void disp_init(int newwidth, int newheight)
{
  Uint32 videoflags = SDL_HWSURFACE | SDL_DOUBLEBUF |
                      (fullscreen ? SDL_FULLSCREEN : SDL_RESIZABLE);
  static int inited = 0;
  static int nativedepth = 8;
  static int desktopaspect = 0;
  int usedepth;
  int resize = 0;
  
  width = newwidth;
  height = newheight;

  if (!inited) {
#ifndef EMSCRIPTEN
    const SDL_VideoInfo *vi;
#endif

    /* Initialize SDL */
    if ( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
      disp_SDLFatal("initializing video subsystem");
    }

#ifndef EMSCRIPTEN
    vi = SDL_GetVideoInfo();
    if (vi != NULL) {
      nativedepth = vi->vfmt->BitsPerPixel;
      if (vi->current_w > 0 && vi->current_h > 0) {
        desktopaspect = vi->current_w * 1024 / vi->current_h;
      }
    }
#endif

    SDL_WM_SetCaption("Acidwarp","acidwarp");
  }

  /* If resizing, there may be old stuff which needs to be freed */
  /* If there was a separate graphics buffer, free it. */
  if (!disp_DrawingOnSurface && buf_graf != NULL) {
    free(buf_graf);
  }
  /* Free secondary surface */
  if (surface != NULL && surface != screen) {
    SDL_FreeSurface(surface);
  }
  /* No need to ever free the screen surface from SDL_SetVideoMode() */
  
#ifndef EMSCRIPTEN
  /* This causes an error in Firefox */
  SDL_ShowCursor(!fullscreen);
#endif

#ifdef HAVE_PALETTE
  if (fullscreen || nativedepth == 8) {
    disp_UsePalette = 1;
    usedepth = 8;
#ifndef EMSCRIPTEN
    /* This seems to have no beneficial effect with Emscrpten SDL.
     * The displayed image never chages in response to SDL_SetPalette().
     * Setting the flag just increases CPU usage.
     */
    videoflags |= SDL_HWPALETTE;
#endif
  } else {
    disp_UsePalette = 0;
    usedepth = nativedepth;
    videoflags |= SDL_ANYFORMAT;
  }
#else
  usedepth = nativedepth;
  videoflags |= SDL_ANYFORMAT;
#endif

  // If going back to windowed mode, restore window size
  if (!fullscreen) {
    scaling = 1;
    if (winwidth != 0) {
	  width = winwidth;
	  height = winheight;
	  resize = 1;
      winwidth = 0;	  
    }
  } else {
    SDL_Rect **modes;

    /* Get available fullscreen modes */
#ifdef HAVE_PALETTE
    if (disp_UsePalette) {
      /* Attempt to find a 256 color mode */
      struct SDL_PixelFormat wantpf;
      memset(&wantpf, 0, sizeof(wantpf));
      wantpf.BitsPerPixel = 8;
      wantpf.BytesPerPixel = 1;
      modes = SDL_ListModes(&wantpf, videoflags);
      if (modes == NULL) {
        /* Couldn't find a 256 colour mode. Try to find any mode. */
        disp_UsePalette = 0;
        videoflags &= ~SDL_HWPALETTE;
        videoflags |= SDL_ANYFORMAT;
        modes = SDL_ListModes(NULL, videoflags);
      }
    }
#else
    modes = SDL_ListModes(NULL, videoflags);
#endif
    if (modes == NULL) {
      disp_SDLFatal("listing full screen modes");
    } else if (modes == (SDL_Rect **)-1) {
      /* All resolutions ok */
      scaling = 1;
    } else {
      // Full screen should really fill the whole screen
      // Find video mode with closest number of pixels
      int newwidth = 0;
	  int newheight = 0;
	  int bestdiff = -1;
	  int curpix = width * height;
	  int i, j;
      for(i=0;modes[i];i++) {
	    /* For every mode, try every possible scaling */
	    for (j=1; j<=2; j++) {
          int asperr, pixerr, curdiff;

          /* Difference in number of pixels */
	      pixerr = modes[i]->w * modes[i]->h / (j * j) - curpix;
          if (pixerr < 0) pixerr = -pixerr;

          /* Difference in aspect ratio compared to desktop */
          if (desktopaspect > 0) {
            int aspect = modes[i]->w * 1024 / modes[i]->h;
            asperr = aspect - desktopaspect;
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
	          (curdiff == bestdiff && j < scaling)) {
            scaling = j;
   	        newwidth = modes[i]->w / j;
	        newheight = modes[i]->h / j;
	        bestdiff = curdiff;
		  }	  
	    }
	  }
	  
	  if (newwidth != 0 && (newwidth != width || newheight != height)) {
	    winwidth = width;
	    winheight = height;
		width = newwidth;
		height = newheight;
		resize = 1;
      }
	}
  }

  /* The screen is a destination for SDL_BlitSurface() copies.
   * Nothing is ever directly drawn here.
   */
  screen = SDL_SetVideoMode(width*scaling, height*scaling,
                            usedepth, videoflags);
  if (!screen) disp_SDLFatal("setting video mode");

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
  }

  if (!surface) disp_SDLFatal("creating secondary surface");

  if (scaling == 1
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
    /* Clearing is only needed for the initial logo */
    if (!inited) memset(buf_graf, 0, width * height);
  }

  if (inited && resize) {
    handleresize(width, height);
  }

  inited = 1;
}
