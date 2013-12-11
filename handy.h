#ifndef __HANDY
#define __HANDY 1

/* Two new control structures for C. Yay! */
#define loop      for (;;)
#define loopend(x) {if(x)break;}
/* Usage:  do { ... } until (boolean); */
#define until(a)  while (!(a))

/* Some basic data types */
/* typedef unsigned char   CHAR; Is this a typo */
typedef unsigned char   UCHAR;
typedef unsigned int    UINT;
typedef unsigned long   ULONG;
typedef enum {FALSE, TRUE} BOOL;

/* Random number stuff that SHOULD have been there */
#include <time.h>
#include <stdio.h> /* Needed for NULL * */
#include <stdlib.h>
#define RANDOMIZE() (srand((UINT)time( (time_t *)NULL )))
#define RANDOM(a) (rand()%(a))

/* Mini-benchmarking tools. Only one second accuracy */
time_t __HANDY_BENCH;
#define START() (__HANDY_BENCH = time (& __HANDY_BENCH))
#define MARK()  ((long)time ((time_t *)0) - __HANDY_BENCH)

/* Stuff that's already there, but is faster as a MACRO */
#define MIN(a,b)  (((a) < (b)) ?  (a) : (b) )
#define MAX(a,b)  (((a) > (b)) ?  (a) : (b) )

#define ABS(a)    (((a) < 0  ) ? -(a) : (a) )
#define FABS(a)   (((a) < F 0) ? -(a) : (a) )
#define DABS(a)   (((a) < 0.0) ? -(a) : (a) )

#define SIGN(a)   (((a) < 0  ) ?   -1 :   1 )
#define FSIGN(a)  (((a) < F 0) ? F(-1): F 1 )
#define DSIGN(a)  (((a) < 0.0) ? -1.0 : 1.0 )

/* This seems nifty to me */
#define DONE        1
#define NOT_DONE		0

#endif
