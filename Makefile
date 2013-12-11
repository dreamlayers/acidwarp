PLATFORM := $(shell uname -o)
ifeq ($(PLATFORM),Emscripten)

CC = emcc
CFLAGS = -g -O2 -Wall -Wmissing-prototypes
EXESUFFIX = .html
LDFLAGS = $(CFLAGS)

# Firefox doesn't like this line
.PHONY : bugfix
bugfix : $(TARGET)
	sed -i 's,^          Module\[.canvas.\].exitPointerLock();$$,//&,' \
	    acidwarp.js

else
ifeq ($(PLATFORM),Cygwin)

CFLAGS = -g -O2 -Wall -I/usr/local/include/SDL -mno-cygwin -DSDL
EXESUFFIX = .exe
LIBS = -L/usr/local/lib -lSDL -lgdi32 -lwinmm -lddraw -ldxguid
CC = gcc
LDFLAGS = $(CFLAGS) -static

else

CFLAGS = -Wall -Wmissing-prototypes -g -O2 $(shell sdl-config --cflags) -DSDL
LIBS = $(shell sdl-config --libs)
LDFLAGS = $(CFLAGS)

endif
endif

LINK = $(CC)
SOURCES = acidwarp.c bit_map.c lut.c palinit.c rolnfade.c
TARGET = acidwarp$(EXESUFFIX)
OBJECTS = $(SOURCES:%.c=%.o)

$(TARGET): $(OBJECTS)
	@rm -f $(TARGET)
	$(LINK) $(LDFLAGS) $^ -o $@ $(LIBS)

acidwarp.o: acidwarp.c handy.h acidwarp.h lut.h bit_map.h \
 palinit.h rolnfade.h warp_text.c
bit_map.o: bit_map.c handy.h bit_map.h
lut.o: lut.c handy.h lut.h
palinit.o: palinit.c handy.h acidwarp.h palinit.h
rolnfade.o: rolnfade.c handy.h acidwarp.h rolnfade.h palinit.h

clean:
	$(RM) *.o $(TARGET)
