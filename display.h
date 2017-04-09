/* display.h */

void disp_setPalette(unsigned char *palette);
void disp_beginUpdate(UCHAR **p, unsigned int *pitch,
                      unsigned int *w, unsigned int *h);
void disp_finishUpdate(void);
void disp_swapBuffers(void);
void disp_processInput(void);
#define DISP_FULLSCREEN 1
#define DISP_DESKTOP_RES_FS 2
void disp_init(int width, int height, int flags);
void disp_quit(void);

/* Callback */
void stopdrawing(void);
