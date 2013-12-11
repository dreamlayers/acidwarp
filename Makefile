CFLAGS = -g -O2 -Wall -I/usr/local/include/SDL -mno-cygwin -DSDL
EXESUFFIX = .exe
LIBS = -L/usr/local/lib -lSDL -lgdi32 -lwinmm -lddraw -ldxguid
CC = gcc

LINK = $(CC)
LDFLAGS = $(CFLAGS) -static
SOURCES = acidwarp.c bit_map.c lut.c palinit.c rolnfade.c
TARGET = acidwarp$(EXESUFFIX)
OBJECTS = acidwarp.o bit_map.o lut.o palinit.o rolnfade.o

acidwarp: $(OBJECTS)
	@rm -f acidwarp
	$(LINK) $(LDFLAGS) $(OBJECTS) -o $(TARGET) $(LIBS)

acidwarp.o: acidwarp.c handy.h acidwarp.h lut.h bit_map.h \
 palinit.h rolnfade.h warp_text.c
	$(CC) $(CFLAGS) -c acidwarp.c -o acidwarp.o

bit_map.o: bit_map.c handy.h bit_map.h
	$(CC) $(CFLAGS) -c bit_map.c -o bit_map.o

lut.o: lut.c handy.h lut.h
	$(CC) $(CFLAGS) -c lut.c -o lut.o

palinit.o: palinit.c handy.h acidwarp.h palinit.h
	$(CC) $(CFLAGS) -c palinit.c -o palinit.o

rolnfade.o: rolnfade.c handy.h acidwarp.h rolnfade.h palinit.h
	$(CC) $(CFLAGS) -c rolnfade.c -o rolnfade.o

clean:
	$(RM) *.o $(TARGET)
	