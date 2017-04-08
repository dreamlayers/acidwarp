/* ACID WARP (c)Copyright 1992, 1993 by Noah Spurrier
 * All Rights reserved. Private Proprietary Source Code by Noah Spurrier
 * Ported to Linux by Steven Wills
 * Ported to SDL by Boris Gjenero
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <SDL.h>
#ifdef WITH_GL
#undef WITH_GLES
#undef WITH_GLEW
#ifdef __APPLE__
#include <OpenGL/gl.h>
#elif defined(_WIN32) || defined(_WIN64)
/* Functions not defined in in opengl32.dll are used, and their
   locations need to be loaded at runtime. Use GLEW for that */
#include <GL/glew.h>
#define WITH_GLEW
#else
#include <GLES2/gl2.h>
#define WITH_GLES
#endif
#if !SDL_VERSION_ATLEAST(2,0,0)
#error OpenGL only supported with SDL 2
#endif
#endif

#include "handy.h"
#include "acidwarp.h"
#include "display.h"

#if !defined(WIN32) && !SDL_VERSION_ATLEAST(2,0,0)
#define HAVE_PALETTE
#endif

#if SDL_VERSION_ATLEAST(2,0,0)
static SDL_Window *window = NULL;
#endif

#ifdef WITH_GL
SDL_GLContext context;
GLuint indtex, paltex, glprogram;

const GLchar vertex[] =
#ifdef WITH_GLES
    "#version 100\n"
    "precision mediump float;\n"
#else
    "#version 110\n"
#endif
    "attribute vec4 Position;\n"
    "attribute vec2 TexPos;\n"
    "varying vec2 TexCoord0;\n"

    "void main()\n"
    "{\n"
        "gl_Position = Position;\n"
        "TexCoord0 = TexPos;\n"
    "}\0";

const GLchar fragment[] =
#ifdef WITH_GLES
    "#version 100\n"
    "precision mediump float;\n"
#else
    "#version 110\n"
#endif
    "uniform sampler2D Palette;\n"
    "uniform sampler2D IndexTexture;\n"
    // Texture coordinates are passed from vertex shader
    "varying vec2 TexCoord0;\n"

    "void main()\n"
    "{\n"
      // Look up pixel in 8 bpp indexed colour image texture
      "vec4 myindex = texture2D(IndexTexture, TexCoord0);\n"
      // Read RGBA value for that pixel from palette texture
      "gl_FragColor = texture2D(Palette, vec2(myindex.r, 0.0));\n"
    "}\0";
#else /* !WITH_GL */
static SDL_Surface *screen = NULL, *out_surf = NULL;
#ifdef ENABLE_THREADS
static UCHAR *buf_out = NULL;
static SDL_Surface *draw_surf = NULL;
#else /* !ENABLE_THREADS */
#define draw_surf out_surf
#endif /* !ENABLE_THREADS */
static int disp_DrawingOnSurface;
#endif /* !WITH_GL */

#ifdef HAVE_PALETTE
static int disp_UsePalette;
#endif

#ifdef HAVE_FULLSCREEN
static int fullscreen = 0;
#if SDL_VERSION_ATLEAST(2,0,0)
static int desktopfs = 0;
#else
static int nativewidth = 0, nativeheight;
#endif
static int winwidth = 0;
static int winheight;
#endif /* HAVE_FULLSCREEN */

static int scaling = 1;
static int width, height;

void disp_setPalette(unsigned char *palette)
{
#ifdef WITH_GL
  static GLubyte glcolors[256 * 4];
  int i;

  for (i = 0; i < 256; i++) {
      glcolors[i*4+0] = palette[i*3+0] << 2;
      glcolors[i*4+1] = palette[i*3+1] << 2;
      glcolors[i*4+2] = palette[i*3+2] << 2;
      glcolors[i*4+3] = 255;
  }

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, paltex);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 1, GL_RGBA,
                  GL_UNSIGNED_BYTE, glcolors);

  glClear(GL_COLOR_BUFFER_BIT);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  SDL_GL_SwapWindow(window);
