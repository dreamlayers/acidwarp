PREFIX := /usr/local
CFLAGS := -g -O2 -Wall -Wmissing-prototypes
SOURCES := acidwarp.c palinit.c rolnfade.c display.c
OBJECTS = $(SOURCES:%.c=%.o)

ifeq ($(GL),1)
# OpenGL ES / WebGL builds require SDL 2
SDL := 2
else
SDL := 1
endif

ifdef EMMAKEN_COMPILER
# Using emmake make to build using Emscripten
PLATFORM := Emscripten
else
PLATFORM := $(shell uname)
endif

ifeq ($(SDL),2)
SDL_CONFIG := sdl2-config
else
SDL_CONFIG := sdl-config
endif

ifeq ($(PLATFORM),Emscripten)
WORKER_SOURCES := bit_map.c lut.c img_int.c img_float.c worker.c
WORKER_LDFLAGS := $(CFLAGS) -s BUILD_AS_WORKER=1
WORKER_OBJECTS := $(WORKER_SOURCES:%.c=%.o)
SOURCES += useworker.c
all: worker.js
worker.js: $(WORKER_OBJECTS)
	@rm -f $@
	$(LINK) $(WORKER_LDFLAGS) $^ -o $@
CC = emcc
ifeq ($(SDL),2)
CFLAGS += -s USE_SDL=2
endif
TARGET := acidwarp.html
$(TARGET): template.html
LDFLAGS := $(CFLAGS) --shell-file template.html

else

CONVERTEXISTS := $(shell command -v convert > /dev/null 2>&1 && \
                   convert -version 2> /dev/null | grep ImageMagick)
ifdef CONVERTEXISTS
CFLAGS += -DADDICON
SOURCES += acid_ico.c
endif

CFLAGS += $(shell $(SDL_CONFIG) --cflags)
LIBS := $(shell $(SDL_CONFIG) --libs) -lm
ifeq ($(GL),1)
ifeq ($(PLATFORM),Darwin)
LIBS += -framework OpenGL
else ifneq (,$(findstring CYGWIN,$(PLATFORM)))
LIBS += -lglew32 -lopengl32
else
LIBS += -lGL
endif
CFLAGS += -DWITH_GL
endif

ifneq (,$(findstring CYGWIN,$(PLATFORM)))
EXESUFFIX = .exe
CC := i686-w64-mingw32-gcc
OBJECTS += acid_res.o
endif

LDFLAGS = $(CFLAGS)
TARGET = acidwarp$(EXESUFFIX)
endif

LINK = $(CC)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@rm -f $(TARGET)
	$(LINK) $(LDFLAGS) $(OBJECTS) -o $@ $(LIBS)

acidwarp.o: acidwarp.c handy.h acidwarp.h lut.h bit_map.h \
 palinit.h rolnfade.h warp_text.c display.h gen_img.c
bit_map.o: bit_map.c handy.h bit_map.h
lut.o: lut.c handy.h lut.h
palinit.o: palinit.c handy.h acidwarp.h palinit.h
rolnfade.o: rolnfade.c handy.h acidwarp.h rolnfade.h palinit.h display.h
display.o: display.c display.h acidwarp.h handy.h
img_float.o: img_float.c gen_img.c acidwarp.h
draw.o: draw.c acidwarp.h
ifneq (,$(findstring CYGWIN,$(PLATFORM)))
acidwarp.ico: acidwarp.png
	icotool -c -o $@ $^
acid_res.o: acid_res.rc acidwarp.ico
	windres $< $@
endif
# Using ImageMagick to nearest neighbour resize icon for SDL.
# Without it, you can manually do this in another program.
acidwarp.rgb: acidwarp.png
	convert $< -sample 64x64 $@
acid_ico.c: acidwarp.rgb
	xxd -i $< > $@

.PHONY: clean install uninstall

clean:
	$(RM) *.o $(TARGET) acidwarp.ico acidwarp.rgb acid_ico.c \
          acidwarp.html.mem acidwarp.js

install: $(TARGET) acidwarp.png acidwarp.desktop
	install $< $(PREFIX)/bin
	xdg-icon-resource install --novendor --context apps \
	                          --size 256 acidwarp.png acidwarp
	xdg-desktop-menu install --novendor acidwarp.desktop

uninstall:
	rm -f $(PREFIX)/bin/$(TARGET)
	xdg-icon-resource uninstall --novendor --context apps \
	                            --size 256 acidwarp
	xdg-desktop-menu uninstall --novendor acidwarp.desktop
