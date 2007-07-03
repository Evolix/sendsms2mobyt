CC=gcc
CFLAGS=-W -Wall -O2
LDFLAGS=
EXEC=sendsms2mobyt

all: $(EXEC)

sendsms2mobyt: sendsms2mobyt.c
	$(CC) -o $@ $< $(CFLAGS)

clean:
	rm -rf *.o

mrproper: clean
	rm -rf $(EXEC)
