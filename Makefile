CC=gcc
CFLAGS = -O2 -funroll-all-loops
LFLAGS = -lvgagl -lvga
PROGS = acidwarp 
OSOURCES = bit_map.o palinit.o rolnfade.o lut.o

acidwarp: $(OSOURCES)
	$(CC) acidwarp.c $(CFLAGS) $(LFLAGS) $(OSOURCES) -o acidwarp ; strip acidwarp
bit_map.o: 
palinit.o:
rolnfade.o:
lut.o:
clean:
	rm -f acidwarp *.o
