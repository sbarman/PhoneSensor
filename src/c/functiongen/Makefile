CC = g++
LD = g++
CFLAGS = -O3 -Wall `sdl-config --cflags` `pkg-config --cflags gtk+-2.0`
LDFLAGS = `sdl-config --libs` `pkg-config --libs gtk+-2.0` -lSDL_image -lXsp -lX11 -lpthread -lasound 

SRC = .
ifndef BIN
 BIN = .
endif
RM = rm -f
OBJS = $(BIN)/sin.o $(BIN)/gui.o
PROG = $(BIN)/FunctionGen
VERS = 0.1.0
 
 
.PHONY: clean distclean
all: $(PROG)

$(PROG): $(OBJS)
	$(LD) $(LDFLAGS) -s -o $(PROG) $(OBJS) -lfftw3
$(BIN)/sin.o: $(SRC)/sin.cpp $(SRC)/sin.h
	$(CC) -o $(BIN)/sin.o $(CFLAGS) -O2 -c $(SRC)/sin.cpp
$(BIN)/gui.o: $(SRC)/gui.cpp $(SRC)/gui.h
	$(CC) -o $(BIN)/gui.o $(CFLAGS) -O2 -c $(SRC)/gui.cpp

clean:
	$(RM) $(OBJS) $(BIN)/*~ $(PROG)
.PHONY: clean
