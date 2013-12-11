/* This generates look up tables (LUT's) for stuff. Yeah. */
/* It's just a loop. */

#include <stdio.h>
#include <math.h>
#define AU ((double)1024)
#define PI2 ((double)6.283185308)

void main (int argc, char * argv[])
{
 int c, res;    /* counter, resolution */

 res = atoi (argv[1]);

 for (c = 0; c < res; ++c)
 {
/*   printf ("%4d,", (int)sqrtl (((long double)res*(long double)res) + ((long double)c*(long double)c)));
*/
   printf ("%4ld,", (long) (atan (
                                ((double)c / (double)res)  ) * AU
                    / PI2)
   );
   if (!((c+1)%10))
     printf ("\n");
  }
}
