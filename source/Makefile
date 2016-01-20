# Project: packLZH
# Makefile created by Matthias Stirner 01/30/2012
# Working with GCC 4.6.0

CC      = gcc
CPP     = g++
RC      = windres -O coff
RM      = del
CFLAGS  = -DDEV_BUILD -I. -O3 -Wall -Wno-long-long -pedantic -funroll-loops -ffast-math -fsched-spec-load -fomit-frame-pointer
LDFLAGS = -s -static-libgcc -static-libstdc++
DEPS    = vliprecalc.h vlicoder.h huffcoder.h lzcoder.h lzhcoder.h bitops.h plzhtbl.h Makefile
OBJ     = vlicoder.o huffcoder.o lzcoder.o lzhcoder.o bitops.o packlzh.o
RES     = icons.res
BIN     = packLZH

%.o: %.cpp $(DEPS)
	$(CPP) -c -o $@ $< $(CFLAGS)
	
%.res: %.rc
	$(RC) $< $@
	
$(BIN): $(OBJ) $(RES)
	$(CPP) -o $@ $^ $(LDFLAGS)

clean:
	$(RM) *.o
