#ifndef __WORKER
#define __WORKER 1

typedef struct {
  /* Image which worker should respond with */
  int want;
  /* Image which worker should compute after response */
  int next;
  /* Flags affecting both images */
  int flags;
  /* Dimensions for both images */
  unsigned int width, height;
} worker_param;

#endif /* !__WORKER */
