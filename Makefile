CFLAGS := -g -O2 -Wall -Wmissing-prototypes
SOURCES := acidwarp.c bit_map.c lut.c palinit.c rolnfade.c display.c img_float.c
OBJECTS := $(SOURCES:%.c=%.o)

PLATFORM := $(shell uname -o)
ifeq ($(PLATFORM),Emscripten)

CC = emcc
EXESUFFIX = .html
LDFLAGS = $(CFLAGS) --pre-js pre.js

else

CFLAGS += $(shell sdl-config --cflags)
LIBS = $(shell sdl-config --libs) -lm

ifeq ($(PLATFORM),Cygwin)
EXESUFFIX = .exe
CC := i686-w64-mingw32-gcc
OBJECTS += acid_res.o
endif

LDFLAGS = $(CFLAGS)
endif

LINK = $(CC)
TARGET = acidwarp$(EXESUFFIX)

$(TARGET): $(OBJECTS)
	@rm -f $(TARGET)
	$(LINK) $(LDFLAGS) $^ -o $@ $(LIBS)

acidwarp.o: acidwarp.c handy.h acidwarp.h lut.h bit_map.h \
 palinit.h rolnfade.h warp_text.c display.h gen_img.c
bit_map.o: bit_map.c handy.h bit_map.h
lut.o: lut.c handy.h lut.h
palinit.o: palinit.c handy.h acidwarp.h palinit.h
rolnfade.o: rolnfade.c handy.h acidwarp.h rolnfade.h palinit.h display.h
display.o: display.c display.h acidwarp.h handy.h
img_float.o: img_float.c gen_img.c acidwarp.h
ifeq ($(PLATFORM),Cygwin)
acidwarp.ico: acidwarp.png
	icotool -c -o $@ $^
acid_res.o: acid_res.rc acidwarp.ico
	windres $< $@
endif

clean:
	$(RM) *.o $(TARGET) acidwarp.ico
