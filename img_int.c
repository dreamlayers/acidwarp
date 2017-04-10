/* ACID WARP (c)Copyright 1992, 1993 by Noah Spurrier
 * All Rights reserved. Private Proprietary Source Code by Noah Spurrier
 * Ported to Linux by Steven Wills
 * Ported to SDL by Boris Gjenero
 */

#include "handy.h"
#include "acidwarp.h"
#include "lut.h"

/* Fixed point image generator using lookup tables goes here */
#define mod(x, y) ((x) % (y))
#define xor(x, y) ((x) ^ (y))
#include "gen_img.c"
