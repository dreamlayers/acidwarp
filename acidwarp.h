/* ACIDWARP.H */

#define VERSION "Acid Warp Version 4.10 (C)Copyright 1992, 1993 by Noah Spurrier and Mark Bilk\nLinux port by Steven Wills"
#define DIRECTN_CHANGE_PERIOD_IN_TICKS               256

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

void printStrArray(char *strArray[]);
void rotatePalette(void);
void rotateforward(int color, UCHAR *Pal);
void rotatebackward(int color, UCHAR *Pal);
void roll_rgb_palArray(UCHAR *Pal);
void makeShuffledList(int *list, int listSize);
void commandline(int argc, char *argv[]);
void graphicsinit();
int generate_image(int imageFuncNum, UCHAR *buf_graf, int xcenter, int ycenter, int xmax, int ymax, int colormax);
void processinput();
void newpal();









