CFLAGS = -g -O2 -Wall -Wmissing-prototypes

PLATFORM := $(shell uname -o)
ifeq ($(PLATFORM),Emscripten)

CC = emcc
EXESUFFIX = .html
LDFLAGS = $(CFLAGS) --pre-js pre.js

else

CFLAGS += $(shell sdl-config --cflags)
LIBS = $(shell sdl-config --libs)

ifeq ($(PLATFORM),Cygwin)

EXESUFFIX = .exe
CC = i686-pc-mingw32-gcc
LDFLAGS = $(CFLAGS)

else

LDFLAGS = $(CFLAGS)

endif
endif

LINK = $(CC)
SOURCES = acidwarp.c bit_map.c lut.c palinit.c rolnfade.c display.c
TARGET = acidwarp$(EXESUFFIX)
OBJECTS = $(SOURCES:%.c=%.o)

$(TARGET): $(OBJECTS)
	@rm -f $(TARGET)
	$(LINK) $(LDFLAGS) $^ -o $@ $(LIBS)

acidwarp.o: acidwarp.c handy.h acidwarp.h lut.h bit_map.h \
 palinit.h rolnfade.h warp_text.c display.h
bit_map.o: bit_map.c handy.h bit_map.h
lut.o: lut.c handy.h lut.h
palinit.o: palinit.c handy.h acidwarp.h palinit.h
rolnfade.o: rolnfade.c handy.h acidwarp.h rolnfade.h palinit.h display.h
display.o: display.c display.h acidwarp.h handy.h

clean:
	$(RM) *.o $(TARGET)
