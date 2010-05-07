CC = g++
LD = g++
CFLAGS = -O3 -Wall `pkg-config --cflags gtk+-2.0`
LDFLAGS = `pkg-config --libs gtk+-2.0` -lXsp -lX11 -lpthread -lasound

SRC = .
ifndef BIN
 BIN = .
endif
RM = rm -f
OBJS = $(BIN)/scope.o $(BIN)/sound.o $(BIN)/gui.o $(BIN)/datastream.o
PROG = $(BIN)/PhoneScope
VERS = 0.1.0
 
.PHONY: clean distclean
all: $(PROG)

$(PROG): $(OBJS)
	$(LD) $(LDFLAGS) -s -o $(PROG) $(OBJS) -lfftw3
$(BIN)/scope.o: $(SRC)/scope.cpp $(SRC)/scope.h
	$(CC) -o $(BIN)/scope.o $(CFLAGS) -O2 -c $(SRC)/scope.cpp
$(BIN)/sound.o: $(SRC)/sound.cpp $(SRC)/sound.h
	$(CC) -o $(BIN)/sound.o $(CFLAGS) -O2 -c $(SRC)/sound.cpp
$(BIN)/gui.o: $(SRC)/gui.cpp $(SRC)/gui.h
	$(CC) -o $(BIN)/gui.o $(CFLAGS) -O2 -c $(SRC)/gui.cpp
$(BIN)/datastream.o: $(SRC)/datastream.h $(SRC)/datastream.cpp
	$(CC) -o $(BIN)/datastream.o $(CFLAGS) -O2 -c $(SRC)/datastream.cpp



clean:
	$(RM) $(OBJS) $(BIN)/*~ $(PROG)
.PHONY: clean
