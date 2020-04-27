INSTALL_DIR = /usr/lib/ladspa/

OS := $(shell uname -s)

CC      = gcc
CFLAGS  = -I. -O3 -Wall -fomit-frame-pointer -funroll-loops -ffast-math -c -fPIC -DPIC
LDFLAGS = -nostartfiles -shared -Wl,-Bsymbolic -lc -lm -lrt

all: spleeter_ladspa.so

spleeter_ladspa.o: spleeter_ladspa.c request.c ladspa.h
	$(CC) $(CFLAGS) $< -o $@

spleeter_ladspa.so: spleeter_ladspa.o
	$(CC) -o $@ $< $(LDFLAGS)

install: all
	-mkdir -p $(INSTALL_DIR)
	cp *.so $(INSTALL_DIR)

clean:
	-rm -f *.so *.o *~

.PHONY: all install clean
