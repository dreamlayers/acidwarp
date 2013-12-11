# Make file for Acid Warp (c)Copyright 1993
PROG_NAME = acidwarp
 
# Flags for debugging
# /AH Memory model Huge
# /c  compile to object code only, no auto-link
# /Zi Include FULL debugging data
# /Od Turn OFF optimization
### /Gs Turn OFF stack checking (I think this might be a mistake)
# /G2 286 output code, allows REP OUTSB inline assembly in particular
# /W3 Most all warnings are shown; like lint
# /nologo Don't crowd the screen with Microsoft Logo
 
 
# Mark's Switches
CFLAGS = /Gs /Od /AH /Gt /FPi /G2 /c /W3 /nologo
LFLAGS = /MAP /NOE /NOI /ST:7500
 
 
#CFLAGS = /AH /c /Zi /Od /G2 /W3 /nologo
#LFLAGS = /CO
 
# Flags for normal compilation
# CFLAGS = /AH /c /G2
# LFLAGS = /CO
 
 
# Object files for acidwarp.exe
OBJECTS = acidwarp.obj bit_map.obj graf.obj int_hook.obj lut.obj palinit.obj\
rolnfade.obj warp_txt.obj
OBJECTS_LIST = acidwarp+bit_map+graf+int_hook+lut+palinit+rolnfade+warp_txt
 
# I still don't know what this line does
all : acidwarp.exe
 
acidwarp.exe : $(OBJECTS)
  link $(LFLAGS) $(OBJECTS_LIST),$(PROG_NAME).exe,$(PROG_NAME).map;
 
#model:
#.obj : .c .h
#    cl /AH /c /Fo.obj /u .c
 
acidwarp.obj : acidwarp.c acidwarp.h
    cl $(CFLAGS) /Foacidwarp.obj acidwarp.c
 
bit_map.obj : bit_map.c acidwarp.h
    cl $(CFLAGS) /Fbit_map.obj bit_map.c
 
rolnfade.obj : rolnfade.c acidwarp.h
    cl $(CFLAGS) /Forolnfade.obj rolnfade.c
 
warp_txt.obj : warp_txt.c acidwarp.h
    cl $(CFLAGS) /Fowarp_txt.obj warp_txt.c
 
int_hook.obj : int_hook.c acidwarp.h
    cl $(CFLAGS) /Foint_hook.obj int_hook.c
 
lut.obj : lut.c lut.h
    cl $(CFLAGS) /Folut.obj lut.c
 
palinit.obj : palinit.c acidwarp.h
    cl $(CFLAGS) /Fopalinit.obj palinit.c
 
graf.obj : graf.c graf.h
    cl $(CFLAGS) /Fograf.obj graf.c
 

