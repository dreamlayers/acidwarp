/* display.h */

void disp_setPalette(unsigned char *palette);
void disp_beginUpdate(void);
void disp_finishUpdate(void);
void disp_processInput(void);
void disp_init(int width, int height);

/* TODO remove these */
extern UCHAR *buf_graf;
extern unsigned int buf_graf_stride;