#else /* !WITH_GL  */
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
#endif /* HAVE_PALETTE */
  {
    /* Update colours in software surface, blit it to the screen
     * with updated colours, and then show it on the screen.
     */
#if SDL_VERSION_ATLEAST(2,0,0)
    /* Is this really necessary,
     * or could code above write directly into sdlPalette->Colors?
     */
    SDL_SetPaletteColors(out_surf->format->palette, sdlColors, 0, 256);
#else /* !SDL_VERSION_ATLEAST(2,0,0) */
    SDL_SetColors(out_surf, sdlColors, 0, 256);
#endif
    if (out_surf != screen) {
      SDL_BlitSurface(out_surf, NULL, screen, NULL);
    }
#if SDL_VERSION_ATLEAST(2,0,0)
    SDL_UpdateWindowSurface(window);
#else /* !SDL_VERSION_ATLEAST(2,0,0) */
    SDL_Flip(screen);
#endif
  }
#endif /* !WITH_GL */
}

void disp_beginUpdate(void)
{
#ifndef WITH_GL
  /* Locking only needed at this point if drawing routines directly draw
   * on a surface, and that surface needs locking.
   */
  if (disp_DrawingOnSurface) {
    if (SDL_MUSTLOCK(draw_surf)) {
      if (SDL_LockSurface(draw_surf) != 0) {
        fatalSDLError("locking surface when starting update");
      }
    }
    buf_graf = draw_surf->pixels;
    buf_graf_stride = draw_surf->pitch;
  }
#endif
}

void disp_finishUpdate(void)
{
#ifndef WITH_GL
  /* Locking only needed at this point if drawing routines directly draw
   * on a surface, and that surface needs locking.
   */
  if (disp_DrawingOnSurface) {
    if (SDL_MUSTLOCK(draw_surf)) SDL_UnlockSurface(draw_surf);
    buf_graf = NULL;
  }
#endif
}

#ifndef WITH_GL
static void disp_toSurface(void)
{
  int row;
  unsigned char *outp, *inp =
#ifdef ENABLE_THREADS
    buf_out;
#else /* !ENABLE_THREADS */
    buf_graf;
#endif
  /* This means drawing was on a separate buffer and it needs to be
   * copied to the surface. It also means the surface hasn't been locked.
   */
  if (SDL_MUSTLOCK(out_surf)) {
    if (SDL_LockSurface(out_surf) != 0) {
      fatalSDLError("locking surface when ending update");
    }
  }
  outp = out_surf->pixels;

  if (scaling == 1) {
    for (row = 0; row < height; row++) {
      memcpy(outp, inp, width);
      outp += out_surf->pitch;
      inp += width;
    }
  } else if (scaling == 2) {
    unsigned char *outp2 = outp + out_surf->pitch;
    int skip = (out_surf->pitch - width) << 1;
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
  if (SDL_MUSTLOCK(out_surf)) {
    SDL_UnlockSurface(out_surf);
  }
}
#endif /* !WITH_GL */

void disp_swapBuffers(void)
{
#ifdef WITH_GL
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, indtex);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_LUMINANCE,
                  GL_UNSIGNED_BYTE, buf_graf);
#else /* !WITH_GL */
  if (!disp_DrawingOnSurface) {
#ifdef ENABLE_THREADS
    {
      UCHAR *temp = buf_graf;
      buf_graf = buf_out;
      buf_out = temp;
    }
#endif /* ENABLE_THREADS */
    disp_toSurface();
  }
#ifdef ENABLE_THREADS
  else {
    SDL_Surface *temp = draw_surf;
    draw_surf = out_surf;
    out_surf = temp;
  }
#endif /* ENABLE_THREADS */

  if (out_surf != screen) {
    SDL_BlitSurface(out_surf, NULL, screen, NULL);
  }
#if SDL_VERSION_ATLEAST(2,0,0)
  SDL_UpdateWindowSurface(window);
#else /* !SDL_VERSION_ATLEAST(2,0,0) */
  SDL_Flip(screen);
#endif
#endif /* !WITH_GL */
}

