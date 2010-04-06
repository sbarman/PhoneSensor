CC = g++
LD = g++
CFLAGS = -O3 -Wall `sdl-config --cflags`
LDFLAGS = `sdl-config --libs` -lSDL_image -lXsp -lX11 -lpthread -lasound
RM = /bin/rm -f
OBJS = Scope.o
PROG = PhoneScope
VERS = 0.1.0
 
.PHONY: clean distclean
all: $(PROG)
$(PROG): $(OBJS)
	$(LD) $(LDFLAGS) -s -o $(PROG) $(OBJS) -lfftw3
Scope.o: Scope.cpp sound.h graphics.h
	$(CC) $(CFLAGS) -O2 -c Scope.cpp
clean:
	$(RM) *~ $(OBJS) $(PROG)
.PHONY: clean
