/* display.h */

void disp_setPalette(unsigned char *palette);
void disp_beginUpdate(void);
void disp_finishUpdate(void);
void disp_processInput(void);
void disp_init(void);

/* FIXME remove these */
extern int XMax, YMax;
extern UCHAR *buf_graf;
extern unsigned int buf_graf_stride;
extern int SKIP;
void handleInputChar(int c);
