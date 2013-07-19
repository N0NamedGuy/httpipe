CC=gcc
BIN=httpipe
SRC=httpipe.c
CFLAGS=-Wall
INSTALL=install
BINDIR=/usr/local/bin

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) $? -o $(BIN)

install: $(BIN)
	$(INSTALL) -m 755 $(BIN) $(BINDIR)

uninstall:
	rm $(BINDIR)/$(BIN)

clean:
	rm $(BIN)