#ifdef HAVE_FULLSCREEN
static void disp_toggleFullscreen(void)
{
#if SDL_VERSION_ATLEAST(2,0,0)
  if (fullscreen) {
    SDL_SetWindowFullscreen(window, 0);
    fullscreen = FALSE;
  } else {
    winwidth = width;
    winheight = height;
    SDL_SetWindowFullscreen(window, desktopfs ?
                                    SDL_WINDOW_FULLSCREEN_DESKTOP :
                                    SDL_WINDOW_FULLSCREEN);
    fullscreen = TRUE;
  }
#else
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
#endif
  SDL_ShowCursor(!fullscreen);
}
#endif

static void disp_processKey(
#if SDL_VERSION_ATLEAST(2,0,0)
                            SDL_Keycode key
#define keymod SDL_GetModState()
#else
                            SDLKey key, SDLMod keymod
#endif
)
{
  switch (key) {
    case SDLK_UP: handleinput(CMD_PAL_FASTER); break;
    case SDLK_DOWN: handleinput(CMD_PAL_SLOWER); break;
    case SDLK_p: handleinput(CMD_PAUSE); break;
    case SDLK_n: handleinput(CMD_SKIP); break;
#ifndef EMSCRIPTEN
    case SDLK_c:
    case SDLK_PAUSE:
      if ((keymod & KMOD_CTRL) == 0) break; /* else like SDLK_q */
    case SDLK_q: handleinput(CMD_QUIT); break;
#endif
    case SDLK_k: handleinput(CMD_NEWPAL); break;
    case SDLK_l: handleinput(CMD_LOCK); break;
#ifdef HAVE_FULLSCREEN
    case SDLK_ESCAPE:
      if (fullscreen) disp_toggleFullscreen();
      break;
    case SDLK_RETURN:
      if (keymod & KMOD_ALT) disp_toggleFullscreen();
      break;
#endif /* HAVE_FULLSCREEN */
    default: break;
  }
#undef keymod
}

static void display_redraw(void)
{
#ifdef WITH_GL
  glClear(GL_COLOR_BUFFER_BIT);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  SDL_GL_SwapWindow(window);
#elif 0
  /* Redraw parts that were overwritten. (This is unlikely with
   * modern compositing window managers */
  if (surface != screen) {
    SDL_BlitSurface(surface, NULL, screen, NULL);
#if SDL_VERSION_ATLEAST(2,0,0)
    SDL_UpdateWindowSurface(window);
#else
    SDL_Flip(screen);
#endif
  } else {
    /* Copy from buf_graf to screen */
    disp_beginUpdate();
    disp_finishUpdate();
  }
#endif /* !WITH_GL */
}

