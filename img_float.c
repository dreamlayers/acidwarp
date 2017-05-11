/* Floating point image generator for Acidwarp */

#define _USE_MATH_DEFINES
#include <math.h>
#include "handy.h"
#include "acidwarp.h"

#define ENABLE_FLOAT

#define mod(x, y) fmod((x), (y))
#define xor(x, y) ((int)(x) ^ (int)(y))

/* ANGLE_UNIT corresponds to M_PI * 2, the full circle in radians.
 * To avoid visible seams, resulting colours need to go from 0 to 254,
 * with each getting an equal slice of the circle.
 */
#define ANGLE_UNIT                  (255.0)
#define ANGLE_UNIT_2                (ANGLE_UNIT*2)
#define ANGLE_UNIT_HALF             (ANGLE_UNIT/2)
#define ANGLE_UNIT_EIGHTH           (ANGLE_UNIT/8)
#define ANGLE_UNIT_QUART            (ANGLE_UNIT/4)
#define ANGLE_UNIT_THREE_QUARTERS   (ANGLE_UNIT*3/4)

/* TRIG_UNIT is full scale of trig functions. It was changed to
 * try to avoid visible seams where the palette wraps.
 */
#define TRIG_UNIT                   (ANGLE_UNIT*2)

static double lut_sin (double a)
{
    return TRIG_UNIT * sin(a * M_PI * 2 / ANGLE_UNIT);
}


static double lut_cos (double a)
{
    return TRIG_UNIT * cos(a * M_PI * 2 / ANGLE_UNIT);
}

static double lut_angle (double dx, double dy)
{
    double angle = atan2(dy, dx) * ANGLE_UNIT / (M_PI * 2);
    /* Always return a positive result */
    if (angle < 0.0) angle += (double)ANGLE_UNIT_2;
    return angle;
}

static double lut_dist (double x, double y)
{
  return sqrt(x*x + y*y);
}

#include "gen_img.c"
