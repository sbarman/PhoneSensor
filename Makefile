CC = g++
LD = g++
CFLAGS = -O3 -Wall `sdl-config --cflags`
LDFLAGS = `sdl-config --libs` -lSDL_image -lXsp -lX11 -lpthread -lasound

SRC = src/c
BIN = bin
RM = rm -f
OBJS = $(BIN)/Scope.o
PROG = $(BIN)/PhoneScope
VERS = 0.1.0
 
.PHONY: clean distclean
all: $(PROG)
$(PROG): $(OBJS)
	$(LD) $(LDFLAGS) -s -o $(PROG) $(OBJS) -lfftw3
$(BIN)/Scope.o: $(SRC)/Scope.cpp $(SRC)/sound.h $(SRC)/graphics.h
	$(CC) -o $(BIN)/Scope.o $(CFLAGS) -O2 -c $(SRC)/Scope.cpp
clean:
	$(RM) $(BIN)/*
.PHONY: clean