void disp_processInput(void) {
  SDL_Event event;

  while ( SDL_PollEvent(&event) > 0 ) {
    switch (event.type) {
#if !SDL_VERSION_ATLEAST(2,0,0)
      case SDL_VIDEOEXPOSE:
        display_redraw();
        break;
#endif /* !SDL_VERSION_ATLEAST(2,0,0) */
#ifdef HAVE_FULLSCREEN
      /* SDL full screen switching has no useful effect with Emscripten */
      case SDL_MOUSEBUTTONDOWN:
        if (event.button.button == SDL_BUTTON_LEFT) {
#if SDL_VERSION_ATLEAST(2,0,2)
          if (event.button.clicks == 2) {
            disp_toggleFullscreen();
          }
#else
          // Earlier SDL versions don't report double clicks
          static Uint32 dblclicktm = 0;
          Uint32 clicktime = SDL_GetTicks();
          // Like !SDL_TICKS_PASSED(), which may not be available
          if ((Sint32)(dblclicktm - clicktime) > 0) {
            disp_toggleFullscreen();
          }
          dblclicktm = clicktime + 200;
#endif // !SDL_VERSION_ATLEAST(2,0,2)
        }
        break;
#endif /* HAVE_FULLSCREEN */
      case SDL_KEYDOWN:
        disp_processKey(event.key.keysym.sym
#if !SDL_VERSION_ATLEAST(2,0,0)
                        , event.key.keysym.mod
#endif
                        );
        break;
#if SDL_VERSION_ATLEAST(2,0,0)
      case SDL_WINDOWEVENT:
        switch (event.window.event) {
        case SDL_WINDOWEVENT_RESIZED:
          /* If full screen resolution is at least twice as large as
           * previous window, then use 2x scaling, else no scaling.
           */
#if defined(HAVE_FULLSCREEN) && !defined(WITH_GL)
          scaling = fullscreen ?
                    ((winwidth != 0 &&
                      event.window.data1 >= 2 * winwidth) ? 2 : 1) : 1;
#endif /* HAVE_FULLSCREEN */
          if (width != (event.window.data1 / scaling) ||
              height != (event.window.data2 / scaling)) {
            disp_init(event.window.data1 / scaling,
                      event.window.data2 / scaling,
#ifdef HAVE_FULLSCREEN
                      fullscreen
#else /* !HAVE_FULLSCREEN */
                    0
#endif
                    );
          }
          break;
        case SDL_WINDOWEVENT_EXPOSED:
          display_redraw();
          break;
        }
        break;
#else /* !SDL_VERSION_ATLEAST(2,0,0) */
      case SDL_VIDEORESIZE:
        /* Why are there events when there is no resize? */
        if (width != (event.resize.w / scaling) ||
            height != (event.resize.h / scaling)) {
          disp_init(event.resize.w / scaling, event.resize.h / scaling,
#ifdef HAVE_FULLSCREEN
                    fullscreen
#else /* !HAVE_FULLSCREEN */
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

#if defined(HAVE_FULLSCREEN) && !SDL_VERSION_ATLEAST(2,0,0)
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

#ifdef ADDICON
extern unsigned char acidwarp_rgb[];
/* For SDL 1 call before SDL_SetWindowMode().
 * For SDL 2 call after window is created.
 */
static void disp_setIcon(void)
{
  SDL_Surface *iconsurface =
    SDL_CreateRGBSurfaceFrom(acidwarp_rgb, 64, 64, 24, 64*3,
                             0x0000ff, 0x00ff00, 0xff0000, 0
    /* Big endian may need:  0xff0000, 0x00ff00, 0x0000ff, 0 */
                             );
  if (iconsurface == NULL) fatalSDLError("creating icon surface");

#if SDL_VERSION_ATLEAST(2,0,0)
  SDL_SetWindowIcon(window, iconsurface);
#else
  /* Must be called before SDL_SetVideoMode() */
  SDL_WM_SetIcon(iconsurface, NULL);
#endif
  SDL_FreeSurface(iconsurface);
}
#endif /* ADDICON */

static void disp_freeBuffer(UCHAR **buf)
{
  if (*buf != NULL) {
    free(*buf);
    *buf = NULL;
  }
}

static void disp_reallocBuffer(UCHAR **buf)
{
  disp_freeBuffer(buf);
  *buf = calloc(width * height, 1);
  if (*buf == NULL) {
      printf("Couldn't allocate graphics buffer.\n");
      quit(-1);
  }
}

#ifndef WITH_GL
static void disp_freeSurface(SDL_Surface **surf)
{
  if (*surf != NULL) {
    SDL_FreeSurface(*surf);
    *surf = NULL;
  }
}

static void disp_allocSurface(SDL_Surface **surf)
{
  *surf = SDL_CreateRGBSurface(SDL_SWSURFACE,
                               width*scaling, height*scaling,
                               8, 0, 0, 0, 0);
  if (!(*surf)) fatalSDLError("creating secondary surface");
}

#endif /* !WITH_GL */

static void disp_allocateOffscreen(void)
{
  /* Drawing must not be happening in the background
   * while the memory being drawn to gets reallocated!
   */
  stopdrawing();
#ifdef WITH_GL
  disp_reallocBuffer(&buf_graf);
  buf_graf_stride = width;
#else /* !WITH_GL */
  /* Free secondary surface */
  if (out_surf != screen) disp_freeSurface(&out_surf);
#ifdef ENABLE_THREADS
  disp_freeSurface(&draw_surf);
#endif /* ENABLE_THREADS */

#ifdef HAVE_PALETTE
  if (disp_UsePalette) {
    /* When using a real palette, buf_graf is used instead. */
    out_surf = screen;
  } else
#endif /* HAVE_PALETTE */
  {
    /* Create 8 bit surface to draw into. This is needed if pixel
     * formats differ or to respond to SDL_VIDEOEXPOSE events.
     */
    disp_allocSurface(&out_surf);
  }

  if (scaling == 1
      /* Normally need to have offscreen data for expose events,
       * but no need for that with Emscripten.
       */
#if defined(HAVE_PALETTE) && !defined(EMSCRIPTEN)
      && !disp_UsePalette
#endif
      ) {
    if (!disp_DrawingOnSurface) {
      disp_freeBuffer(&buf_graf);
#ifdef ENABLE_THREADS
      disp_freeBuffer(&buf_out);
#endif /* ENABLE_THREADS */
    }
    disp_DrawingOnSurface = 1;
#ifdef ENABLE_THREADS
    disp_allocSurface(&draw_surf);
#endif /* ENABLE_THREADS */
  } else {
    disp_DrawingOnSurface = 0;
    disp_reallocBuffer(&buf_graf);
#ifdef ENABLE_THREADS
    disp_reallocBuffer(&buf_out);
#endif /* ENABLE_THREADS */
    buf_graf_stride = width;
  }
#endif /* !WITH_GL */
}

#ifdef WITH_GL

static void disp_glerror(char *s)
{
  GLenum err;
  fprintf(stderr, "OpenGL error at %s. Error log follows:", s);
  while((err = glGetError()) != GL_NO_ERROR) {
    fprintf(stderr, "%x\n", err);
  }
  exit(-1);
}

static GLuint disp_newtex(void)
{
    GLuint texname;

    glGenTextures(1, &texname);

    glBindTexture(GL_TEXTURE_2D, texname);

    /* Needed for GL_NEAREST sampling on non power of 2 (NPOT) textures
     * in OpenGL ES 2.0 and WebGL.
     */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

    /* Setting GL_TEXTURE_MAG_FILTER to nearest prevents image defects */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    return texname;
}

static GLuint loadShader(GLuint program, GLenum type,
                         const GLchar *shaderSrc) {
    GLint compile_status;
    GLuint shader;
    shader = glCreateShader(type);
    if (shader == 0) disp_glerror("glCreateShader");
    glShaderSource(shader, 1, &shaderSrc, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_status);
    if (compile_status != GL_TRUE) {
      GLsizei loglen = 0;
      GLchar infolog[1024];
      printf("OpenGL error: %s shader failed to compile. Info log follows:\n",
             (type == GL_VERTEX_SHADER) ? "vertex" : "fragment");
      glGetShaderInfoLog(shader, sizeof(infolog), &loglen, infolog);
      fwrite(infolog, loglen, 1, stderr);
      quit(-1);
    }
    glAttachShader(program, shader);
    return 0;
}

static void disp_glinit(int width, int height, Uint32 videoflags)
{
  GLuint buffer;
  GLint status;

  /* Vertices consist of point x, y, z, w followed by texture x and y */
  static const GLfloat vertices[] = {
      -1.0, -1.0, 0.0, 1.0, 0.0, 1.0,
      -1.0,  1.0, 0.0, 1.0, 0.0, 0.0,
       1.0, -1.0, 0.0, 1.0, 1.0, 1.0,
       1.0,  1.0, 0.0, 1.0, 1.0, 0.0,
  };

#ifdef WITH_GLES
  /* WebGL 1.0 is based on OpenGL ES 2.0 */
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#else
  // TODO: Make it work with core profile
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
#endif
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
 
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

  window = SDL_CreateWindow("Acidwarp",
                            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                            width, height,
                            videoflags | SDL_WINDOW_OPENGL);
  if (window == NULL) fatalSDLError("creating SDL OpenGL window");
  context = SDL_GL_CreateContext(window);
  if (context == NULL) fatalSDLError("creating OpenGL profile");

#ifdef WITH_GLEW
  {
    GLenum err = glewInit();
    if (GLEW_OK != err) {
      fprintf(stderr, "Error initializing GLEW: %s\n",
              glewGetErrorString(err));
      quit(-1);
    }
    /* TODO: Check exactly what version is required */
    if (!GLEW_VERSION_2_0) {
      fprintf(stderr, "Quitting because OpenGL 2.0 not supported\n");
      quit(-1);
    }
  }
#endif

  glprogram = glCreateProgram();
  if (glprogram == 0) disp_glerror("glCreateProgram");
  loadShader(glprogram, GL_VERTEX_SHADER, vertex);
  loadShader(glprogram, GL_FRAGMENT_SHADER, fragment);
  glBindAttribLocation(glprogram, 0, "Position");
  glBindAttribLocation(glprogram, 1, "TexPos");
  glLinkProgram(glprogram);
  glGetProgramiv(glprogram, GL_LINK_STATUS, &status);
  if (status != GL_TRUE) disp_glerror("glLinkProgram");
  glUseProgram(glprogram);

  glGenBuffers(1, &buffer);
  glBindBuffer(GL_ARRAY_BUFFER, buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE,
                        (char *)&vertices[6] - (char *)&vertices[0], 0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
                        (char *)&vertices[6] - (char *)&vertices[0],
                        /* Why is this parameter declared as a pointer? */
                        (void *)((char *)&vertices[4] - (char *)&vertices[0]));

  /* https://www.opengl.org/discussion_boards/showthread.php/163092-Passing-Multiple-Textures-from-OpenGL-to-GLSL-shader */

  /* 256 x 1 texture used as palette lookup table */
  glUniform1i(glGetUniformLocation(glprogram, "Palette"), 0);
  glActiveTexture(GL_TEXTURE0);
  paltex = disp_newtex();
  glBindTexture(GL_TEXTURE_2D, paltex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 1, 0,
               GL_RGBA, GL_UNSIGNED_BYTE, NULL);

  /* 8 bpp indexed colour texture used for image */
  glUniform1i(glGetUniformLocation(glprogram, "IndexTexture"), 1);
  glActiveTexture(GL_TEXTURE1);
  indtex = disp_newtex();

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
}
#endif /* WITH_GL */

#if SDL_VERSION_ATLEAST(2,0,0)
void disp_init(int newwidth, int newheight, int flags)
{
  Uint32 videoflags;
  static int inited = 0;

  width = newwidth;
  height = newheight;
#ifdef HAVE_FULLSCREEN
  fullscreen = (flags & DISP_FULLSCREEN) ? 1 : 0;
#endif

  if (!inited) {
#ifdef HAVE_FULLSCREEN
    if (flags & DISP_DESKTOP_RES_FS) {
      /* Need to know later when entering full screen another time */
      desktopfs = TRUE;
    }
#endif

    videoflags =
#ifdef HAVE_FULLSCREEN
                 (fullscreen ?
                  (desktopfs ? SDL_WINDOW_FULLSCREEN_DESKTOP :
                   SDL_WINDOW_FULLSCREEN)
                  : SDL_WINDOW_RESIZABLE);
#else
                 SDL_WINDOW_RESIZABLE;
#endif

#ifdef HAVE_FULLSCREEN
    SDL_ShowCursor(!fullscreen);
#endif

    scaling = 1;

#ifdef WITH_GL
    disp_glinit(width, height, videoflags);
#else /* !WITH_GL */

    /* The screen is a destination for SDL_BlitSurface() copies.
     * Nothing is ever directly drawn here, except with Emscripten.
     */
    window = SDL_CreateWindow("Acidwarp",
                              SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              width*scaling, height*scaling, videoflags);
    if (window == NULL) fatalSDLError("creating SDL window");
#endif /* !WITH_GL */

#ifdef ADDICON
    /* Must be called after window is created */
    disp_setIcon();
#endif

    inited = 1;
  } /* !inited */

  /* Raspberry Pi console will set window to size of full screen,
   * and not give a resize event. */
  SDL_GetWindowSize(window, &width, &height);

#ifdef WITH_GL
  /* Create or recreate texture and set viewport, eg. when resizing */
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, indtex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0,
               GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
  glViewport(0, 0, width, height);
#else /* !WITH_GL */
  /* Needs to be called again after resizing */
  screen = SDL_GetWindowSurface(window);
  if (!screen) fatalSDLError("getting window surface");
#endif /* !WITH_GL */

  disp_allocateOffscreen();

  /* This may be unnecessary if switching between windowed
   * and full screen mode with the same dimensions. */
  handleresize(width, height);
}

#else /* !SDL_VERSION_ATLEAST(2,0,0) */

void disp_init(int newwidth, int newheight, int flags)
{
  Uint32 videoflags;
  static int inited = 0;
#ifdef HAVE_PALETTE
  static int nativedepth = 8;
  int usedepth;
#endif
#ifdef HAVE_FULLSCREEN
  static int desktopaspect = 0;
#endif

  width = newwidth;
  height = newheight;
#ifdef HAVE_FULLSCREEN
  fullscreen = (flags & DISP_FULLSCREEN) ? 1 : 0;
#endif
  videoflags = SDL_HWSURFACE | SDL_DOUBLEBUF |
#ifndef HAVE_PALETTE
               SDL_ANYFORMAT |
#endif
#ifdef HAVE_FULLSCREEN
               /* It would make sense to remove SDL_RESIZABLE for full screen,
                * but that causes window to not be resizable anymore in Linux
                * after it returns to windowed mode. */
               (fullscreen ? SDL_FULLSCREEN : 0) |
#endif
               SDL_RESIZABLE;

  if (!inited) {
#ifdef HAVE_FULLSCREEN
    const SDL_VideoInfo *vi;

    /* Save information about desktop video mode */
    vi = SDL_GetVideoInfo();
    if (vi != NULL) {
#ifdef HAVE_PALETTE
      nativedepth = vi->vfmt->BitsPerPixel;
#endif
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
#endif /* HAVE_FULLSCREEN */

    SDL_WM_SetCaption("Acidwarp","acidwarp");

#ifdef ADDICON
    /* Must be called before SDL_SetVideoMode() */
    disp_setIcon();
#endif

#ifdef HAVE_FULLSCREEN
    /* This causes an error when using Emscripten and Firefox */
    SDL_ShowCursor(!fullscreen);
#endif

    inited = 1;
  }  /* !inited */

#ifdef HAVE_PALETTE
  usedepth = nativedepth;
#endif
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
      /* This was slower with Emscripten SDL 1 before,
       * but now it should be faster.
       */
      videoflags |= SDL_HWPALETTE;
    } else {
      disp_UsePalette = 0;
      videoflags |= SDL_ANYFORMAT;
    }
#endif
  }

  /* The screen is a destination for SDL_BlitSurface() copies.
   * Nothing is ever directly drawn here, except with Emscripten.
   */
  screen = SDL_SetVideoMode(width*scaling, height*scaling,
#ifdef HAVE_PALETTE
                            usedepth,
#else
                            0,
#endif
                            videoflags);
  if (!screen) fatalSDLError("setting video mode");
  /* No need to ever free the screen surface from SDL_SetVideoMode() */

  disp_allocateOffscreen();

  /* This may be unnecessary if switching between windowed
   * and full screen mode with the same dimensions. */
  handleresize(width, height);
}
#endif /* !SDL_VERSION_ATLEAST(2,0,0) */

void disp_quit(void)
{
#ifdef WITH_GL
  disp_freeBuffer(&buf_graf);
  // FIXME: clean up OpenGL stuff
#else /* !WITH_GL */
  if (disp_DrawingOnSurface) {
    if (out_surf == screen) {
      out_surf = NULL;
    } else {
      disp_freeSurface(&out_surf);
    }
#ifdef ENABLE_THREADS
    disp_freeSurface(&draw_surf);
#endif /* ENABLE_THREADS */
  } else {
    disp_freeBuffer(&buf_graf);
#ifdef ENABLE_THREADS
    disp_freeBuffer(&buf_out);
#endif /* ENABLE_THREADS */
  }
  /* Do not free result of SDL_GetWindowSurface() or SDL_SetVideoMode() */
  screen = NULL;
#endif
}
