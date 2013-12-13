/* Floating point equivalents of Acidwarp lookup table functions */

#ifndef __LUT
#define __LUT 1

#include <math.h>

/* Resulting colours need to go from 0 to 254, with
 * each getting an equal slice of the circle.
 */
#define ANGLE_UNIT                  (255.0)
#define TRIG_UNIT                   (ANGLE_UNIT*2)
/* The same idea as 2*PI, PI/2, PI/4, etc. */
#define ANGLE_UNIT_2                (ANGLE_UNIT*2)
#define ANGLE_UNIT_HALF             (ANGLE_UNIT/2)
#define ANGLE_UNIT_EIGHTH           (ANGLE_UNIT/8)
#define ANGLE_UNIT_QUART            (ANGLE_UNIT/4)
#define ANGLE_UNIT_THREE_QUARTERS   (ANGLE_UNIT*3/4)


static double lut_sin (double a)
{
    return TRIG_UNIT * sin(a * M_PI / ANGLE_UNIT);
}


static double lut_cos (double a)
{
    return TRIG_UNIT * cos(a * M_PI / ANGLE_UNIT);
}

static double lut_angle (double dx, double dy)
{
    double angle = atan2(dy, dx) * ANGLE_UNIT / M_PI;
    /* Always return a positive result */
    if (angle < 0.0) angle += (double)ANGLE_UNIT_2;
    return angle;
}

static double lut_dist (double x, double y)
{
  return sqrt(x*x + y*y);
}

#endif /* !__LUT */
