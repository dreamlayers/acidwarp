#define X_TITLE		80
#define Y_TITLE		98/*92*/

#define NOAHS_FACE	0

void bit_map_uncompress (UCHAR far *buf_graf, UCHAR far *bit_data, int x_map, int y_map, int xmax, int ymax);
void writeBitmapImageToArray(UCHAR far *buf_graf, int image_number, int xmax, int ymax);

