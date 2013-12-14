#include <stdio.h>
#include <string.h>
#include <math.h>
#include "lut.h"
#include "palinit.c"

static void print_lut(char *arg)
{
  int angle;

  printf("ANGLE_UNIT, %i\n", ANGLE_UNIT);
  printf("TRIG_UNIT, %i\n\n", TRIG_UNIT);

  if (arg == NULL || strcmp(arg, "lut_sin") == 0) {
    printf("angle, lut_sin(angle), sin(angle)*TRIG_UNIT\n");
    for (angle = 0; angle < ANGLE_UNIT+5; angle++) {
      printf("%i, %li, %f\n", angle, lut_sin(angle),
             sin((double)angle * M_PI * 2 / ANGLE_UNIT) * TRIG_UNIT);
    }
    printf("\n");
  }

  if (arg == NULL || strcmp(arg, "lut_angle") == 0) {
    printf("\nangle, lut_angle(x,y), atan2(y,x)\n");
    for (angle = 0; angle < ANGLE_UNIT+5; angle++) {
      double x, y, a;
      x = cos((double)angle * M_PI * 2 / ANGLE_UNIT) * 100;
      y = sin((double)angle * M_PI * 2 / ANGLE_UNIT) * 100;
      a = atan2(y, x);
      if (a < 0.0) a += M_PI * 2;
      printf("%i, %li, %f\n", angle,
             lut_angle(x, y), a * ANGLE_UNIT / (M_PI * 2));
    }
    printf("\n");
  }
}

static void print_palette(UCHAR *pal)
{
  int color;

  printf("index, red, green, blue\n");
  for (color = 0; color <= 255; color++) {
    printf("%i, %i, %i, %i\n", color,
           pal[color * 3], pal[color * 3 + 1], pal[color * 3 + 2]);
  }
  printf("\n");
}

static void test_palette(void (*func)(UCHAR *), const char *name, char *arg)
{
  UCHAR pal[256 * 3];

  if (arg == NULL || strcmp(arg, name) == 0) {
    printf("%s\n", name);
    memset(pal, 0, sizeof(pal));
    func(pal);
    print_palette(pal);
  }
}

#define TEST_PALETTE(palette) test_palette(palette, #palette, arg)

static void test_palettes(char *arg)
{
  TEST_PALETTE(init_rgbw_palArray);
  TEST_PALETTE(init_w_palArray);
  TEST_PALETTE(init_w_half_palArray);
  TEST_PALETTE(init_pastel_palArray);
}

#undef TEST_PALETTE

int main(int argc, char ** argv)
{
  char *arg = NULL;
  if (argc == 2) arg = argv[1];

  print_lut(arg);
  test_palettes(arg);

  return 0;
}
