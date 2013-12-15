/* display.h */

void disp_setPalette(unsigned char *palette);
void disp_beginUpdate(void);
void disp_finishUpdate(void);
void disp_processInput(void);
#define DISP_FULLSCREEN 1
#define DISP_DESKTOP_RES_FS 2
void disp_init(int width, int height, int flags);

/* TODO remove these */
extern UCHAR *buf_graf;
extern unsigned int buf_graf_stride;
