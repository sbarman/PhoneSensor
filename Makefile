CC = g++
LD = g++
CFLAGS = -O3 -Wall `sdl-config --cflags`
LDFLAGS = `sdl-config --libs` -lSDL_image -lXsp -lX11 -lpthread -lasound

SRC = src/c
BIN = bin
RM = rm -f
OBJS = $(BIN)/scope.o $(BIN)/sound.o $(BIN)/graphics.o
PROG = $(BIN)/PhoneScope
SIN = $(BIN)/SinGen
VERS = 0.1.0
 
.PHONY: clean distclean
all: $(PROG)
sin: $(SIN)

$(PROG): $(OBJS)
	$(LD) $(LDFLAGS) -s -o $(PROG) $(OBJS) -lfftw3
$(BIN)/scope.o: $(SRC)/scope.cpp $(SRC)/scope.h
	$(CC) -o $(BIN)/scope.o $(CFLAGS) -O2 -c $(SRC)/scope.cpp
$(BIN)/sound.o: $(SRC)/sound.cpp $(SRC)/sound.h
	$(CC) -o $(BIN)/sound.o $(CFLAGS) -O2 -c $(SRC)/sound.cpp
$(BIN)/graphics.o: $(SRC)/graphics.cpp $(SRC)/graphics.h
	$(CC) -o $(BIN)/graphics.o $(CFLAGS) -O2 -c $(SRC)/graphics.cpp

$(SIN): $(BIN)/sin.o
	$(LD) $(LDFLAGS) -s -o $(SIN) $(BIN)/sin.o -lfftw3
$(BIN)/sin.o: $(SRC)/sin.cpp
	$(CC) -o $(BIN)/sin.o $(CFLAGS) -O2 -c $(SRC)/sin.cpp

clean:
	$(RM) $(OBJS) $(BIN)/*~ $(PROG)
.PHONY: clean
