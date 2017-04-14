/* ACIDWARP.H */

#define VERSION "Acid Warp Version 4.10 (C)Copyright 1992, 1993 by Noah Spurrier and Mark Bilk\nSDL port by Boris Gjenero based on Linux port by Steven Wills"

#define DIRECTN_CHANGE_PERIOD_IN_TICKS               256

#ifdef EMSCRIPTEN
#define ENABLE_WORKER
#else /* !EMSCRIPTEN */
#define HAVE_FULLSCREEN
#define ENABLE_THREADS
#endif

/* Palette types  */
#define RGBW_PAL          	0
#define W_PAL                   1
#define W_HALF_PAL              2
#define PASTEL_PAL              3
#define RGBW_LIGHTNING_PAL      4
#define W_LIGHTNING_PAL         5
#define W_HALF_LIGHTNING_PAL    6
#define PASTEL_LIGHTNING_PAL    7
#define RED 0
#define GREEN 1
#define BLUE 2
#define NUM_PALETTE_TYPES       8

#define NUM_IMAGE_FUNCTIONS 40
#define NOAHS_FACE   0

enum acidwarp_command {
  CMD_PAUSE = 1,
  CMD_SKIP,
  CMD_QUIT,
  CMD_NEWPAL,
  CMD_LOCK,
  CMD_PAL_FASTER,
  CMD_PAL_SLOWER
};

void handleinput(enum acidwarp_command cmd);
void handleresize(int newwidth, int newheight);
void generate_image_float(int imageFuncNum, UCHAR *buf_graf,
                          int xcenter, int ycenter,
                          int width, int height,
                          int colors, int pitch, int normalize);
void generate_image(int imageFuncNum, UCHAR *buf_graf,
                    int xcenter, int ycenter,
                    int width, int height,
                    int colors, int pitch);
void fatalSDLError(const char *msg);
void quit(int retcode);
void makeShuffledList(int *list, int listSize);
#ifdef ENABLE_WORKER
void startloop(void);
#endif

#define DRAW_LOGO 1
#define DRAW_FLOAT 2
#define DRAW_SCALED 4
void draw_init(int flags);
void draw_same(void);
void draw_next(void);
void draw_abort(void);
void draw_quit(void);
extern int abort_draw;
